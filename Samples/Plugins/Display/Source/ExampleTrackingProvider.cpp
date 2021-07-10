#include "XR/IUnityXRInput.h"
#include "XR/IUnityXRTrace.h"

#include "ProviderContext.h"

#include <cmath>

class ExampleTrackingProvider : public ProviderImpl
{
public:
    ExampleTrackingProvider(ProviderContext& ctx, UnitySubsystemHandle handle)
        : ProviderImpl(ctx, handle)
    {
    }
    virtual ~ExampleTrackingProvider() {}

    UnitySubsystemErrorCode Initialize() override;
    UnitySubsystemErrorCode Start() override;

    UnitySubsystemErrorCode Tick(UnityXRInputUpdateType updateType);
    UnitySubsystemErrorCode FillDeviceDefinition(UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceDefinition* definition);
    UnitySubsystemErrorCode UpdateDeviceState(UnityXRInternalInputDeviceId deviceId, UnityXRInputUpdateType updateType, UnityXRInputDeviceState* state);
    UnitySubsystemErrorCode HandleEvent(unsigned int eventType, UnityXRInternalInputDeviceId deviceId, void* buffer, unsigned int size);
    UnitySubsystemErrorCode TryGetDeviceStateAtTime(UnityXRTimeStamp time, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceState* state);

    void Stop() override;
    void Shutdown() override;

private:
    UnityXRPose GetSimPose();
    UnityXRPose GetRenderPose();

private:
    static const int kInputDeviceHMD = 0;
};

