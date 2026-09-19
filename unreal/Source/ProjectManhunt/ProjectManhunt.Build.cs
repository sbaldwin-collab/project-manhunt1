using UnrealBuildTool;

public class ProjectManhunt : ModuleRules
{
	public ProjectManhunt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"UMG",
			"Slate",
			"SlateCore",
			"MotionWarping",
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Niagara",
		});

		// Uncomment once the animation team's Control Rig / IK Rig assets are in place
		// and a retargeted skeleton exists to build against.
		// PrivateDependencyModuleNames.Add("ControlRig");
	}
}
