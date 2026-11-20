# L4D2 FreeCamera prototype (experimental)

Based on experimental/module-host-first-person-body commit 73c6dc8aefc7a0fedaed8c1826ab27aada1f2bd7.

Implemented together:
1. Native IClientMode OverrideView[19] replacement of camera origin/angles.
2. WASD, mouse, Space (ascend) and Shift (descend) camera movement with independent world coordinates.
3. IClientMode CreateMove[27] input suppression for movement, attacks, and player aim.
Additional: F6, HUD toggle/status/emergency off/speed, map-reset and first-person-body gating.

WARNING: NOT GAME-VALIDATED. Hook indexes 19/27 are hypotheses from
publicly available L4D2 code; the view-offset candidates are L4D2 (+60/+72)
and SDK2013 (+64/+76), selected only after plausibility checks. The current
Windows game binary has NOT been independently disassembled to certify the
ClientMode ABI. Discovery uses the historical code pattern
89 04 B5 [array pointer] E8 and a two-level pointer read. Require exact PE and SHA256
match and unique signature; do not bypass the refusal checks. View / command
plausibility checks abort on a likely incompatible ABI.

Tests: compile tests/free_camera_core_test.cpp with C++17, run scripts/test.sh,
then build Windows x86 via scripts/build-mingw.sh. The branch workflow builds
an install ZIP artifact. Test only offline and back up existing DLL/config.
No guarantee of VAC safety, third-party MOD compatibility or PVS behavior.

Manual smoke test: menu must report hook ready; press F6, check View/Move
counters rising, move and rotate, verify player remains still/server-side aim
unchanged, F6 restore, reload map, test third-person view compatibility and first-person body. If the
hook is unavailable, inspect the log and validate the exact game DLL rather
than forcing a vtable index.

## Why the first prototype was completely inactive

- Main menu ID 930 was missing from the NativeHud opensChild allowlist, so the menu entry never navigated.
- A subsequent attempted fix incorrectly used TWO pointer reads for g_pClientMode. Disassembly of this exact client.dll confirms its global array directly holds each object's pointer; a single read is correct.
- Camera setup offsets matched generic SDK2013, not the alternative L4D2 CViewSetup layout: fov 0x34, origin 0x3c, angles 0x48. We now validate both and refuse ambiguous cases.
- Added in-menu counts for toggle, F6 polling and F6 edges, and a camera layout label to separate UI errors from hooking/ABI errors.

The exact 2026 target binary must still confirm IClientMode slots 19/27 in practice. No successful in-game test has been performed.

## Exact binary correction after user screenshot

The older conclusion that ClientMode needed an extra pointer dereference was
**incorrect**. The prior uploaded Windows x86 client.dll is 8,305,664 bytes;
SHA256 c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f
matches the repository's required build exactly. Disassembly shows:

- client+0x22854e writes EAX directly into the two-entry ClientMode array
  at client+0x7b1b8c plus 4*esi. Read the object pointer ONCE.
- client+0x26edc0 calculates object address as client+0x7c3240 + 0x148*index.
- constructor client+0x273ddd sets vtable at client+0x5bd4fc.
- vtable entry 19 = client+0x26ff40, entry 27 = client+0xc2d70.
- L4D2 view origin is at CViewSetup+0x3c, angles at +0x48.

The locator uses exact build RVAs and checks registration opcode, object
identity and original vtable slots before installing the experimental hooks.
It no longer relies on a large code-section scan or a wrong extra pointer read.
The camera/menu integration still requires an offline game test.


## Source-only input adjustment (no build requested)

- FreeCamera input now uses WASD for planar flight, Space to rise, Shift to descend and mouse to aim. Ctrl is no longer assigned. The previous Shift 3x-speed modifier is removed; flight speed remains adjustable in the FreeCamera menu.
- The native HUD uses number/navigation keys and is configured not to capture the mouse. Opening the FreeCamera panel no longer pauses camera movement or mouse-look; pressing Esc and loss of game focus still pause it. F6 toggles even while the HUD is open.
- Source's native right basis at yaw=0 is -Y (not +Y). The former +Y strafe direction has been corrected. Horizontal mouse delta now uses negative yaw for rightward mouse movement, matching Source conventions.
- Updated the standalone camera math tests to assert D/right and mouse/right signs. Per request, **no build, CI or runtime test** was started for this change.


## Progressive flight acceleration (source-only change)

The old prototype used a constant speed of 260 Source units/second. The new
camera uses a world-space velocity vector with a responsive start and physical
acceleration, steering and deceleration. Mouse-look remains direct.

- Speed menu value is now **maximum flying speed**, default 1000 units/sec,
  adjustable by +/-50 in the FreeCamera menu (minimum 30, maximum 2500).
- On a new movement key, an initial 220 units/sec (capped at the chosen maximum)
  avoids sluggish short taps. Velocity accelerates to the ceiling in about
  0.35-0.45 seconds of continuous movement, regardless of tick rate.
- Releasing all movement keys brakes to zero in approximately 0.13 seconds;
  sharp reversals/steering also use stronger acceleration so switching
  directions feels responsive.
- Direction input is normalized (diagonal is not faster). Velocity uses
  trapezoidal integration with frame delta clamped at 0.05 seconds.
