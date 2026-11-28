#pragma once
#include "module_verification.hpp"
#include "dango/source_entities.hpp"
namespace visual::localplayer {
// Query the actual local player on the game thread; camera/spectated actors and
// Steam/P2P roster discovery are deliberately not used for range warnings.
inline int index(){
 using namespace source_camera;constexpr int invalid=-1;
 if(!hooked||!engine||!engineModule||!owns(engine,engineModule))return invalid;
 static HMODULE checked=nullptr;static bool accepted=false;if(checked!=engineModule){checked=engineModule;accepted=verifiedModuleFile(engineModule,"d451800729322f25bfc65d58a408edb4f4243a8ea8c26fabccbba4bc7ecc1db3");}if(!accepted)return invalid;
 constexpr unsigned char prefix[]={0x6a,0xff,0xe8,0x39,0x5c,0x02,0x00,0x8b,0x80,0xd8,0x00,0x00};
 auto fn=reinterpret_cast<const unsigned char*>(engineModule)+0x5d030;
 if(!readable(fn,sizeof(prefix))||!executable(fn)||std::memcmp(fn,prefix,sizeof(prefix)))return invalid;
 using Fn=int(__thiscall*)(void*);int index=reinterpret_cast<Fn>(const_cast<unsigned char*>(fn))(engine);
 return index>=1&&index<=64?index:invalid;
}
inline std::uint32_t handle(const dango::inventory::Frame& frame){
 constexpr std::uint32_t invalid=0xffffffffu;
 const int index=localplayer::index();
 if(index<1||!source_entities::list)return invalid;
 for(const auto& r:frame.records)if(r.index==unsigned(index)){
  auto entries=std::uint32_t(std::int64_t(reinterpret_cast<uintptr_t>(source_entities::list))+source_entities::profile::EntryBaseDelta);source_entities::wire::Entry e;
  if(source_entities::wire::load(source_entities::reader,entries+r.index*16,e)&&e.entity&&e.serial==r.serial&&r.handle==((e.serial<<12)|r.index))return r.handle;
 }return invalid;
}
}
