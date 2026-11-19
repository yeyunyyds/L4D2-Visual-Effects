#pragma once
#include <cstddef>
#include <cstdint>
// Verified local-player-only JZ rel8 in the native C_TerrorPlayer draw path.
namespace visual::firstperson::branch {
constexpr std::uint32_t DrawRva=0x223360,EntryRva=0x258570,LocalGateRva=0x2233f1;
constexpr std::uint32_t DisplacementRva=0x223402,RenderableTableRva=0x5b63dc;
constexpr std::uint32_t PrimaryTableRva=0x5b6494,RenderableColRva=0x69fc18;
constexpr unsigned DrawSlot=9;
constexpr unsigned char Original=0x13,PassThrough=0x00;
inline constexpr unsigned char GateBytes[]={
0x57,0xe8,0x79,0xc2,0xde,0xff,0x83,0xc4,0x04,
0x80,0xb8,0xec,0x25,0x00,0x00,0x00,0x74,0x13,
0x8b,0x55,0x0c,0x52,0x53,0x8b,0xce,0xe8,
0xf1,0x0b,0xe4,0xff,0x5b,0x5f,0x5e,0x5d,
0xc2,0x08,0x00,0x5b,0x5f,0x33,0xc0,0x5e,0x5d,
0xc2,0x08,0x00};
constexpr std::size_t DisplacementOffset=DisplacementRva-LocalGateRva;
static_assert(DisplacementOffset==17&&GateBytes[DisplacementOffset]==Original);
inline bool matches(const unsigned char* data,std::size_t size,bool allowPatch) noexcept {
 if(!data||size!=sizeof(GateBytes))return false;
 for(std::size_t i=0;i<size;++i)
  if(i==DisplacementOffset){
   if(data[i]!=Original&&(!allowPatch||data[i]!=PassThrough))return false;
  }else if(data[i]!=GateBytes[i])return false;
 return true;
}
}
