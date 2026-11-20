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

## TLTPV first-/third-person mode gate (opt-in, revised)

`firstPersonBody.tltpvCompatibility` remains **false by default**. The option
only adapts native `firstperson` / `thirdpersonshoulder` camera changes.

The initial experimental version repeatedly switched the native local-DrawModel
byte during movement because the horizontal camera distance was compared
against a network entity snapshot updated once every 200 ms. Studio draw and
the game-frame callback also both wrote a shared mode; top-down views with
small XY separation were incorrectly identified as first person.

The revised camera observer:

* Reads verified `c_thirdpersonshoulder` (advisory, never assumed universally
  synchronized with the current view).
* Directly re-reads the local player's verified `m_vecOrigin` recvprop during
  the camera game frame, instead of relying on the stale 200-ms inventory
  snapshot; never calls an unverified game virtual.
* Includes eye height (`camera.eye.z - localOrigin.z`) when looking for a
  first-person eye, not merely XY distance.
* Publishes view changes from the game frame only. Unknown samples briefly
  preserve the previous mode; confirmed Third immediately restores the native
  DrawModel gate; First requires at least 96 ms of consecutive evidence.
* Studio drawing reads the committed mode without reclassifying or updating it.
  The dynamic-alignment history resets only when the committed mode changes.

Diagnostics report the committed mode, most recently read shoulder CVar,
XY separation and eye height. `-1` indicates unavailable distance or CVar.
This remains an experimental heuristic, not direct access to the engine's
`CAM_IsThirdPerson()` implementation. Real camera mode, renderer interactions
and TLTPV game behavior require in-game validation.
