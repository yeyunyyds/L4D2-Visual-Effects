# L4D2 FreeCamera prototype (experimental)

Based on experimental/module-host-first-person-body commit 73c6dc8aefc7a0fedaed8c1826ab27aada1f2bd7.

Implemented together:
1. Native IClientMode OverrideView[19] replacement of camera origin/angles.
2. WASD, mouse, Space/Ctrl, Shift camera movement with independent world coordinates.
3. IClientMode CreateMove[27] input suppression for movement, attacks, and player aim.
Additional: F6, HUD toggle/status/emergency off/speed, map-reset and first-person-body gating.

WARNING: NOT GAME-VALIDATED. Hook indexes 19/27 and view offsets 64/76
are hypotheses from publicly available Source/L4D2 code, not yet independently
confirmed in the current Windows game binary. ClientMode discovery uses the
historical code pattern 89 04 B5 [array pointer] E8. Require exact PE and SHA256
match and unique signature; do not bypass the refusal checks. View / command
plausibility checks abort on a likely incompatible ABI.

Tests: compile tests/free_camera_core_test.cpp with C++17, run scripts/test.sh,
then build Windows x86 via scripts/build-mingw.sh. The branch workflow builds
an install ZIP artifact. Test only offline and back up existing DLL/config.
No guarantee of VAC safety, third-party MOD compatibility or PVS behavior.

Manual smoke test: menu must report hook ready; press F6, check View/Move
counters rising, move and rotate, verify player remains still/server-side aim
unchanged, F6 restore, reload map, test TLTPV and first-person body. If the
hook is unavailable, inspect the log and validate the exact game DLL rather
than forcing a vtable index.

## Why the first prototype was completely inactive

- Main menu ID 930 was missing from the NativeHud opensChild allowlist, so the menu entry never navigated.
- ClientMode was reached through only one pointer read, while L4D2's g_pClientMode signature points to a slot array (two pointer reads).
- Camera setup offsets matched generic SDK2013, not the alternative L4D2 CViewSetup layout: fov 0x34, origin 0x3c, angles 0x48. We now validate both and refuse ambiguous cases.
- Added in-menu counts for toggle, F6 polling and F6 edges, and a camera layout label to separate UI errors from hooking/ABI errors.

The exact 2026 target binary must still confirm IClientMode slots 19/27 in practice. No successful in-game test has been performed.
