#pragma once
// Studio flags are not the player's m_fFlags. Source SDK Studio interface
// calls bit 0x200 STUDIORENDER_SHADOWDEPTHTEXTURE. Preserve original bone
// matrices for these draws; NEVER suppress the draw itself.
// Confirm this bit on the actual target L4D2 build during preview testing.
namespace visual::firstperson::studiopass {
constexpr unsigned ShadowDepthBit=0x200u;
inline bool preserveOriginal(int flags) noexcept {
 return (static_cast<unsigned>(flags)&ShadowDepthBit)!=0u;
}
} // namespace visual::firstperson::studiopass
