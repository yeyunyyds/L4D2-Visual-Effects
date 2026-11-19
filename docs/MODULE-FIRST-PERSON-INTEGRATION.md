# Module-host first-person integration: build and manual validation

This experimental branch is based on `refactor/module-boundaries-remove-sprays`.
It does not contain the image, spray, or Steam P2P modules.

## Contracts

- `feature_composition.hpp` is the sole registration point for feature adapters.
- `module_host.hpp` exposes the optional device-created and Studio-decoration callbacks.
- `source_render.hpp` calls the registered Studio decorator only after existing
  entity pose modifications, and the decorator must call its continuation exactly
  once. Without a decorator (or during quiesce), the original call is retained.
- `first_person_body_module.hpp` owns menu, configuration, on-frame validation,
  native-visibility hook cleanup, device reference, and render decoration.
- All local-player checks, model identity gates, inverse-bind calibration, and
  experimental GPU clipping behavior retain the prior experimental implementation.
- `logging.level` defaults to `info`. Only the periodic multi-record snapshot
  is Debug-gated; normal event and failure logs remain enabled.
- `邦友的酒` is a nested menu item inside entity scaling, not a runtime dependency.

## Verification

Automated: module-host dispatch, stop/quiesce, Studio continuation, visual
preset and log migration, first-person body alignment math, menu routing, and
existing algorithm tests; GitHub Actions Windows x86 builds and archive checks.

Not verified by automation: actual L4D2 animation/physics/render behavior,
camera/Studio callback timing under load, exotic third-party skeletons,
game online/anti-cheat compatibility. Back up the original DLL and hud.json.
Test in offline environments before wider use.

## TLTPV first-/third-person mode gate (opt-in)

`firstPersonBody.tltpvCompatibility` defaults to `false`, including old
configurations. It does not parse the complete TLTPV CFG state machine.
The module reads the SHA-256-verified client.dll ConVar object at RVA
`0x7589c8` for `c_thirdpersonshoulder` (read-only name/parent checks),
then compares the current camera eye to the local networked player origin.

Third and Unknown modes restore the native DrawModel gate on the next game
frame and skip the Studio draw decoration; a second camera probe occurs at
draw time. Changing camera modes invalidates the dynamic alignment tracking
state. When compatibility is disabled, the original body rendering path remains.

Test TLTPV's ordinary `firstperson` and `thirdpersonshoulder` transitions
in game. Shoulder camera collision and network interpolation can produce an
Unknown result. This is an experimental spatial heuristic, not a verified
engine camera-mode accessor.
