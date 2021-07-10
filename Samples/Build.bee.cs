using System;
using System.IO;
using Bee.Core;
using Bee.NativeProgramSupport.Building.FluentSyntaxHelpers;
using Bee.Toolchain.Android;
using Bee.Toolchain.IOS;
using Bee.Toolchain.Windows;
using NiceIO;
using Unity.BuildSystem.MacSDKSupport;
using Unity.BuildSystem.NativeProgramSupport;

class Build
{
    static void Main()
    {
        BuildProviderPlugin("Display");
        BuildProviderPlugin("Input");
    }
    private static void BuildProviderPlugin(string example)
    {
        var np = new NativeProgram($"{example}ProviderSample");
        np.Sources.Add($"Samples/Plugins/{example}/Source");
        np.IncludeDirectories.Add($"Headers");

        var toolchains = new ToolChain[]
        {
            new WindowsToolchain(new Lazy<WindowsSdk>(() => WindowsSdk.Locatorx64.UserDefaultOrLatest).Value),
            new MacToolchain(new Lazy<MacSdk>(() => MacSdk.Locatorx64.UserDefaultOrLatest).Value),
            new AndroidNdkToolchain(new Lazy<AndroidNdk>(() => (AndroidNdk) ToolChain.Store.Android().r16b().Arm64().Sdk).Value),
            new AndroidNdkToolchain(new Lazy<AndroidNdk>(() => (AndroidNdk) ToolChain.Store.Android().r16b().Armv7().Sdk).Value),
            new IOSToolchain(new Lazy<IOSSdk>(() => IOSSdk.LocatorArm64.UserDefaultOrLatest).Value),
        };

        np.OutputName.Add(s => s.Platform is AndroidPlatform, $"lib{np.Name}");

        foreach (var toolchain in toolchains)
        {
            var name = toolchain.Platform.Name.ToLower();
            if (!toolchain.CanBuild)
            {
                Console.WriteLine($"Can't build {toolchain.ToString()}");
                continue;
            }
            
            var nativeProgramConfiguration = new NativeProgramConfiguration(CodeGen.Debug, toolchain, lump: false);

            //we want to build the nativeprogram into an executable. each toolchain can provide different ways of "linking" (static, dynamic, executable are default ones)
            var format = toolchain is IOSToolchain ? toolchain.StaticLibraryFormat : toolchain.DynamicLibraryFormat;

            //and here the magic happens, the nativeprogram gets setup for the specific configuration we just made.
            var folder = name;
            var deployFolder = new NPath($"Samples/Packages/com.unity.xr.sdk.{example.ToLower()}sample/Runtime/").Combine(folder).Combine(toolchain.Architecture.DisplayName);
            var output = np.SetupSpecificConfiguration(nativeProgramConfiguration, format).DeployTo(deployFolder);

            var metaFolder = new NPath("Samples/Packages/Meta/").Combine(folder).Combine(toolchain.Architecture.DisplayName);
            var metaFile = metaFolder.Combine("plugin." + output.Path.Extension + ".meta");
            var deployedMetaFile = deployFolder.Combine(output.Path.FileName + ".meta");
            CopyMetaFileWithNewGUID(metaFile, deployedMetaFile);
            
            var alias = name;
            Backend.Current.AddAliasDependency(alias, output.Path);
            Backend.Current.AddAliasDependency(alias, deployedMetaFile);
        }
    }

    private static void CopyMetaFileWithNewGUID(NPath metaFile, NPath deployedMetaFile)
    {
        string contents = File.ReadAllText(metaFile.ToString());
        contents = contents.Replace("guid:", $"guid: {Guid.NewGuid().ToString().Replace("-", "")}");
        Backend.Current.RegisterFileInfluencingGraph(metaFile);
        Backend.Current.AddWriteTextAction(deployedMetaFile, contents);
    }
}