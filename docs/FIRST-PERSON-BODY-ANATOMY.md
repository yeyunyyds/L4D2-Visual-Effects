# First-person body rendering

## Scope

This project implements a local-player body display using the game's native
model rendering path. It adjusts render-only Studio bone matrices to reduce
head and arm geometry obscuring the first-person camera. It does not alter
gameplay collision, networking, player physics or the weapon viewmodel.

## Rendering behavior

1. Identify the verified local player and validate model metadata and the
   Studio bone hierarchy.
2. When a valid neck ancestor exists, collapse head descendants toward the
   neck using a temporary matrix copy.
3. Independently locate left and right upper-arm branches and, when enabled,
   collapse descendants toward their shoulder parents.
4. Apply optional camera-relative body positioning to the render-only copy.
5. Call the original Studio drawing continuation exactly once; if a model
   cannot be safely classified, leave its unknown bone branches unchanged.

No GPU height-clipping path is used.

## Settings

The feature is **off by default**. Once enabled, the menu offers local-body
display, independent head and arm hiding, dynamic positioning, camera yaw
following, manual XYZ adjustments, per-model calibration, diagnostics, and
saving settings.

Optional third-person view compatibility is also off by default. It only
governs whether this feature participates in the local player's drawing when
the view changes; it does not provide a camera or alter external plug-ins.

## Limitations

Visual results depend on model skeletons, vertex skinning and animations.
Accessories with separate bones and extreme camera or character motions can
still reveal geometry near the neck or shoulders. Camera/body alignment is
render-only and does not provide full inverse kinematics or independent
weapon interaction. The current user-tested view-switching behavior does not
guarantee compatibility with every model or game configuration.
