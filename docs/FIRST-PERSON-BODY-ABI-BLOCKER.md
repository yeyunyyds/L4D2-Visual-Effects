# First-person body: ABI-gated investigation

**Status: diagnostic-only, not a working body renderer.**
This experimental DLL is built separately from the v0.8.0 release. It does not
install a first-person visibility hook, call DrawModelExecute twice, alter bones,
write entity visibility fields or use the HUD stage to emulate a body.

## Verified observations

* The current Visual callbacks are engine model draw [19] and Studio Render [25].
  These callbacks are downstream of the renderable submission decision.
* Camera capture uses the verified VClient016 View_Render [25] interface.
* local_player.hpp obtains the true local player's handle using a validated
  engine DLL entry point; source_entities.hpp has handle+serial+epoch/model
  checks and survivor team/life-state/health fields.
* The existing game binary profile validates these known interfaces only.
  No specific C_TerrorPlayer::ShouldDraw / ShouldDrawLocalPlayer entry, slot,
  calling convention, or submission path has been proven for this game build.
  Source SDK 2013 is not an ABI specification for L4D2.
* No real L4D2 client.dll/engine.dll or running game is available for disassembly
  or in-game visual acceptance testing. The exact filter cause is not proven.

## To unlock approach A

1. Obtain exact client.dll corresponding to the current DLL SHA256 and PE
   fingerprints from a legitimate installed game.
2. Trace renderable selection for the local survivor. Determine whether first
   person suppression happens in ShouldDraw / ShouldDrawLocalPlayer, EF_NODRAW,
   or elsewhere in the render list. Record disassembly proof.
3. Independently validate object identity, x86 calling convention, vtable slot
   if applicable, hook ownership, chain policy and stop-time unhook semantics.
4. Override only the verified visibility decision for the verified local
   survivor, with observer, third-person, incapacitated, death and epoch gates.
   Do not create an extra draw or alter gameplay state.
5. Test true scene depth, head/arm clipping, movement, MOD skins, incapacitation,
   death, map transitions and all existing Visual modules inside L4D2.

This menu stores intent but deliberately NEVER declares rendering success.
