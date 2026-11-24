# November 2026 L4D2 build
Offline evidence from uploaded Windows x86 DLLs (not bundled).

- client.dll: 4d01ed9fb856d57ce549b8587c386f361083ee581b001295fac0eff4c835d52d, timestamp 1794952363, SizeOfImage 0x963000.
- engine.dll: d451800729322f25bfc65d58a408edb4f4243a8ea8c26fabccbba4bc7ecc1db3, timestamp 1794946460, SizeOfImage 0x73f000.
- studiorender.dll: a7efe8304d0e0bb5cfa87193d234670ea59ead1323d43a10da60b2fbd6f65d14, timestamp 1794274100, SizeOfImage 0x453000.

VClient016 and VEngineClient013 camera slots retain their RVAs. Entity list slots moved +0x30, ModelInfo slots +0x50. TraceRay 0x1797a0 (clear 0x14ba30, ret 0x179e6b); checked Ray/result/filter anchors retain their offsets. Studio DrawModel[25] remains 0x6a150, engine DrawModelExecute[19] 0xe0600. FPB RTTI COL and local gate disassembled; updated gate call displacements and ClientMode slots. Native entity deformation BuildTransformations/RecvProxy is intentionally restricted to Studio-copy fallback until its new-build ABI is independently confirmed. Old client/engine builds are not selected by the new fixed profile. Static evidence only; runtime validation pending.
