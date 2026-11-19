# Exact client.dll binary investigation (2026-11-19)

The supplied client.dll is 8,305,664 bytes. SHA256:
c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f.

PE machine: 0x14c (x86). Timestamp: 1782845135. Image size: 0x963000.
These exactly match the client version in the existing Visual profile.

RTTI identifies C_TerrorPlayer primary vtable at RVA 0x5b6494,
IClientRenderable secondary subobject at entity+4 with vtable RVA 0x5b63dc,
and CompleteObjectLocator RVA 0x69fc18. Secondary slot 3 points to RVA
0x253140 (x86 thiscall bool ShouldDraw(), no stack arguments).

Native disassembly of RVA 0x253140:
- If entity m_fEffects has bit 0x20, returns false.
- Primary vtable slot 240 reads the netprop at base+0x1c9a, identified
  directly from this client's own network-property registration as m_isGhost.
  When true, the code compares the entity to the local player and may
  suppress the draw. This is GHOST filtering, not verified alive-survivor
  first-person filtering.
- Otherwise delegates to RVA 0x223440. For normal living actors this
  compares GetTeamNumber() against 1; base+0x147 is m_lifeState.

The C_BasePlayer secondary ShouldDraw slot 3 points to RVA 0x66210.
It contains explicit local-player conditional filtering but C_TerrorPlayer's
normal branch does NOT delegate to that base implementation. A simple
force-true C_TerrorPlayer::ShouldDraw patch is therefore not proven useful.

The new runtime probe is READ-ONLY: it chains the original ShouldDraw
once and returns the exact original bool, counting local calls / true /
false. It requires exact disk hash, PE profile, function fingerprint, two
RTTI-derived vtable identities, local handle/serial/epoch, and live
survivor life/team/health checks. It unhooks on disabled/map/death states;
callbacks retain a pinned module if a later third-party hook chains us.

Still unknown: whether first-person player exclusion happens in the
engine's render-list submission before ShouldDraw. The exact engine.dll
and an in-game probe session are needed. This build does NOT restore
native first-person body visibility, has not been tested in-game, and
must NOT be treated as a completed release.
