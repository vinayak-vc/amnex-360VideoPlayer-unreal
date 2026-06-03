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
            "EnhancedInput"
        });
    }
}
