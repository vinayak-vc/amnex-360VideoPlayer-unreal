# StereoscopicProject

An Unreal Engine 5.5 stereoscopic 360° video player for dual-projector / OV output.

## Overview

Real-time stereoscopic 360° video playback system. Two inverted spheres (left/right eye) display equirectangular video using custom UV-mapped materials. A C++ `StereoCapture` actor captures each sphere to separate render targets (`RT_Left`, `RT_Right`), composited via `WBP_StereoOutput` for stereoscopic display.

## Features

- 360° equirectangular video playback via Unreal MediaPlayer
- Per-eye UV splitting (`EyeSelect` parameter in `Left_Mat` / `Right_Mat`)
- C++ `StereoCapture` actor with two modes:
  - **Video Mode** — both captures at same position, UV-based eye separation
  - **Scene Mode** — IPD offset per eye for real geometric parallax
- Blueprint controller (`BP_Controller`) with camera rotation input and video library UI
- Runtime video switching via `VideoFiles` array

## Project Structure

```
Source/StereoscopicProject/
  Public/
    StereoCapture.h       # Stereo capture actor header
    BFL_FileUtils.h       # File utility function library
  Private/
    StereoCapture.cpp     # Dual SceneCaptureComponent2D logic
    BFL_FileUtils.cpp

Content/
  Blueprints/
    BP_360VideoPlayer     # Media player actor (OpenFile + Play)
    BP_Controller         # Pawn: input, video switching, UI
    Stereo/
      BP_StereoCapture    # (legacy placeholder)
      WBP_StereoOutput    # Output widget — displays RT_Left / RT_Right
    UI/
      WBP_VideoLibrary    # Video selection UI
      WBP_VideoCard       # Per-video card with play button
  RT/
    RT_Left               # Left eye render target
    RT_Right              # Right eye render target
  Matrials/
    Left_Mat              # Left sphere material (EyeSelect=0.0)
    Right_Mat             # Right sphere material (EyeSelect=1.0)
  Mesh/
    invertedShpherighPoly # Inside-out sphere mesh for 360 viewing
```

## Setup

1. Open `StereroScopicProject.uproject` in **Unreal Engine 5.5**
2. Build C++ project in Visual Studio (Development Editor | Win64)
3. Open level `main`
4. Select `BP_Controller` in Outliner → Details → **Video Files** → add absolute path(s) to `.mp4` video files
   - Example: `C:/Videos/my360video.mp4`
5. Press Play

## Key Bindings

| Input | Action |
|-------|--------|
| Mouse X/Y | Rotate camera yaw/pitch |
| Space | Play / Pause toggle |
| C | Toggle Video / Scene mode |
| ToggleVideoUI | Show/hide video library |

## Engine Version

Unreal Engine **5.5**

> Note: Project filename is `StereroScopicProject` (legacy typo). Internal module is correctly named `StereoscopicProject`.
