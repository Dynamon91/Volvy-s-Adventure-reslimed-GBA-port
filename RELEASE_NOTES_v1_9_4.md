# Volvy's Adventure: ReSlimed — GBA v1.9.4

## Update: safer BG tile streaming and fewer cache allocation scans

Continues **directly from v1.9.3**. Source-converted sprite art, level tiles,
room placements, gameplay physics, collision and Shrewy behavior are unchanged.

- **GBA stack safety**: `scene_stream_update` no longer allocates a 1,024-entry
  `NativeSceneUpload` array (4,096 bytes) on the ARM7TDMI call stack. The BG1
  scene cache writes its upload list directly into `s_scene_uploads`, the
  persistent buffer that VBlank already uses. The existing guard prevents
  writes while a previous upload batch is pending. The 4 KiB copy has also
  been removed.
- **Faster cache churn**: the 32x32 native 4bpp tile cache reuses its lowest
  free slot with a monotonically increasing cursor during each allocation
  pass, instead of rescanning all previously filled slots for every tile.
  Cache slots and upload order remain identical to v1.9.3.
- **New regression tests**: 540 deterministic scroll/jump windows compare the
  optimized slot allocation, uploads, tilemap flags and source-to-slot mapping
  with an independent first-fit reference. A source contract check ensures the
  staging buffer stays persistent and is not accidentally returned to stack.

### Verification (2026-11-22)

| Check | Result |
|---|---|
| Host C gameplay, asset, renderer and new cache churn tests | PASS |
| ARMv4T/Thumb codegen, test headers/stubs | PASS |
| Clang/LLD experimental GBA image and structural validation | PASS |
| ROM bytes | 2,097,152 (2 MiB) |
| mGBA gameplay and physical GBA | **NOT VERIFIED** |
| devkitARM/libgba | **NOT VERIFIED** |

The PNG preview is **source-derived only**, not a captured gameplay frame.
Estimated overall completion remains **44%**: the patch reduces risk and
rendering overhead but does not reconstruct additional content.
