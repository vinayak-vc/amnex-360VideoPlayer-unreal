using UnrealBuildTool;

public class StereoscopicProject : ModuleRules {
    public StereoscopicProject(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "LidarPointCloudRuntime",
            "UMG",
            "Slate",
            "SlateCore",
            "EnhancedInput",
            "DisplayCluster",
            "PakFile",
            "AssetRegistry"
        });

        // DesktopPlatform (OS file dialog for the Browse button) isn't available
        // in Shipping — only link it for editor/Development configs.
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDependencyModuleNames.Add("DesktopPlatform");
        }
    }
}
