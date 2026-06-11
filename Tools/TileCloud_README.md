# Tiling a large point cloud for the UE tile streamer

The UE `ATileStreamingManager` streams a big cloud that has been split into
spatial tiles **by an external tool — no UE install required on this machine.**

## Option 1 — LAStools (simplest)
```
lastile -i big.laz -tile_size 50 -buffer 0 -odir tiles -o tile.laz
```
- `-tile_size 50` = 50 (source units, usually metres) square tiles. Tune to get
  tiles of roughly 50-200 MB each. Smaller tiles = smoother streaming, more files.
- Output: `tiles/tile_*.laz`, each a valid LAS/LAZ with its own header bbox.

## Option 2 — PDAL (open source, free)
```
pdal split --length 50 -i big.laz -o tiles/tile#.laz
```
(or a pipeline with `filters.splitter` `length=50`).

## Then, in the UE build
1. Place an `ATileStreamingManager` actor in the level (or spawn one).
2. Set `TileDirectory` to the folder of tiles (or call
   `InitializeFromDirectory(path)` from the loader UI after browsing).
3. Set `ImportScale` to match LidarPointCloud import (default **100** = m -> cm).
4. Tune `LoadRadius` / `UnloadRadius` / `MaxResidentTiles` for RAM vs view distance.

The manager loads the nearest tiles (async), each as a self-LODing
LidarPointCloudComponent, and evicts far tiles → RAM bounded, renders in nDisplay
SBS (stereo) the same as any point cloud.

## Notes / current prototype limits
- Tile bounds are read straight from the LAS/LAZ header (works on .laz).
- `ImportScale` MUST match the engine import scale or tiles select/align wrong.
- Axis assumption: source X/Y/Z map directly to UE X/Y/Z (no flip). If your data
  imports rotated, the bounds-based selection still works (sphere-ish), but verify.
- Tile pop-in on fast movement is expected; widen LoadRadius / raise budget.
