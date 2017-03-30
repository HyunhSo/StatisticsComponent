# StatisticsComponent
Engine Version 4.15.1
how to setup -> https://www.youtube.com/watch?v=itIDrofWQik

When turning on, missing modules message should pop out, press yes and wait till editor opens.
To generate visual studio files, click RMB on uproject file->Generate visual studio project files.

Only 2 classes are important here, StatisticsComponent and StatBarWidget,
functions defined in StatBarWidget has been implemented in BP_StatBar(blueprint), which are simple 1 node returns.
No changes have been made in ThirdPersonCharacter, BaseCharacter or GameMode classes.

In Build file new modules have been added to be able to use UUserWidget class
 
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UMG" });

PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
  
