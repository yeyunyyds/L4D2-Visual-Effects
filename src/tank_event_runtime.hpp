#pragma once
#include "tank_event_contract.hpp"
#include "dango/source_entities.hpp"
#include "module_verification.hpp"
#include "diagnostics.hpp"
#include <mutex>
namespace visual::tank::events {
using Factory=void*(__cdecl*)();using Proxy=void(__cdecl*)(const void*,void*,void*);
inline std::atomic<Factory> originalFactory{nullptr};inline std::array<std::atomic<Proxy>,3> originalProxies{};
inline std::shared_ptr<const Layout> binding=std::make_shared<const Layout>();
inline std::atomic<bool> active{false};inline std::mutex mutex;inline Recorder recorder;
inline uint32_t factoryThread=0,boundEpoch=0;inline bool installed=false,failed=false;
inline unsigned patchedMask=0;inline bool factoryPatched=false;
inline std::string status="disabled: requires Tank prediction AND diagnostic logging";
inline std::atomic<uint64_t> factories{0},invalid{0};inline std::array<std::atomic<uint64_t>,3> receives{};
inline std::deque<nlohmann::json> history;inline nlohmann::json bindingDetails;inline bool bindingDirty=false;
inline bool exchange(uint32_t address,void* expected,void* replacement){
 auto slot=reinterpret_cast<void**>(uintptr_t(address));DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
 auto previous=InterlockedCompareExchangePointer(slot,replacement,expected);DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);return previous==expected;
}
inline void __cdecl receive(unsigned k,const void* data,void* object,void* output){
 // Preserve the engine's exact callback and inputs, including rejected samples.
 auto original=originalProxies[k].load(std::memory_order_acquire);if(original)original(data,object,output);
 if(!active.load(std::memory_order_acquire))return;
 try{auto b=std::atomic_load(&binding);auto frame=std::atomic_load(&source_camera::snapshot);ProxyData d;int32_t decoded=0;
  if(!frame->active||!wire::load(source_entities::reader,uint32_t(uintptr_t(data)),d)||d.property!=b->properties[k]||d.type!=0||(d.element!=0&&d.element!=-1)||uint32_t(uintptr_t(object))!=b->base||uint64_t(b->base)+b->offsets[k]!=uintptr_t(output)||!wire::load(source_entities::reader,uint32_t(uintptr_t(output)),decoded)){++invalid;return;}
  std::lock_guard<std::mutex> lock(mutex);if(!active||b!=std::atomic_load(&binding))return;
  auto thread=GetCurrentThreadId();if(factoryThread&&factoryThread!=thread){recorder.flush();factoryThread=0;}
  ++receives[k];recorder.field(k,d.value,decoded,frame->epoch,GetTickCount64(),b->base);
 }catch(...){++invalid;}
}
inline void __cdecl playerProxy(const void* d,void* o,void* out){receive(0,d,o,out);}
inline void __cdecl eventProxy(const void* d,void* o,void* out){receive(1,d,o,out);}
inline void __cdecl dataProxy(const void* d,void* o,void* out){receive(2,d,o,out);}
inline void* __cdecl factory(){
 auto original=originalFactory.load(std::memory_order_acquire);void* result=original?original():nullptr;
 if(!active.load(std::memory_order_acquire))return result;
 try{auto b=std::atomic_load(&binding);auto frame=std::atomic_load(&source_camera::snapshot);if(!frame->active||uintptr_t(result)!=b->singleton){++invalid;return result;}
  std::lock_guard<std::mutex> lock(mutex);if(active&&b==std::atomic_load(&binding)){++factories;factoryThread=GetCurrentThreadId();recorder.begin(frame->epoch,GetTickCount64(),b->base);}
 }catch(...){++invalid;}return result;
}
inline std::array<Proxy,3> replacements(){return {playerProxy,eventProxy,dataProxy};}
// Called on the existing game-frame thread; original callbacks remain available
// after restoration so an in-flight callback can still chain normally.
inline void stop(){
 active=false;std::lock_guard<std::mutex> lock(mutex);recorder.reset();factoryThread=0;if(!installed)return;
 auto b=std::atomic_load(&binding);auto hooks=replacements();
 if(factoryPatched&&exchange(b->descriptor+offsetof(wire::ClientClass,event),reinterpret_cast<void*>(factory),reinterpret_cast<void*>(uintptr_t(b->factory))))factoryPatched=false;
 for(unsigned k=0;k<3;++k)if((patchedMask&(1u<<k))&&exchange(b->properties[k]+offsetof(wire::Prop,proxy),reinterpret_cast<void*>(hooks[k]),reinterpret_cast<void*>(uintptr_t(b->proxies[k]))))patchedMask&=~(1u<<k);
 installed=factoryPatched||patchedMask;failed=failed||installed;status=installed?"callback ownership changed; restore incomplete, exit game before replacing DLL":"disabled; event callbacks restored";
}
inline nlohmann::json codeDetails(uint32_t address){
 nlohmann::json j;auto module=source_camera::clientModule;auto p=reinterpret_cast<void*>(uintptr_t(address));if(!source_camera::owns(p,module)||!source_camera::executable(p))return j;
 j["clientRva"]=address-uint32_t(uintptr_t(module));std::array<unsigned char,192> bytes{};if(source_entities::reader.read(address,bytes.data(),bytes.size())){std::string hex;const char* digits="0123456789abcdef";for(auto b:bytes){hex+=digits[b>>4];hex+=digits[b&15];}j["first192Bytes"]=hex;}return j;
}
inline void update(bool requested,const dango::GameCameraFrame& frame){
 if(!requested||!frame.active){stop();return;}
 if(installed&&boundEpoch!=frame.epoch)stop();
 {std::lock_guard<std::mutex> lock(mutex);recorder.flush();if(installed||failed)return;}
 if(!source_entities::bound)return;
 uint32_t descriptor=0;for(auto address:source_entities::classCatalog.descriptors){wire::ClientClass c;if(wire::load(source_entities::reader,address,c)&&wire::text(source_entities::reader,c.name)=="CTEPlayerAnimEvent"){descriptor=address;break;}}
 if(!descriptor)return;
 // Existing verified client build plus decoded getter identity, actual table
 // names/types/offsets, module ownership and executable proxy targets.
 auto module=source_camera::clientModule;
 if(!visual::verifiedModuleFile(module,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")){std::lock_guard<std::mutex> lock(mutex);status="client file identity rejected; no event hooks installed";failed=true;return;}
 auto owned=[&](uint32_t a){return source_camera::owns(reinterpret_cast<void*>(uintptr_t(a)),module);};
 auto code=[&](uint32_t a){return owned(a)&&source_camera::executable(reinterpret_cast<void*>(uintptr_t(a)));};
 auto inspected=inspect(source_entities::reader,descriptor,owned,code);
 std::lock_guard<std::mutex> lock(mutex);status=inspected.reason;
 bindingDetails={{"schema",1},{"epoch",frame.epoch},{"reason",inspected.reason},{"factory",codeDetails(inspected.factory)},{"virtualMethodsCalled",false}};
 if(inspected.singleton){uint32_t vt=0;if(wire::load(source_entities::reader,inspected.singleton,vt)){bindingDetails["singletonMethods"]=nlohmann::json::array();for(unsigned slot=0;slot<=10;++slot){uint32_t fn=0;if(wire::load(source_entities::reader,vt+slot*4,fn)){auto detail=codeDetails(fn);detail["slot"]=slot;bindingDetails["singletonMethods"].push_back(detail);}}}}
 bindingDirty=true;
 if(!inspected.valid){failed=true;return;}
 auto b=std::make_shared<const Layout>(inspected);std::atomic_store(&binding,b);originalFactory=reinterpret_cast<Factory>(uintptr_t(b->factory));
 for(unsigned k=0;k<3;++k)originalProxies[k]=reinterpret_cast<Proxy>(uintptr_t(b->proxies[k]));
 auto hooks=replacements();unsigned patched=0;for(;patched<3;++patched){if(!exchange(b->properties[patched]+offsetof(wire::Prop,proxy),reinterpret_cast<void*>(uintptr_t(b->proxies[patched])),reinterpret_cast<void*>(hooks[patched])))break;patchedMask|=1u<<patched;}
 bool ok=patched==3&&exchange(b->descriptor+offsetof(wire::ClientClass,event),reinterpret_cast<void*>(uintptr_t(b->factory)),reinterpret_cast<void*>(factory));
 if(!ok){for(unsigned k=0;k<patched;++k)if(exchange(b->properties[k]+offsetof(wire::Prop,proxy),reinterpret_cast<void*>(hooks[k]),reinterpret_cast<void*>(uintptr_t(b->proxies[k]))))patchedMask&=~(1u<<k);installed=patchedMask!=0;failed=true;status="event pointer installation rejected; capture disabled";return;}
 factoryPatched=true;
 installed=true;boundEpoch=frame.epoch;active=true;status="event factory + three receive proxies observed; original callbacks preserved";
}
inline nlohmann::json summary(){std::lock_guard<std::mutex> lock(mutex);return {{"installed",installed},{"active",active.load()},{"reason",status},{"factoryCalls",factories.load()},{"playerReceives",receives[0].load()},{"eventReceives",receives[1].load()},{"dataReceives",receives[2].load()},{"invalidSamples",invalid.load()},{"queueDropped",recorder.dropped()},{"recentEvents",history}};}
inline void poll(){
 nlohmann::json details;{std::lock_guard<std::mutex> lock(mutex);if(bindingDirty){details=bindingDetails;bindingDirty=false;}}
 if(!details.is_null()){std::ofstream info(visual::diagnostics::path.parent_path()/L"tank-animation-binding.json",std::ios::binary);info<<details.dump(2);}
 std::vector<Event> list;{std::lock_guard<std::mutex> lock(mutex);list=recorder.take();}if(list.empty())return;
 auto actors=std::atomic_load(&source_entities::combatSnapshot);auto path=visual::diagnostics::path.parent_path()/L"tank-animation-events.jsonl";
 std::error_code error;auto bytes=std::filesystem::file_size(path,error);if(!error&&bytes>=4u*1024u*1024u){auto old=path;old.replace_filename(L"tank-animation-events.previous.jsonl");if(!MoveFileExW(path.c_str(),old.c_str(),MOVEFILE_REPLACE_EXISTING)){std::lock_guard<std::mutex> lock(mutex);status="event log rotation failed";return;}}
 std::ofstream file(path,std::ios::app|std::ios::binary);
 for(const auto& e:list){nlohmann::json j={{"schema",1},{"epoch",e.epoch},{"id",e.id},{"tick",e.tick},{"mask",e.mask},{"factoryBoundary",e.factory},{"complete",e.complete},{"boundary",e.boundary},{"raw",e.raw},{"decoded",e.decoded},{"source","CTEPlayerAnimEvent receive proxies"}};
  if(e.mask&1)j["playerHandle"]=uint32_t(e.decoded[0]);if(e.mask&2){j["event"]=e.decoded[1];if(e.decoded[1]==33||e.decoded[1]==34)j["candidate"]="public HAND_ATTACK/HAND_LOW_ATTACK mapping; not yet verified";}if(e.mask&4)j["data"]=e.decoded[2];
  // Attribute only a complete tuple and an exact full handle in a recent frame.
  if(e.complete&&actors->epoch==e.epoch&&(e.tick>=actors->tick?e.tick-actors->tick:actors->tick-e.tick)<=250)for(const auto& a:actors->records)if(a.handle==uint32_t(e.decoded[0])){j["actorClass"]=a.className;if(a.zombieClass)j["zombieClass"]=*a.zombieClass;if(a.sequence)j["actorSequence"]=*a.sequence;if(a.simulationTime)j["actorSimulationTime"]=*a.simulationTime;break;}
  {std::lock_guard<std::mutex> lock(mutex);history.push_back(j);while(history.size()>12)history.pop_front();}file<<j.dump()<<'\n';
 }
 if(!file){std::lock_guard<std::mutex> lock(mutex);status="event log write failed";}
}
}
