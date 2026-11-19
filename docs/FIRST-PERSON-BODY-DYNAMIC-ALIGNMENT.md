# Experimental model-aware dynamic first-person body placement

## Intent, provenance and verification
The user has confirmed that L4D2Visual's native DrawModel visibility gate and
manual Studio pose translation work in their real game. The new dynamic position
path is **not yet game-tested**, and must never be described as an exact copy
of Evo's proprietary implementation.

The supplied "L4D2 Evolved.zip" v1.1.9 contains ONLY binaries and configuration:
- vr/config.sample lists FirstPersonBodyAnchorOffsetMeters, CameraOffsetMeters,
  FallbackCameraOffsetMeters, AnchorRotationOffsetDeg and optional head/arm filters.
- updatelog.txt v1.0.9 says calibrations are stored per MOD rather than per survivor.
- L4D2Evolved.dll diagnostic strings describe inverse-bind recovery, a verified
  model eye position with stale-eye rejection, a chest/neck/head fallback,
  headFromAnchor / chestFromAnchor, bodyYawRoot, and upper-body locking.

Our model-aware implementation is independently authored. No binary code is
copied and no undocumented Evo interfaces are called.

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
   direction, bounded to 0.90 rad maximum; disabled by default. Optional
   per-model yaw trim is bounded to ±40 degrees.
7. Copy all Studio matrices; apply the bounded transform to the temporary copy.
   Then run the existing Evo-inspired hide-head/hide-arms on that same copy,
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
- Toggle dynamic mode ON (default OFF, to preserve proven original behavior).
- Toggle follow camera yaw separately (experimental; default OFF).
- Adjust fallback eye offset (+/-1 Source unit) and global yaw trim (+/-5 degrees).
- Position submenu retains manual XYZ and step increments.
- Bind trims to current MOD, reset this MOD, then Save Settings to persist.
- Diagnostic submenu reports aligned/rejected counts, active live camera
  samples vs failures, active model and calibration source.

## Important limitations
- Evo uses its own VR/NoHMD HMD-centred body yaw, room-scale lean,
  head/IK and weapon interaction. **Those proprietary systems are NOT included.**
- Current engine matrices MAY still be one render stage behind if Source
  updates them after the Studio callback. The diagnostic measures successful
  reads, not proven temporal alignment.
- Model rig metadata may be invalid or absent. Unsupported MODs fall back
  safely; the same-mapped inverse-bind data should be checked in game.
- Body shadows, weapon world entity, clipping geometry, exotic crouch/ladder
  rigs, ragdolls and camera bobbing can differ. The broken legacy GPU clipping
  toggle remains OFF and is NOT part of dynamic alignment.
- Dynamic alignment intentionally avoids changing game physics and net state.
- Testing of jump/landing math, per-model persistence, eye fallbacks and
  model/epoch reset is automated, but full game/animation testing is required.
- Use only in offline testing initially; compatibility/VAC safety is not proven.