UnitySubsystemErrorCode ExampleTrackingProvider::Initialize()
{
    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleTrackingProvider::Start()
{
    m_Ctx.input->InputSubsystem_DeviceConnected(m_Handle, kInputDeviceHMD);
    return kUnitySubsystemErrorCodeSuccess;
}

static float s_Time = 0.0f;

UnitySubsystemErrorCode ExampleTrackingProvider::Tick(UnityXRInputUpdateType updateType)
{
    // Latch poses for sim
    if (updateType == kUnityXRInputUpdateTypeDynamic)
    {
        s_Time += 0.01f;
        if (s_Time > 1.0f)
            s_Time -= 2.0f;
    }

    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleTrackingProvider::FillDeviceDefinition(UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceDefinition* definition)
{
    // Fill in your connected device information here when requested.  Used to create customized device states.
    auto& input = *m_Ctx.input;
    input.DeviceDefinition_SetName(definition, "head tracking");
    input.DeviceDefinition_SetCharacteristics(definition, (UnityXRInputDeviceCharacteristics)(kUnityXRInputDeviceCharacteristicsHeadMounted | kUnityXRInputDeviceCharacteristicsTrackedDevice));
    input.DeviceDefinition_SetManufacturer(definition, "sample");
    input.DeviceDefinition_SetSerialNumber(definition, "1234");

    input.DeviceDefinition_AddFeatureWithUsage(definition, "head position", kUnityXRInputFeatureTypeAxis3D, kUnityXRInputFeatureUsageCenterEyePosition);
    input.DeviceDefinition_AddFeatureWithUsage(definition, "head rotation", kUnityXRInputFeatureTypeRotation, kUnityXRInputFeatureUsageCenterEyeRotation);

    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleTrackingProvider::UpdateDeviceState(UnityXRInternalInputDeviceId deviceId, UnityXRInputUpdateType updateType, UnityXRInputDeviceState* state)
{
    /// Called by Unity when it needs a current device snapshot
    UnityXRPose pose;
    auto& input = *m_Ctx.input;
    switch (updateType)
    {
    case kUnityXRInputUpdateTypeDynamic:
        pose = GetSimPose();
        break;
    case kUnityXRInputUpdateTypeBeforeRender:
        pose = GetRenderPose();
        break;
    default:
        break;
    }

    if (deviceId == kInputDeviceHMD)
    {
        input.DeviceState_SetAxis3DValue(state, 0, pose.position);
        input.DeviceState_SetRotationValue(state, 1, pose.rotation);
    }

    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleTrackingProvider::HandleEvent(unsigned int eventType, UnityXRInternalInputDeviceId deviceId, void* buffer, unsigned int size)
{
    /// Simple, generic method callback to inform the plugin or individual devices of events occurring within unity
    return kUnitySubsystemErrorCodeFailure;
}

UnitySubsystemErrorCode ExampleTrackingProvider::TryGetDeviceStateAtTime(UnityXRTimeStamp time, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceState* state)
{
    /// Unity calls this when requesting a state at a specific time in the past
    return kUnitySubsystemErrorCodeSuccess;
}

void ExampleTrackingProvider::Stop()
{
    m_Ctx.input->InputSubsystem_DeviceDisconnected(m_Handle, kInputDeviceHMD);
}

void ExampleTrackingProvider::Shutdown()
{
}

UnityXRPose ExampleTrackingProvider::GetSimPose()
{
    UnityXRPose ret{};

    const float bound = 3.14159f / 6.0f;
    float r = 0.0f;

    // t:  -1 ->  0 ->   1
    // r: -30 -> 30 -> -30
    if (s_Time < 0.0f)
    {
        r = (s_Time + 0.5f) * 2.0f * bound;
    }
    else
    {
        r = ((1.0f - s_Time) - 0.5f) * 2.0f * bound;
    }

    ret.rotation.y = sin(r / 2.0f);
    ret.rotation.w = cos(r / 2.0f);
    return ret;
}

UnityXRPose ExampleTrackingProvider::GetRenderPose()
{
    return GetSimPose();
}

// Binding to C-API below here

static UnitySubsystemErrorCode UNITY_INTERFACE_API Input_Initialize(UnitySubsystemHandle handle, void* userData)
{
    auto& ctx = GetProviderContext(userData);

    ctx.trackingProvider = new ExampleTrackingProvider(ctx, handle);

    UnityXRInputProvider inputProvider{};
    inputProvider.userData = &ctx;

    inputProvider.Tick = [](UnitySubsystemHandle handle, void* userData, UnityXRInputUpdateType updateType) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.trackingProvider->Tick(updateType);
    };

    inputProvider.FillDeviceDefinition = [](UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceDefinition* definition) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.trackingProvider->FillDeviceDefinition(deviceId, definition);
    };

    inputProvider.UpdateDeviceState = [](UnitySubsystemHandle handle, void* userData, UnityXRInternalInputDeviceId deviceId, UnityXRInputUpdateType updateType, UnityXRInputDeviceState* state) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.trackingProvider->UpdateDeviceState(deviceId, updateType, state);
    };

    inputProvider.HandleEvent = [](UnitySubsystemHandle handle, void* userData, unsigned int eventType, UnityXRInternalInputDeviceId deviceId, void* buffer, unsigned int size) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.trackingProvider->HandleEvent(eventType, deviceId, buffer, size);
    };

    inputProvider.TryGetDeviceStateAtTime = [](UnitySubsystemHandle handle, void* userData, UnityXRTimeStamp time, UnityXRInternalInputDeviceId deviceId, UnityXRInputDeviceState* state) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.trackingProvider->TryGetDeviceStateAtTime(time, deviceId, state);
    };

    ctx.input->RegisterInputProvider(handle, &inputProvider);

    return ctx.trackingProvider->Initialize();
}

UnitySubsystemErrorCode Load_Input(ProviderContext& ctx)
{
    ctx.input = ctx.interfaces->Get<IUnityXRInputInterface>();
    if (ctx.input == nullptr)
        return kUnitySubsystemErrorCodeFailure;

    UnityLifecycleProvider inputLifecycleHandler{};
    inputLifecycleHandler.userData = &ctx;

    inputLifecycleHandler.Initialize = &Input_Initialize;

    inputLifecycleHandler.Start = [](UnitySubsystemHandle handle, void* userData) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.trackingProvider->Start();
    };

    inputLifecycleHandler.Stop = [](UnitySubsystemHandle handle, void* userData) -> void {
        auto& ctx = GetProviderContext(userData);
        ctx.trackingProvider->Stop();
    };

    inputLifecycleHandler.Shutdown = [](UnitySubsystemHandle handle, void* userData) -> void {
        auto& ctx = GetProviderContext(userData);
        ctx.trackingProvider->Shutdown();

        delete ctx.trackingProvider;
    };

    return ctx.input->RegisterLifecycleProvider("XR SDK Display Sample", "Head Tracking Sample", &inputLifecycleHandler);
}