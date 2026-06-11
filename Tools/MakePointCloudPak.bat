@echo off
REM ============================================================================
REM  Machine-2 pipeline: cook a LiDAR point-cloud asset and pak it standalone.
REM  Produces an external .pak you copy to target machines and load at runtime
REM  via APointCloudLoaderActor::LoadFromPak / the UI Browse button.
REM
REM  PREREQS (do once, in editor on this machine):
REM    1. Import your .laz  ->  ULidarPointCloud asset, saved under:
REM         /Game/StreamedClouds/PC_Big   (rename to taste)
REM    2. Project Settings > Packaging > "Use Io Store" = OFF  (legacy pak).
REM
REM  EDIT THE VARIABLES BELOW, then run this .bat.
REM ============================================================================

REM --- Engine + project ---
set ENGINE=C:\Program Files\Epic Games\UE_5.5
set PROJECT=C:\UnrealProject\StereroScopicProject\StereroScopicProject.uproject
set PROJECTNAME=StereroScopicProject

REM --- Asset to ship (package path of the imported cloud, WITHOUT extension) ---
set ASSET=/Game/StreamedClouds/PC_Big

REM --- Output pak ---
set OUTPAK=C:\UnrealProject\out\PointCloud_P.pak

REM ============================================================================
set UAT="%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat"
set UNREALPAK="%ENGINE%\Engine\Binaries\Win64\UnrealPak.exe"
set COOKDIR=%~dp0..\Saved\Cooked\Windows\%PROJECTNAME%\Content

echo.
echo [1/3] Cooking %ASSET% for Windows ...
call %UAT% BuildCookRun -project="%PROJECT%" -noP4 -platform=Win64 -clientconfig=Development ^
    -cook -map= -unattended -nocompileeditor -skipstage -pak=false ^
    -CookCultures=en -targetplatform=Windows
if errorlevel 1 ( echo COOK FAILED & exit /b 1 )

echo.
echo [2/3] Building UnrealPak response file ...
REM Convert /Game/StreamedClouds/PC_Big -> Content\StreamedClouds\PC_Big
set RELPATH=%ASSET:/Game/=Content/%
set RELPATH=%RELPATH:/=\%
set RESP=%~dp0pak_response.txt
> "%RESP%" echo "%COOKDIR%\%RELPATH:Content\=%.uasset" "../../../%PROJECTNAME%/%RELPATH%.uasset"
>> "%RESP%" echo "%COOKDIR%\%RELPATH:Content\=%.uexp" "../../../%PROJECTNAME%/%RELPATH%.uexp"
>> "%RESP%" echo "%COOKDIR%\%RELPATH:Content\=%.ubulk" "../../../%PROJECTNAME%/%RELPATH%.ubulk"
echo Response file:
type "%RESP%"

echo.
echo [3/3] Paking -> %OUTPAK% ...
%UNREALPAK% "%OUTPAK%" -create="%RESP%" -compress
if errorlevel 1 ( echo PAK FAILED & exit /b 1 )

echo.
echo DONE. Copy %OUTPAK% to a target machine, launch the build, Browse to it, Load.
echo Asset object path (if you ever need it explicitly): %ASSET%.PC_Big
