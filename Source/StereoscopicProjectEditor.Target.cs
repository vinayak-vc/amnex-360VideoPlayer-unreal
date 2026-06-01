using UnrealBuildTool;

public class StereoscopicProjectEditorTarget : TargetRules {
    public StereoscopicProjectEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("StereoscopicProject");
    }
}