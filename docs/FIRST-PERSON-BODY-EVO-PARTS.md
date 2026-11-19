# Evo-style first-person body branch hiding (independent implementation)

Based on the **exact uploaded L4D2 Evolved v1.1.9 ZIP**, not Evo source code:
- vr/config.sample includes FirstPersonBodyHideHead and FirstPersonBodyHideArms.
- L4D2Evolved.dll help text describes head-and-descendant collapse toward the
  neck, and first-person world-arm removal below the clavicles while preserving
  a source pose for viewmodel IK. No disassembled function was copied.

L4D2Visual uses an independently written, smaller version of that idea:
1. Retain actual lowercased MDL bone names and parents in StudioCache metadata.
2. Find a head bone with a verified neck ancestor. Find left/right upper-arm
   roots by name. Traverse validated parent-child links to collect descendants.
3. On the already-verified *local, alive survivor* Studio draw only, read the
   current render bone matrices into a private temporary array. Translate that
   array for the existing manual XYZ offset (if any).
4. For selected bones, replace the 3x3 transform with a tiny-scale transform
   and set the translation to their neck or shoulder-parent world position.
5. Pass the temporary array to the existing original Studio draw exactly once.
   Neither real skeleton, network state, entity origin nor viewmodel is edited.

New settings inside the original "firstPersonBody" hud.json namespace:
hideHead=true (when body rendering is enabled), hideArms=false. Toggle each
independently from ONE second-level First-person body menu. Existing XYZ
and legacy GPU clip experiments remain accessible, labeled separately.

Important limits: this does not clone Evo's VR IK, anchor calibration, material
manager, or third-person world-weapon filtering. Some MOD meshes bind several
body parts to a shared bone, or contain head accessories outside the head
branch. Such models may retain artifacts. Unsupported head/neck layouts fail
closed and leave the source pose intact.

The previous native visibility and manual XYZ features were user-tested;
**head and arm collapse in this build has NOT been tested in L4D2**.
Tests cover typical ValveBiped branches, unchanged torso/neck, no head fallback,
malformed skeleton and non-finite matrices. In-game validation of head hair,
weapons, crouch, map changes, and third-party MOD is still required.
