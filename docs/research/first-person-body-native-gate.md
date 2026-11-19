# Native first-person body approach A: binary evidence

The exact uploaded PE32 game binaries match Visual's profiles:
client.dll SHA256 c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f (timestamp 1782845135, image 0x963000);
engine.dll SHA256 8a6d468059394bf4cfce32f11dcba12ef45de88da1373084d5a2916d0bf7ba86 (timestamp 1782845142, image 0x73f000).

MSVC RTTI proves C_TerrorPlayer's primary vtable at client+0x5b6494 and
IClientRenderable vtable at client+0x5b63dc for entity+4, with COL
client+0x69fc18. DrawModel slot [9] is client+0x258570 and the
instruction at +0x2586ed directly calls client+0x223360.

Disassembly of +0x223360 compares the actor with GetLocalPlayer(-1)
and branches on living state (client object+0x143 = entity+0x147).
On the native local-player path, at +0x2233f1 it checks a byte
at a client runtime object+0x25ec. The exact semantic name of this
field has NOT been proven. The conditional JZ at client+0x223401
displacement 0x13 skips the original base-class DrawModel call,
returning zero. Changing ONLY displacement byte +0x223402 to 0x00
makes the jump fall through to the SAME original DrawModel call.
This is native visibility restoration, not an additional model draw
or an overlay. All other native checks and scene depth remain original.

The plugin enables this one-byte patch only for the verified local,
living team-2 C_TerrorPlayer with valid handle/serial/map epoch,
model and fresh snapshot. It validates full client and engine disk hashes,
PE headers, RTTI identities, DrawModel vtable method, direct callsite,
native prologue, and 46-byte branch signature. It refuses overwritten
or unsupported code and restores byte 0x13 when disabled, dead,
out of game, invalid or unloading; never writes to the DLL file.
The prior read-only ShouldDraw probe is retained for comparison.

**No in-game tests were possible here.** Whether this restores visible
legs, correct animation, occlusion, third-party MOD compatibility, or
causes head/hair and third-person arm overlap remains unknown.
Possible camera and pass-specific artifacts are expected.
Do not publish as a stable release, or assume VAC safety.
