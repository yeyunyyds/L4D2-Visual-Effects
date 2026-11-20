# Model-aware dynamic first-person body placement

## Default configuration

The first-person body feature is **off by default**. Enabling it uses the
user-tested placement values: dynamic anchor ON, camera yaw following ON,
head/arm hiding ON, manual forward=-13, right=-1 and up=-1.
Existing `hud.json` settings remain authoritative when explicitly saved.

These position values are visual adjustments, not exact anatomical eye
alignment. Different models and animations can require separate calibration.

## Purpose

Dynamic positioning maintains a stable relation between the local player's
rendered body and the live game camera. It combines verified model metadata,
per-model calibration, render-time camera measurements, and a temporary
Studio bone-matrix copy without changing gameplay state.

## Runtime algorithm
1. Keep current strict native visibility gate, alive-local-survivor identity,
   entity handle/serial, verified DLL identities, model epoch and renderable+4
   checks. Other players and viewmodel arms bypass dynamic alignment.
2. During this Studio DrawModel call, read the current engine camera matrices
   from the already-verified VEngineClient013, not the frame snapshot published
   after View_Render. Fail closed if source matrices or camera pose are invalid.
3. Read verified studiohdr_t MDL eye position and mstudiobone_t poseToBone
   inverse bind matrices, inverted to recover model-local rest bone positions.
   Require model bind eye to be close to head and above upper-chest. Otherwise
   fallback to inverse-bind head or neck + adjustable eye offset.
4. Choose the upper-spine/chest ancestor of the neck as the anchor. Head/neck
   animated wobble must not become the body root.
5. Compare the camera eye with the predicted eye reconstructed from the current
   animated upper-body anchor and model-specific neutral offset.
   Compute a render-only translation to align the anchor. Vertical correction
   is deliberately immediate to handle jump/landing; lateral correction uses
   a short 18ms filter. Reset on model, handle, epoch, elapsed-time or teleport.
6. Optional yaw alignment from left/right clavicles to horizontal camera
   direction, bounded to 0.90 rad maximum; enabled in the tuned preset. Optional
   per-model yaw trim is bounded to ±40 degrees.
7. Copy all Studio matrices; apply the bounded transform to the temporary copy.
   Then apply the separately configured head/arm branch hiding on that copy,
   and call the native Studio original exactly once.
8. If there is no valid inverse-bind pose, bootstrap a live model-relative
   anchor at the first valid sample. Invalid camera, anchor, matrices, or
   >96-unit corrections bypass dynamic placement, preserving original drawing
   (and any existing manual offset).
9. Preserve 3-axis manual XYZ offsets as post-calibration fine-tuning.
   "Save current XYZ and yaw to current MOD" moves the current global trims
   into that MOD's profile and clears the global trims. The model key combines
   canonical MDL name and Studio header checksum. Profiles persist in hud.json.

## Controls and diagnostics
- Main menu -> First-person Body (existing single submenu) -> Dynamic Anchor
- Dynamic mode defaults ON for this user-tested camera-framing preset.
- Camera-yaw following defaults ON when the feature is enabled.
- Adjust fallback eye offset (+/-1 Source unit) and global yaw trim (+/-5 degrees).
- Position submenu retains manual XYZ and step increments.
- Bind trims to current MOD, reset this MOD, then Save Settings to persist.
- Diagnostic submenu reports aligned/rejected counts, active live camera
  samples vs failures, active model and calibration source.

## Important limitations

- Engine camera matrices can update at different render stages. A successful
  camera read does not by itself prove exact temporal alignment.
- Rig metadata may be invalid or unavailable. Unsupported model layouts fall
  back without applying unverified bone transformations.
- Body shadows, clothing, weapon world models, crouch/ladder poses, ragdolls,
  camera bobbing and other geometry may differ.
- This feature does not modify game physics or network state.
- Automated tests cover alignment math, calibration persistence, eye-position
  fallbacks and reset handling. Broader game configuration coverage is limited.