- Pausing via Esc or focus loss clears accumulated velocity so resuming
  cannot unexpectedly fling the camera; opening the HUD still permits flight.
- Tests now cover start/hold/cap/release/reverse/diagonal/vertical/mouse,
  time step bounds, restart resets and tick-rate consistency.

The new velocity calculation is **Source-like**, not a claim that the game
uses the same physics implementation. This commit intentionally skips CI and
does not build the DLL, per the user's previous request.


## Render-paced FreeCamera and smooth velocity (source-only)

- CreateMove samples WASD/Space/Shift, accumulates mouse deltas, and suppresses
  native player commands; it no longer advances the camera.
- The outer View_Render increments a real-frame marker before OverrideView.
  Each real render consumes input and integrates exactly once. Repeated
  OverrideView callbacks in that frame reuse the same pose for GF camera data.
- QPC render delta and continuous exponential velocity use tau=0.12 seconds
  for acceleration, 0.065 for release, 0.085 for sharp direction changes.
  Exact analytic position integration avoids adding separate positional lag.
- Frames more than 200ms apart, pauses, focus loss, map reset and toggling
  cancel residual movement. Delta is capped at 50ms.
- Real FPS and synthetic GF frames are separate; generated frames never
  execute this movement code. End-to-end GF compatibility still needs testing.
- New diagnostics show render steps, repeated callbacks, gaps and delta.
- Cross-thread input and render state is guarded by one short-lived mutex.
- Math tests cover motion continuity, boundaries, frame id deduplication,
  mouse direction, release, reversal, and tick-rate invariance.
- No build or CI run requested for this code change.

## Single-layer GitHub Actions installation download

The workflow keeps generating `build/L4D2Visual-0.8.0.zip` internally for
packaging validation and direct release distribution. Because GitHub Actions
artifact downloads are ZIP files already, the workflow now extracts that
installer into `build/free-camera-artifact/` before upload. As a result,
the downloaded `L4D2Visual-FreeCamera-x86.zip` contains `bin/` and
`README.md` immediately, not a second nested `.zip` file. The workflow
checks the exact file set before upload and includes no separate license files.
Previous workflow artifacts are unaffected; the change applies to subsequent
builds. No build is triggered by this documentation/workflow commit.


## FreeCamera + first-person body: world-anchored rendering (Scheme A)

When F6 enables FreeCamera while the first-person body option remains enabled,
the native local-player DrawModel path stays eligible. This is the **existing
CTerrorPlayer entity**, not a spawned body, dummy, or server-side object.

- The local-player handle, serial, alive/team/model checks and client.dll
  fingerprints are still required. On failure, the original draw suppression
  is restored; nothing is forcibly drawn from invalid entity data.
- While FreeCamera is requested, the player-to-camera distance must NOT be
  interpreted as a third-person view. The existing camera-distance latch is
  reset on both F6 transitions; normal third-person behavior resumes after F6.
- If the explicitly verified native shoulder-thirdperson ConVar is enabled
  with compatibility on, native third-person rendering retains ownership.
- In the FreeCamera Studio callback, forward the ORIGINAL matrices and
  original draw origin. No camera-follow dynamic body anchor, manual offset,
  yaw alignment, or head/arm changes are applied. No extra DrawModel call.
- The game engine still owns entity state, animation, draw order, visibility,
  culling and lifetime. This feature does not write any server entity state.
- Diagnostics expose "FreeCamera world-anchored body". This is an experimental
  visual-only integration, not a fix for a suspected server.dll crash.
- Unit tests cover normal vs detached policy, the native shoulder case,
  bone pass-through, and both F6 transitions. No binary build/CI was requested.


## F6 first-person viewmodel hide: ClientMode predicate (default ON)

The initial entity-inventory/DrawModelExecute filter did not hide the
first-person hands and weapon in an in-game test. It has been removed.

The replacement uses **ClientMode::ShouldDrawViewModel[30]** as a narrow
visibility decision, leaving the game's first-/third-person camera mode
unchanged. This is not a third-person toggle: it only mimics the model
visibility result while FreeCamera is requested and hideViewmodel=true.

The exact supported client.dll has SHA256
c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f.
Its known ClientMode vtable at client+0x5bd4fc contains:
- OverrideView[19] at client+0x26ff40
- CreateMove[27] at client+0x0c2d70
- ShouldDrawViewModel[30] at client+0x2286b0

Disassembly shows a no-argument, boolean-return function at slot 30.
The runtime validates the on-disk DLL SHA256, PE profile and method
machine-code prefix before replacing the vtable slot. Existing executable
hooks on that slot are chained: the original visibility decision is called
exactly once, then masked to false only when F6+hideViewmodel are both on.
If binding fails, the camera continues but native ViewModel display is
retained; this is visible in the menu diagnostics.

The menu flag remains default ON, persistent under
\`freeCamera.hideViewmodel\` in hud.json. Counters show
ViewModel checks/native-visible/hidden. It does not use r_drawviewmodel,
sv_cheats, server.dll, the viewmodel entity list, or DrawModelExecute
suppression, and it never changes the world-anchored FPB model.

Unit tests cover native-result passthrough on F6 off or option off
and forced false only when both are on. In-game efficacy and compatibility
with other ClientMode patches remain to be tested.
