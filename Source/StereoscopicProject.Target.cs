using UnrealBuildTool;

public class StereoscopicProjectTarget : TargetRules {
    public StereoscopicProjectTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("StereoscopicProject");
    }
}