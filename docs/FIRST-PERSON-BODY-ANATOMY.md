# Evo-style first-person body presentation

## Design goal and authorship

This experimental feature aims to reproduce the **visual characteristics of an
Evo-style first-person body view**: showing the local player’s body while
reducing obstructive head, arm, and animation geometry near the camera.
The description refers to a **visual style**, not a shared implementation.

All first-person body handling described here is independently implemented
within L4D2 Visual Effects. **No Evo source code, binary code, or proprietary
assets are copied, extracted, ported, or incorporated.** Naming the visual
reference does not imply affiliation, endorsement, or feature-level parity.

## Local model and bone handling

The plugin retains the game's native model drawing path and processes only
validated, local-player Studio render data. Its display-only operations are:

1. Inspect validated Studio model metadata and its bone parent hierarchy.
2. Identify the head bone and its descendants when a verifiable neck
   ancestor is available. Collapse that branch toward the neck on a temporary
   per-draw matrix copy, leaving the original pose unchanged.
3. Independently identify left and right upper-arm branches and, if enabled,
   collapse their descendants toward the corresponding shoulder parent.
4. Apply optional camera-relative body positioning to the same render-only
   matrix copy, then call the original Studio drawing function once.
5. Leave unrecognized bone structures unchanged rather than guessing which
   vertices belong to the head or arms.

These transformations do not change the player entity, original animation
buffers, game collision, network state, or first-person weapon viewmodel.

## User controls

The unified First-person Body submenu contains the body visibility switch,
head/arm hiding controls, dynamic positioning and per-model calibration,
manual XYZ adjustments, legacy GPU height clipping, diagnostics, and saving.
Head and arm hiding are independent options. The legacy clipping experiment
is separate from bone-based hiding and remains disabled in the current
experimental visual preset.

## Limitations and validation

The appearance depends on each MOD's skeleton, skin weighting, and animation
poses. Accessories may use independent bones, or geometry near the neck may
remain visible during jumps, crouches, or other extreme poses. This feature
does not claim to reproduce external VR/IK, material-management, or weapon
interaction systems.

The original native-body display and manual positioning were confirmed in
game by the user. Head/arm hiding and dynamic alignment have passed isolated
source tests and builds, but broader animation and MOD compatibility have
not been verified. These are experimental capabilities, not a stable release.
