#include "IUnityInterface.h"

#include "IUnityXRDisplay.h"
#include "IUnitySubsystemExample.h"
//#include "IUnityXRInput.h"
#include "IUnityXRMeshing.h"
#include "IUnityXRPreInit.h"
#include "IUnityXRStats.h"
#include "IUnityXRTrace.h"
#include "UnitySubsystemTypes.h"
#include "UnityXRDisplayStats.h"
#include "UnityXRTypes.h"

// Add other subsystem header includes here to include them in the C build test

static IUnitySubsystemExampleInterface* s_SubsystemExample = NULL;

static UnitySubsystemErrorCode UNITY_INTERFACE_API example_UpdateState(UnitySubsystemHandle handle, void* data, UnitySubsystemExampleState * state)
{
    return kUnitySubsystemErrorCodeSuccess;
}

static UnitySubsystemErrorCode UNITY_INTERFACE_API Lifecycle_Initialize(UnitySubsystemHandle handle, void* data)
{
    UnitySubsystemExampleProvider provider;
    provider.UpdateState = &example_UpdateState;
    s_SubsystemExample->RegisterExampleProvider(handle, &provider);

    return kUnitySubsystemErrorCodeSuccess;
}

static UnitySubsystemErrorCode UNITY_INTERFACE_API Lifecycle_Start(UnitySubsystemHandle handle, void* data)
{
    return kUnitySubsystemErrorCodeSuccess;
}

static void UNITY_INTERFACE_API Lifecycle_Stop(UnitySubsystemHandle handle, void* data)
{
}

static void UNITY_INTERFACE_API Lifecycle_Shutdown(UnitySubsystemHandle handle, void* data)
{
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_SubsystemExample = unityInterfaces->GetInterfaceSplit(0xAB695A1C94804266ULL, 0xBDB5A1B347AC54B8ULL);

    UnityLifecycleProvider exampleLifecycleHandler =
    {
        NULL,
        &Lifecycle_Initialize,
        &Lifecycle_Start,
        &Lifecycle_Stop,
        &Lifecycle_Shutdown
    };

    s_SubsystemExample->RegisterLifecycleProvider("PluginName", "PluginID", &exampleLifecycleHandler);
}
