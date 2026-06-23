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
            "PointForgeViewer",
        });

        // Package PointForge binaries
        string PointForgeDir = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "../../../../PointForge/build/Release"));
        if (System.IO.Directory.Exists(PointForgeDir))
        {
            RuntimeDependencies.Add("$(ProjectDir)/PointForge/...", PointForgeDir + "/...");
        }
    }
}
