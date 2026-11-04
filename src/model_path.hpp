#pragma once
#include "dango/source_entities.hpp"
#include "module_verification.hpp"
#include "render_override.hpp"
namespace visual::modelpath {
inline RenderOverride setting;inline std::atomic<int> current{-1};
inline bool value(void*& object,int& number){
 auto module=source_camera::clientModule;if(!module)return false;
 auto base=reinterpret_cast<std::uintptr_t>(module);object=reinterpret_cast<void*>(base+0x7a62d0);
 std::uint32_t parent{},table{},name{};
 auto& reader=source_entities::reader;
 auto addr=std::uint32_t(reinterpret_cast<std::uintptr_t>(object));
 if(!source_entities::wire::load(reader,addr+0x1c,parent)||parent!=addr ||
    !source_entities::wire::load(reader,addr,table)||table!=base+0x65af2c ||
    !source_entities::wire::load(reader,addr+12,name)||source_entities::wire::text(reader,name)!="cl_modelfastpath")return false;
 return source_entities::wire::load(reader,addr+0x30,number);
}
inline bool set(void* object,int number){
 auto table=*reinterpret_cast<void***>(object);
 auto expected=reinterpret_cast<std::uintptr_t>(source_camera::clientModule)+0x3d3d90;
 if(reinterpret_cast<std::uintptr_t>(table[14])!=expected || !source_camera::executable(table[14]))return false;
 // Statically verified native integer setter, preserving callbacks/string state.
 reinterpret_cast<void(__thiscall*)(void*,int)>(table[14])(object,number);return true;
}
inline void apply(bool enabled){
 if(enabled&&setting.owned)return;
 if(!enabled&&!setting.owned)return;
 void* object{};int number{};
 if(!value(object,number)){current=-1;return;}
 current=number;
 if(enabled&&!setting.owned && !verifiedModuleFile(source_camera::clientModule,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f"))return;
 static bool reportedFailure=false;
 bool wasOwned=setting.owned;
 if(!setting.update(enabled,number,[&](int v){if(!set(object,v))return false;current=v;return true;})){
  if(!reportedFailure)source_camera::log("Render compatibility native setter unavailable");reportedFailure=true;return;
 }
 reportedFailure=false;
 if(wasOwned!=setting.owned)source_camera::log("Render compatibility cl_modelfastpath="+std::to_string(current)+" saved="+std::to_string(setting.original));
}
}
