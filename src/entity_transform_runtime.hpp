#pragma once
#include "entity_transform.hpp"
#include "dango/source_entities.hpp"
#include "studio_cache.hpp"
#include "module_verification.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
namespace visual::deform {
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::atomic<bool> live{false},stopRequested{false},clean{true};
inline std::atomic<unsigned> boneLimit{128};inline std::atomic<DWORD> owner{0};
inline std::atomic<unsigned long> builds{0},fields{0},studioDraws{0},failures{0},restores{0};
inline std::atomic<const char*> status{"关闭；未安装实体构建或网络代理"};
using BuildFn=void(__thiscall*)(void*,void*,void*,void*,const float*,int,void*);
using ProxyFn=void(__cdecl*)(const void*,void*,void*);
struct Hook {void** slot=nullptr;void* original=nullptr;void* replacement=nullptr;std::atomic<bool> installed{false};};
inline Hook buildHooks[128],proxyHooks[128];inline std::atomic<unsigned> buildCount{0},proxyCount{0};
struct State {std::mutex mutex;std::atomic<unsigned> route{0};std::atomic<const char*> reason{u8"等待骨骼构建或Studio绘制"};bool fieldOwned=false;std::uint32_t fieldOffset=0;float saved=1,last=1;std::uint32_t matrix=0;std::vector<Matrix3x4> before,after;std::vector<unsigned char> changed;};
struct Target {std::uint32_t epoch=0,handle=0,base=0,index=0,model=0,modelOffset=0,scaleOffset=0;std::uint64_t tick=0;Rule rule;bool field=false,view=false,build=false;void** buildSlot=nullptr;void** proxySlot=nullptr;std::string label;std::shared_ptr<State> state=std::make_shared<State>();};
struct Targets {std::uint32_t epoch=0;std::uint64_t tick=0;std::unordered_map<std::uint32_t,std::shared_ptr<Target>> bases;};
inline auto emptyTargets=std::make_shared<const Targets>();
inline std::shared_ptr<const Targets> targets=emptyTargets;
inline std::vector<std::shared_ptr<Target>> pending;
inline bool current(const Target& t,bool checkModel=true){using namespace source_entities;if(std::atomic_load(&source_camera::snapshot)->epoch!=t.epoch)return false;if(!list)return false;auto object=std::uint32_t(reinterpret_cast<uintptr_t>(list));auto address=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta+t.index*sizeof(wire::Entry));wire::Entry e;if(!wire::load(reader,address,e)||e.entity!=t.base||e.serial!=(t.handle>>12))return false;std::int16_t model=0;return !checkModel||!t.modelOffset||(wire::load(reader,t.base+t.modelOffset,model)&&unsigned(model)==t.model);}
inline bool write(std::uint32_t p,const void* data,size_t n){SIZE_T done=0;return p&&n<=256*48&&WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(uintptr_t(p)),data,n,&done)&&done==n;}
inline bool swap(Hook& h,bool attach){DWORD old=0,ignored=0;if(!h.slot||GetModuleHandleW(L"client.dll")!=source_camera::clientModule||!source_camera::owns(h.slot,source_camera::clientModule)||!VirtualProtect(h.slot,4,PAGE_READWRITE,&old))return false;void* expected=attach?h.original:h.replacement;void* to=attach?h.replacement:h.original;auto actual=InterlockedCompareExchangePointer(h.slot,to,expected);bool restored=VirtualProtect(h.slot,4,old,&ignored)!=FALSE;if(actual==expected)h.installed=attach;return actual==expected&&restored;}
inline bool install(Hook* hooks,std::atomic<unsigned>& count,void** slot,void* replacement){
 unsigned n=count.load(std::memory_order_acquire);for(unsigned i=0;i<n;++i)if(hooks[i].slot==slot){auto& h=hooks[i];if(h.installed)return true;void* currentValue=nullptr;if(!source_entities::reader.read(std::uint32_t(uintptr_t(slot)),&currentValue,4)||currentValue!=h.original)return false;return swap(h,true);}
 if(n>=128)return false;void* original=nullptr;if(!source_entities::reader.read(std::uint32_t(uintptr_t(slot)),&original,4)||!source_camera::executable(original)||original==replacement)return false;
 // Retain code through process lifetime if a foreign hook later chains us.
 HMODULE pinned=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(replacement),&pinned))return false;
 auto& h=hooks[n];h.slot=slot;h.original=original;h.replacement=replacement;count.store(n+1,std::memory_order_release);return swap(h,true);
}
inline void detach(Hook* hooks,unsigned count){for(unsigned i=0;i<count;++i)if(hooks[i].installed&&!swap(hooks[i],false))++failures;}

