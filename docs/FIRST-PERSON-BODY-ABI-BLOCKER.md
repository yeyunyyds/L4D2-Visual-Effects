# First-person body approach A experiment

Both client.dll and engine.dll have now been independently verified to match
the expected exact Windows x86 binary profiles. A native local-player-specific
DrawModel early-out has been identified in client.dll. This experimental build
attempts to restore original scene model drawing using one reversible byte in
the in-memory original function. It does not draw the model twice or replace
the game DLL. See docs/research/first-person-body-native-gate.md.

No L4D2 game runtime is available for testing. Functional success and model
appearance are unverified. Original head, hair and arms may clip into the
camera or weapon. All other native game checks remain unchanged. The
experiment defaults off and should not be used as a stable release.
