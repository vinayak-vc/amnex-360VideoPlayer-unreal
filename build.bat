@echo off

call "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" StereoscopicProjectEditor Win64 Development "C:\UnrealProject\StereroScopicProject\StereroScopicProject.uproject" -waitmutex

echo Exit Code: %ERRORLEVEL%

cmd /k