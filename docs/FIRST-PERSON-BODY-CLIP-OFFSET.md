# First-person body: adjustable height clipping and 3-axis body offset (experimental)

## Status
The user confirmed the preceding native DrawModel visibility-gate build rendered
the first-person body successfully. This revision adds two new, separate,
default-inactive effects. **Neither new effect has been tested in-game.**
Static unit tests and the exact x86 DLL build are separate from visual success.

## Height-based clipping, no head recognition
- User sets a numeric height above the original model origin (0..128 Source units).
- Only the **currently verified local living CTerrorPlayer** model may be affected.
- During the existing StudioRender DrawModel callback, save a previously-unused
  Direct3D9 clip plane and D3DRS_CLIPPLANEENABLE state, enable a plane that keeps
  vertices at or below (modelOrigin.z + render-only offset.z + clipHeight), then
  restore both plane and mask immediately upon callback completion.
- Shader pipelines need the plane in *homogeneous clip space*, not world space.
  Use transpose(inverse(camera.worldToClip)) to convert the world-height plane;
  fixed-function pipelines take the world-space plane.
- Shader changes **inside** the Studio callback or deferred render queues may
  ignore the scoped clip plane; this is unverified and the diagnostic reports
  only that the D3D state was accepted, not that pixels were clipped.
- Rendered geometry is not capped. Hands raised above the plane also disappear.
- If there is no free clip-plane slot, the D3D device lacks support, or the
  camera transform fails, clipping is skipped without modifying game state.

## Body-only three-axis position offset
- User adjusts local-forward, local-right, and world-Z offset in Source units.
- Uses the last validated Source camera to compute horizontal forward/right
  directions; looking up or down does not move the model diagonally.
- A copy of the current Studio bone matrices is translated, and a shifted
  Studio draw origin is passed to the *original* Studio draw callback once.
- Server entity, movement origin, networked coordinates, animation cache, and
  collision boxes remain unchanged. Weapon viewmodel is not selected.
- If bone metadata or camera validation fails, the original unmodified draw is
  used. Existing entity deformation is composed before this final body offset.
- Original render bounds and some shadows may not follow larger offsets.

## Menu
Main menu [3]: existing first-person body on/off. [9]: advanced settings.
The advanced page has: clipping submenu, position submenu, diagnostics, save.
Clip height increments: +/-1 and +/-5, default 65. Clip defaults OFF.
Position increments: +/-0.5, +/-1, +/-5, with a selectable step; all axes
default 0 and have independent limits of -64..64.
Settings stay in hud.json under firstPersonBody, alongside existing settings.
The menu does not reuse ID 820 (entity transforms). IDs 860, 862, 863, 864
navigate; 861 and 870..883 are action-only.

## Test matrix
- Native visibility patch: previously confirmed in-game by user.
- New clipping & position effects: source tests and compile only; NO real-game
  confirmation. D3D state acceptance != visual clipping.
- Target check: local entity base+4, full handle/serial/epoch, validated living
  survivor, native gate active, fresh camera.
- State restoration: scoped to one native StudioRender call, no duplicated draw.
- Existing model MODs, shadow/depth passes, crouch, jump, all animated skins,
  and third-party DXVK variants must still be checked by a human tester.
- Disable or revert this build if shader artifacts or crashes occur.