inline std::shared_ptr<Target> target(void* self){if(!live)return {};auto s=std::atomic_load(&targets);auto now=GetTickCount64();if(now<s->tick||now-s->tick>300)return {};auto i=s->bases.find(std::uint32_t(uintptr_t(self)));if(i==s->bases.end()||!current(*i->second))return {};return i->second;}
inline void restorePose(Target& t){
 auto& s=*t.state;
 if(s.matrix&&current(t)){
  std::uint32_t pointer=0;std::array<Matrix3x4,256> now;
  if(source_entities::wire::load(source_entities::reader,t.base+0x6c0,pointer)&&pointer==s.matrix&&s.after.size()<=256&&source_entities::reader.read(pointer,now.data(),s.after.size()*48)){
   bool changed=false;for(size_t i=0;i<s.after.size();++i)if(s.changed[i]&&!std::memcmp(&now[i],&s.after[i],48)){now[i]=s.before[i];changed=true;}
   if(changed){if(write(pointer,now.data(),s.after.size()*48))++restores;else ++failures;}
  }
 }
 s.matrix=0;s.before.clear();s.after.clear();s.changed.clear();s.route=0;
}

inline void restoreField(Target& t){auto& s=*t.state;if(!s.fieldOwned)return;if(current(t,false)){float value=0;if(source_entities::wire::load(source_entities::reader,t.base+s.fieldOffset,value)&&value==s.last){if(write(t.base+s.fieldOffset,&s.saved,4))++restores;else ++failures;}}s.fieldOwned=false;}
inline bool setField(Target& t,float value){if(!t.scaleOffset||!current(t))return false;auto& s=*t.state;float now=0;if(!source_entities::wire::load(source_entities::reader,t.base+t.scaleOffset,now)||!std::isfinite(now))return false;if(!s.fieldOwned||now!=s.last)s.saved=now;if(now!=value&&!write(t.base+t.scaleOffset,&value,4))return false;s.fieldOwned=true;s.fieldOffset=t.scaleOffset;s.last=value;++fields;return true;}
__declspec(noinline) inline void buildWork(BuildFn original,void* self,void* header,void* positions,void* rotations,const float* root,int mask,void* computed){
 if(!original){++failures;return;}auto t=GetCurrentThreadId()==owner?target(self):nullptr;if(t&&!t->build)t.reset();
 std::array<unsigned char,256> previousMask{};unsigned previousCount=0;
 if(t){std::lock_guard<std::mutex> l(t->state->mutex);previousCount=unsigned(t->state->changed.size());if(previousCount<=256)std::copy(t->state->changed.begin(),t->state->changed.end(),previousMask.begin());restorePose(*t);if(t->scaleOffset&&!setField(*t,1.f)){++failures;t.reset();}}
 original(self,header,positions,rotations,root,mask,computed);if(!t||t->field)return;
 try {
 using source_entities::wire::load;auto& reader=source_entities::reader;std::uint32_t raw=0,matrices=0,flags=0,magic=0;int version=0,count=0;
 auto h=std::uint32_t(uintptr_t(header));if(!load(reader,h,raw)||!load(reader,raw,magic)||magic!=0x54534449||!load(reader,raw+4,version)||(version!=48&&version!=49)||!load(reader,raw+156,count)||count<=0||unsigned(count)>boneLimit||!load(reader,h+44,flags)||!flags||!load(reader,t->base+0x6c0,matrices)||!matrices){t->state->reason=u8"构建头或骨骼布局不匹配";++failures;return;}
 std::array<std::uint32_t,5> flagVector{};if(!reader.read(h+44,flagVector.data(),20)||!boneFlagVector(flagVector,unsigned(count))){t->state->reason=u8"构建标记向量不匹配；使用Studio补充";++failures;return;}
 std::array<Matrix3x4,256> before,after;std::array<unsigned,256> boneFlags;Matrix3x4 rootMatrix;
 if(!reader.read(matrices,before.data(),size_t(count)*48)||!reader.read(flags,boneFlags.data(),size_t(count)*4)){++failures;return;}
 for(int i=0;i<count;++i)boneFlags[i]=((boneFlags[i]&unsigned(mask))||(previousCount==unsigned(count)&&previousMask[i]))?1u:0u;
 float pivot[3]{};if(t->view){if(!centroid(before.data(),count,pivot,boneFlags.data(),1))return;}else if(!reader.read(std::uint32_t(uintptr_t(root)),&rootMatrix,48)){++failures;return;}else for(unsigned r=0;r<3;++r)pivot[r]=rootMatrix.m[r][3];
 if(!transform(before.data(),after.data(),count,pivot,t->rule,boneFlags.data(),1)){++failures;return;}
 std::lock_guard<std::mutex> l(t->state->mutex);auto& s=*t->state;s.before.assign(before.begin(),before.begin()+count);s.after.assign(after.begin(),after.begin()+count);s.changed.resize(count);s.matrix=matrices;
 for(int i=0;i<count;++i)s.changed[i]=boneFlags[i]!=0;bool ok=write(matrices,after.data(),size_t(count)*48);if(!ok){restorePose(*t);++failures;return;}t->state->reason=u8"骨骼构建路径";t->state->route=1;++builds;
 }catch(...){++failures;status="骨骼缓存分配失败；保留原生构建";}
}
// x86 Source CRecvProxyData: RecvProp pointer, 16-byte variant, element, objectId.
struct ProxyData {std::uint32_t prop;unsigned char value[16];int element,objectId;};static_assert(sizeof(ProxyData)==28);
inline void proxyWork(ProxyFn original,const void* data,void* self,void* out){
 if(!original){++failures;return;}original(data,self,out);
 if(GetCurrentThreadId()!=owner)return;ProxyData d{};if(!data||!source_entities::reader.read(std::uint32_t(uintptr_t(data)),&d,sizeof(d))){++failures;return;}
 auto t=target(self);if(!t||(!t->field&&!t->build)||d.objectId!=int(t->index)||out!=reinterpret_cast<void*>(uintptr_t(t->base+t->scaleOffset)))return;
 std::lock_guard<std::mutex> l(t->state->mutex);float native=0;if(!source_entities::reader.read(std::uint32_t(uintptr_t(out)),&native,4)||!std::isfinite(native))return;
 t->state->saved=native;float want=t->field?factors(t->rule)[0]:1.f;
 if(write(std::uint32_t(uintptr_t(out)),&want,4)){t->state->fieldOwned=true;t->state->fieldOffset=t->scaleOffset;t->state->last=want;}
}
// Each installed slot has a stable thunk and its own exact upstream. Chained
// calls remain correct even if another plugin replaces an entity's vtable.
template<size_t I> void __fastcall buildThunk(void* self,void*,void* header,void* positions,void* rotations,const float* root,int mask,void* computed){buildWork(reinterpret_cast<BuildFn>(buildHooks[I].original),self,header,positions,rotations,root,mask,computed);}
template<size_t I> void __cdecl proxyThunk(const void* data,void* self,void* out){proxyWork(reinterpret_cast<ProxyFn>(proxyHooks[I].original),data,self,out);}
using BuildHookFn=void(__fastcall*)(void*,void*,void*,void*,void*,const float*,int,void*);
template<size_t... I> constexpr auto buildTable(std::index_sequence<I...>){return std::array<BuildHookFn,sizeof...(I)>{&buildThunk<I>...};}
template<size_t... I> constexpr auto proxyTable(std::index_sequence<I...>){return std::array<ProxyFn,sizeof...(I)>{&proxyThunk<I>...};}
inline constexpr auto buildThunks=buildTable(std::make_index_sequence<128>{});
inline constexpr auto proxyThunks=proxyTable(std::make_index_sequence<128>{});
inline bool installBuild(void** slot){unsigned n=buildCount.load();for(unsigned i=0;i<n;++i)if(buildHooks[i].slot==slot)return install(buildHooks,buildCount,slot,reinterpret_cast<void*>(buildThunks[i]));return n<128&&install(buildHooks,buildCount,slot,reinterpret_cast<void*>(buildThunks[n]));}
inline bool installProxy(void** slot){unsigned n=proxyCount.load();for(unsigned i=0;i<n;++i)if(proxyHooks[i].slot==slot)return install(proxyHooks,proxyCount,slot,reinterpret_cast<void*>(proxyThunks[i]));return n<128&&install(proxyHooks,proxyCount,slot,reinterpret_cast<void*>(proxyThunks[n]));}

