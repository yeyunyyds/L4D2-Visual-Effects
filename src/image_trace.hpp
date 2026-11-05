#pragma once
#include "module_verification.hpp"
#include "dango/source_trace_contract.hpp"
namespace visual::picturetrace {
inline void* object=nullptr;inline bool faulted=false;
inline void reset(){object=nullptr;faulted=false;}
inline bool connect(){
 using namespace source_camera;auto module=engineModule;
 if(!hooked||!module||!matchesImage(module,EngineTimestamp,EngineImageSize,"picture trace engine"))return false;
 if(!verifiedModuleFile(module,"8a6d468059394bf4cfce32f11dcba12ef45de88da1373084d5a2916d0bf7ba86"))return false;
 using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface"));if(!factory)return false;
 auto candidate=factory("EngineTraceClient003",nullptr);auto base=reinterpret_cast<std::uintptr_t>(module);
 if(candidate!=reinterpret_cast<void*>(base+0x42fa5c)||!readable(candidate,4))return false;
 auto table=*reinterpret_cast<void***>(candidate);if(table!=reinterpret_cast<void**>(base+0x387474)||!readable(table,32))return false;
 constexpr std::uint32_t methods[]={0x177630,0x177600,0x178950,0x177990,0x179ff0,0x179760,0x177d60,0x177da0};
 for(unsigned i=0;i<8;++i)if(table[i]!=reinterpret_cast<void*>(base+methods[i])||!executable(table[i]))return false;
 const unsigned char entry[]={0x53,0x8b,0xdc,0x83,0xec,0x08,0x83,0xe4,0xf0},clear[]={0x55,0x8b,0xec,0x56,0x8b,0x75,0x08,0x6a,0x54},tail[]={0xc2,0x10,0};
 if(!readable(table[5],sizeof(entry))||std::memcmp(table[5],entry,sizeof(entry))||!readable(reinterpret_cast<void*>(base+0x14ba10),sizeof(clear))||std::memcmp(reinterpret_cast<void*>(base+0x14ba10),clear,sizeof(clear))||!readable(reinterpret_cast<void*>(base+0x179e2b),sizeof(tail))||std::memcmp(reinterpret_cast<void*>(base+0x179e2b),tail,sizeof(tail)))return false;
 object=candidate;return true;
}
inline bool __fastcall reject(void*,void*,void*,int){return false;}
inline int __fastcall worldOnly(void*,void*){return 1;}
inline void* methods[]={reinterpret_cast<void*>(&reject),reinterpret_cast<void*>(&worldOnly)};
struct Filter {void** table=methods;};
inline bool cast(dango::Vec3 start,dango::Vec3 end,dango::trace::Result& output){
 if(faulted||!object||!dango::wall::finite(start)||!dango::wall::finite(end))return false;
 auto ray=dango::trace::ray(start,end);dango::trace::GuardedResult guarded;Filter filter;
 using Call=void(__thiscall*)(void*,const dango::trace::Ray&,unsigned,void*,dango::trace::Result*);
 auto table=*reinterpret_cast<void***>(object);reinterpret_cast<Call>(table[5])(object,ray,0x400b,&filter,&guarded.result);
 if(!guarded.intact()||!dango::trace::valid(guarded.result,start,end)){faulted=true;return false;}
 output=guarded.result;return true;
}
} // namespace visual::picturetrace
