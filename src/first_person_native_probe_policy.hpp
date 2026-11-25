#pragma once
#include <cstddef>
#include <cstdint>
namespace visual::firstperson::nativeprobe {
enum class GetterKind { Unknown, Immediate, AbsoluteSlot };
struct Getter { GetterKind kind=GetterKind::Unknown; std::uint32_t operand=0; };
inline std::uint32_t little32(const std::uint8_t* p) noexcept {
 return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
// Only recognize complete argument-independent cdecl getter bodies.
// Never execute game code or infer the incoming EDI argument.
inline Getter decode(const std::uint8_t* p,std::size_t size) noexcept {
 if(!p)return {};
 if(size>=6&&p[5]==0xc3){
  if(p[0]==0xb8)return {GetterKind::Immediate,little32(p+1)};
  if(p[0]==0xa1)return {GetterKind::AbsoluteSlot,little32(p+1)};
 }
 if(size>=7&&p[0]==0x8b&&p[1]==0x05&&p[6]==0xc3)
  return {GetterKind::AbsoluteSlot,little32(p+2)};
 return {};
}
inline bool fieldAddress(std::uint32_t object,std::uint32_t& out) noexcept {
 if(!object||object>0xffffffffu-0x25ecu)return false;
 out=object+0x25ecu;return true;
}
inline const char* label(GetterKind k) noexcept {
 switch(k){case GetterKind::Immediate:return "immediate";case GetterKind::AbsoluteSlot:return "absolute-slot";default:return "unresolved";}
}
}