template<class Reader> std::uint32_t scaleProp(Reader& reader,std::uint32_t root){using namespace dango::entitywire;std::set<std::uint32_t> seen;for(unsigned depth=0;root&&depth<32;++depth){if(!seen.insert(root).second)return 0;Table table;if(!load(reader,root,table)||table.count<0||table.count>4096)return 0;std::uint32_t parent=0;for(int i=0;i<table.count;++i){Prop p;auto at=table.props+unsigned(i)*sizeof(Prop);if(!load(reader,at,p))return 0;auto name=text(reader,p.name);if(name=="m_flModelScale"&&p.type==1)return at;if(name=="baseclass"&&p.type==6&&p.offset==0)parent=p.table;}root=parent;}return 0;}
inline void cleanup(){live=false;auto old=std::atomic_load(&targets);std::atomic_store(&targets,emptyTargets);if(GetModuleHandleW(L"client.dll")==source_camera::clientModule)for(const auto& i:old->bases){auto& t=*i.second;std::lock_guard<std::mutex> l(t.state->mutex);restorePose(t);restoreField(t);}detach(buildHooks,buildCount.load());detach(proxyHooks,proxyCount.load());pending.clear();clean=true;}
inline void onFrameWork(const dango::GameCameraFrame& camera,unsigned limit){owner=GetCurrentThreadId();boneLimit=limit;auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::snapshot);
 if(stopRequested||!any(*cfg)||!camera.active||camera.epoch!=frame->epoch||camera.tick<frame->tick||camera.tick-frame->tick>250){if(!clean)cleanup();status="关闭或没有有效目标；构建和代理已卸载";return;}
 if(!source_camera::clientModule||!verifiedModuleFile(source_camera::clientModule,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")){status="客户端版本未验证；不安装原生形变";return;}
 static std::shared_ptr<const Settings> previousCfg;static std::shared_ptr<const source_entities::Frame> previousFrame;
 if(live&&previousCfg==cfg&&previousFrame==frame)return;pending.clear();pending.reserve(frame->records.size());previousCfg=cfg;previousFrame=frame;
 static std::uint32_t propEpoch=0;static std::unordered_map<std::uint32_t,std::uint32_t> props;if(propEpoch!=frame->epoch){props.clear();propEpoch=frame->epoch;}
 auto old=std::atomic_load(&targets);auto next=std::make_shared<Targets>();next->epoch=frame->epoch;next->tick=frame->tick;unsigned eligible=0;
 auto object=std::uint32_t(uintptr_t(source_entities::list));auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 for(const auto& r:frame->records){auto rule=selected(*cfg,r);if(!rule.enabled||identity(rule)||!r.modelIndex||*r.modelIndex<=0)continue;source_entities::wire::Entry entry;std::uint32_t net=0;if(!source_entities::wire::load(source_entities::reader,entries+r.index*16,entry)||entry.serial!=r.serial||!entry.entity||!source_entities::wire::load(source_entities::reader,object+source_entities::profile::CacheOffset+r.index*8,net)||!net)continue;
 auto t=std::make_shared<Target>();pending.push_back(t);t->epoch=frame->epoch;t->handle=r.handle;t->base=entry.entity;t->index=r.index;t->model=unsigned(*r.modelIndex);t->tick=frame->tick;t->rule=rule;t->label=r.className;t->view=handModelClass(r.className)&&r.className.find("View")!=std::string::npos;
 auto previous=old->bases.find(t->base);if(previous!=old->bases.end()&&(previous->second->handle!=t->handle||previous->second->epoch!=t->epoch||previous->second->model!=t->model)){std::lock_guard<std::mutex> lock(previous->second->state->mutex);restorePose(*previous->second);restoreField(*previous->second);}if(previous!=old->bases.end()&&previous->second->handle==t->handle&&previous->second->epoch==t->epoch&&previous->second->model==t->model)t->state=previous->second->state;
 auto address=source_entities::classAddress(net);if(address){auto info=source_entities::classes.find(*address);if(info!=source_entities::classes.end()){auto mi=info->second.fields.find("m_nModelIndex");if(mi!=info->second.fields.end())t->modelOffset=mi->second.offset;if(previous!=old->bases.end()&&previous->second->state==t->state&&factors(previous->second->rule)!=factors(t->rule)){std::lock_guard<std::mutex> l(t->state->mutex);restorePose(*t);}auto field=info->second.fields.find("m_flModelScale");if(!cfg->studioOnly&&field!=info->second.fields.end()&&field->second.type==1&&field->second.offset<0x4000){t->scaleOffset=field->second.offset;
 t->field=survivorPlayerClass(r.className)&&r.team&&*r.team==3&&uniform(rule);
 bool admitted=t->field;std::uint32_t table=0;if(!admitted&&source_entities::wire::load(source_entities::reader,t->base,table)&&source_camera::owns(reinterpret_cast<void*>(uintptr_t(table)),source_camera::clientModule))admitted=t->build=installBuild(t->buildSlot=reinterpret_cast<void**>(uintptr_t(table+175*4)));
 if(admitted){
  bool proxyReady=false;source_entities::wire::ClientClass c;
  if(source_entities::wire::load(source_entities::reader,*address,c)){
   auto at=props.find(c.table);if(at==props.end())at=props.emplace(c.table,scaleProp(source_entities::reader,c.table)).first;
   if(at->second)proxyReady=installProxy(t->proxySlot=reinterpret_cast<void**>(uintptr_t(at->second+32)));
  }
  if(t->field&&!proxyReady){t->field=false;++failures;}
  if(t->field||t->build){std::lock_guard<std::mutex> l(t->state->mutex);if(t->field)restorePose(*t);if(!setField(*t,t->field?factors(rule)[0]:1.f)){t->field=t->build=false;++failures;}}
 }else t->scaleOffset=0;
 }}}
 if(t->field){t->state->reason=u8"客户端字段与网络代理路径";t->state->route=2;}
 if(!t->field&&!t->build){std::lock_guard<std::mutex> l(t->state->mutex);restorePose(*t);restoreField(*t);}
 next->bases[t->base]=t;++eligible;
 }
 live=false;for(const auto& i:old->bases)if(!next->bases.count(i.first)||next->bases[i.first]->state!=i.second->state){auto& t=*i.second;std::lock_guard<std::mutex> l(t.state->mutex);restorePose(t);restoreField(t);}
 std::atomic_store(&targets,std::shared_ptr<const Targets>(next));live=eligible>0;clean=false;status=eligible?"实体形变已就绪：构建优先，Studio补充":"没有可绘制的已选实体";
 std::set<void**> needBuild,needProxy;for(const auto& i:next->bases){if(i.second->build)needBuild.insert(i.second->buildSlot);if(i.second->build||i.second->field)needProxy.insert(i.second->proxySlot);}for(unsigned i=0;i<buildCount.load();++i)if(buildHooks[i].installed&&!needBuild.count(buildHooks[i].slot))swap(buildHooks[i],false);for(unsigned i=0;i<proxyCount.load();++i)if(proxyHooks[i].installed&&!needProxy.count(proxyHooks[i].slot))swap(proxyHooks[i],false);
}
inline void onFrame(const dango::GameCameraFrame& camera,unsigned limit)noexcept{try{onFrameWork(camera,limit);}catch(...){live=false;for(const auto& t:pending){std::lock_guard<std::mutex> lock(t->state->mutex);restorePose(*t);restoreField(*t);}cleanup();++failures;status="形变初始化失败；已恢复并停止";}}

inline bool studio(std::uint32_t header,std::uint32_t renderable,const Matrix3x4* bones,const dango::Vec3& origin,Matrix3x4* out,unsigned capacity,unsigned& count){if(!live||!bones)return false;std::uint32_t getter=0;if(!source_entities::method(renderable,0,getter))return false;auto base=dango::entitywire::addressGetter(source_entities::reader,getter,renderable);if(!base)return false;auto t=target(reinterpret_cast<void*>(uintptr_t(*base)));if(!t||t->field)return false;
 static thread_local StudioCache cache;auto pose=cache.find(source_entities::reader,header,t->epoch,nullptr);if(!pose||pose->count>capacity){status="模型无有效Studio骨骼或超过骨骼上限";t->state->reason=u8"无有效Studio骨骼或超出上限";t->state->route=4;return false;}count=unsigned(pose->count);std::array<Matrix3x4,256> input;if(!source_entities::reader.read(std::uint32_t(uintptr_t(bones)),input.data(),count*48))return false;
 {std::lock_guard<std::mutex> l(t->state->mutex);auto& s=*t->state;if(t->build&&s.matrix&&s.after.size()==count){std::uint32_t pointer=0;std::array<Matrix3x4,256> currentPose;if(source_entities::wire::load(source_entities::reader,t->base+0x6c0,pointer)&&pointer==s.matrix&&source_entities::reader.read(pointer,currentPose.data(),count*48)){bool same=true,changed=false;for(unsigned i=0;i<count;++i)if(s.changed[i]){changed=true;if(std::memcmp(&currentPose[i],&s.after[i],48))same=false;}if(same&&changed)return false;}}}

 float pivot[3]={origin.x,origin.y,origin.z};if(t->view&&!centroid(input.data(),count,pivot))return false;if(!transform(input.data(),out,count,pivot,t->rule))return false;t->state->reason=u8"Studio绘制副本路径";t->state->route=3;++studioDraws;return true;
}
inline void stop(){stopRequested=true;live=false;if(GetCurrentThreadId()==owner&&!clean)cleanup();for(unsigned i=0;!clean&&i<30;++i)Sleep(5);detach(buildHooks,buildCount.load());detach(proxyHooks,proxyCount.load());}
}
