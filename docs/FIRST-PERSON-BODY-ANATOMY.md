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

Optional third-person view compatibility is also off by default. In this
experimental preview, verified native DrawModel visibility is now view-scoped:
with compatibility ON, the gate is open only after the main view commits
first-person; on third-person or unresolved view states it restores the original
DrawModel byte. A positive native shoulder view closes the gate even with
compatibility OFF. Visual-owned FreeCamera world-body mode remains explicitly
allowed and keeps the real character in its world pose. The
policy combines the original skeletal eye estimate, live entity-origin-relative
camera height and horizontal separation, stationary-origin motion evidence,
and explicit third-person evidence. On return, the head can be hidden alone
before the full-body resume delay. FreeCamera bypasses these cosmetic changes.
This does not provide a camera or alter external plug-ins.

A missing same-draw camera reading may retain the last *positively verified*
first-person cosmetics for at most 48ms when the local identity/model is the
same, the published game-camera frame remains recent, and no third-person
shoulder/mode evidence exists. A dynamic body alignment can only reuse its last
verified translation/yaw inside that same grace window. Any explicit third-person
state, stale source, model change, or prolonged sample failure falls back to the
previous native rendering behavior; the native DrawModel gate itself is unchanged.
See [v1.1.1 release notes](releases/v1.1.1.md) for limitations.

## Limitations

Visual results depend on model skeletons, vertex skinning and animations.
Accessories with separate bones and extreme camera or character motions can
still reveal geometry near the neck or shoulders. Camera/body alignment is
render-only and does not provide full inverse kinematics or independent
weapon interaction. The current user-tested view-switching behavior does not
guarantee compatibility with every model or game configuration.

### Experimental secondary-camera pass isolation

This preview uses a recent validated centered first-person camera (within
8 Source units horizontally from the verified local entity origin, eye height
30..88) as a short-lived reference. If another Studio draw for the same
entity/model, within 100ms, samples a camera at least 14 units off the entity
origin and at least 12 units from the reference eye, FPB leaves that draw's
**original bones and origin untouched**. It does not suppress the draw or change
the native DrawModel gate, and does not rewrite the original camera. This avoids
feeding suspected secondary-pass samples back into the main-view view-policy
hysteresis and dynamic alignment. The heuristic requires a committed first
person view, shoulder CVar 0, and positive first-person sample evidence; it
does not classify any engine draw flags as shadow passes.

Debug logs emit throttled `FPB_SECONDARY_BYPASS` events and the FPB diagnostics
page shows the cumulative bypass count. This is an experimental isolation fix,
not proof that extra colored geometry or shadow appearance is resolved. Verify
with original and MODded survivors, crouch/landing, FreeCamera, native third
person and model transitions before merging.

### Preview: native visibility ownership and Studio shadow depth

The previously unconditional native patch could expose local draws even when
the game's own third-person visibility would suffice. This preview restores
the original native JZ byte when a compatible normal camera is not positively
confirmed first person; explicit shoulder third person also restores it.
This is a game-frame transition, not a Studio-draw-time code patch. Restore
failures retain the previous unknown/faulted handling.

The Studio decorator now receives the original Studio flags. A Studio pass
carrying bit `0x200` (called `STUDIORENDER_SHADOWDEPTHTEXTURE` in the public
Source SDK, not yet independently proven for this L4D2 build) uses the original
bones/position while still executing the original rendering continuation.
This is *not* shadow removal and does not discard any color/model draw.

Debug emits `FPB_SHADOW_NATIVE` (rate-limited) if that bit is observed;
the diagnostics menu contains a count. If the flags are absent, this bypass
does not activate. The fix remains experimental: a custom third-person camera
that does not set the native shoulder CVar may still be classified as first
person, and a color pass with no identifiable flags will still render.
