#pragma once
#include "tank_cvar_contract.hpp"
#include "tank_model.hpp"
#include "dango/source_entities.hpp"
namespace visual::tank {
inline std::shared_ptr<const Parameters> parameters=std::make_shared<const Parameters>();
struct CvarProbe{const char* name;float low,high;std::optional<CvarValue> binding;};
inline std::vector<CvarProbe> probes={{"tank_swing_range",1,512,{}},{"tank_swing_arc",1,360,{}},{"tank_windup_time",0,10,{}},{"tank_swing_duration",0,10,{}},{"tank_swing_interval",0,10,{}},{"tank_swing_miss_interval",0,10,{}}};
inline bool validVtable(std::uint32_t object){std::uint32_t vt=0,code=0;return dango::entitywire::load(source_entities::reader,object,vt)&&dango::entitywire::load(source_entities::reader,vt,code)&&source_camera::executable(reinterpret_cast<void*>(uintptr_t(code)));}
inline void scanModule(HMODULE module){
 if(!module)return;auto base=uint32_t(reinterpret_cast<uintptr_t>(module));auto& reader=source_entities::reader;IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS nt{};
 if(!dango::entitywire::load(reader,base,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>1024*1024||!dango::entitywire::load(reader,base+uint32_t(dos.e_lfanew),nt)||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt.FileHeader.NumberOfSections>32||nt.OptionalHeader.SizeOfImage>32*1024*1024)return;
 struct Block{uint32_t address;std::vector<unsigned char> bytes;};std::vector<Block> blocks;std::size_t used=0;
 auto sections=base+uint32_t(dos.e_lfanew)+4+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
 for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i){IMAGE_SECTION_HEADER section{};if(!dango::entitywire::load(reader,sections+i*sizeof(section),section))return;
  auto size=section.Misc.VirtualSize;if(!(section.Characteristics&IMAGE_SCN_MEM_READ)||(section.Characteristics&IMAGE_SCN_MEM_EXECUTE)||!size||used+size>24u*1024u*1024u||uint64_t(section.VirtualAddress)+size>nt.OptionalHeader.SizeOfImage)continue;
  Block b{base+section.VirtualAddress,std::vector<unsigned char>(size)};bool readable=true;
  for(std::size_t offset=0;offset<size;offset+=65536){auto chunk=std::min(std::size_t(65536),std::size_t(size)-offset);if(!reader.read(b.address+uint32_t(offset),b.bytes.data()+offset,chunk)){readable=false;break;}}
  if(readable){used+=size;blocks.push_back(std::move(b));}
 }
 for(auto& probe:probes){if(probe.binding)continue;std::set<uint32_t> names;std::size_t length=std::strlen(probe.name)+1;
  for(const auto& b:blocks)for(auto i=std::search(b.bytes.begin(),b.bytes.end(),probe.name,probe.name+length);i!=b.bytes.end();i=std::search(i+1,b.bytes.end(),probe.name,probe.name+length))names.insert(b.address+uint32_t(i-b.bytes.begin()));
  std::map<uint32_t,CvarValue> candidates;
  for(const auto& b:blocks)for(std::size_t i=0;i+4<=b.bytes.size();i+=4){uint32_t ptr;std::memcpy(&ptr,b.bytes.data()+i,4);if(!names.count(ptr)||i<12)continue;auto address=b.address+uint32_t(i)-12;
   auto value=readCvar(reader,address,probe.name,probe.low,probe.high);if(value&&validVtable(value->object)&&validVtable(value->parent))candidates[value->parent]=*value;
  }
  if(candidates.size()==1)probe.binding=candidates.begin()->second;
 }
}
inline void pollCvars(std::uint64_t now,bool enabled){
 static std::uint64_t next=0,rescan=0;static HMODULE priorServer=nullptr;static std::string scope;
 if(!enabled)return;auto server=GetModuleHandleW(L"server.dll");auto desired=server?"in-process server-module ConVar (read-only)":"replicated-client ConVar (read-only)";
 if(server!=priorServer||scope!=desired){for(auto& p:probes)p.binding.reset();priorServer=server;scope=desired;next=rescan=0;}
 if(now<next)return;next=now+500;
 if(now>=rescan){rescan=now+10000;if(server)scanModule(server);else scanModule(source_camera::clientModule);}
 Parameters out;out.tick=now;out.source=desired;
 std::optional<float>* outputs[]={&out.range,&out.arc,&out.windup,&out.duration,&out.interval,&out.missInterval};
 for(unsigned i=0;i<probes.size();++i){auto& p=probes[i];if(!p.binding)continue;auto v=readCvar(source_entities::reader,p.binding->object,p.name,p.low,p.high);if(v)*outputs[i]=v->value;else p.binding.reset();}
 std::atomic_store(&parameters,std::make_shared<const Parameters>(std::move(out)));
}
}
