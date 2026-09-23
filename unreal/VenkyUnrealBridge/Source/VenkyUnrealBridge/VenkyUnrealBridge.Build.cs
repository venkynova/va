using UnrealBuildTool;

public class VenkyUnrealBridge : ModuleRules
{
    public VenkyUnrealBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "HttpServer",
                "HTTP",
                "Json",
                "JsonUtilities"
            }
        );
    }
}
