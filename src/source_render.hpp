#pragma once
#include <windows.h>
#include <atomic>
#include <map>
#include <mutex>
#include "dango/source_entities.hpp"
#include "render_contract.hpp"
#include "studio_pose.hpp"
#include "studio_cache.hpp"
#include "target_identity.hpp"
#include "target_index.hpp"
#include "entity_selection.hpp"
#include "proximity.hpp"
#include "render_settings.hpp"
#include <unordered_map>
#include "render_profile.hpp"
#include "module_verification.hpp"

namespace visual::render {
using dango::Vec3;
struct DrawState {std::uint32_t header,hardware,renderable,modelToWorld,decals,flags,lod;};
struct ModelInfo {
    Vec3 origin,angles;
    std::uint32_t renderable,model,modelToWorld,lightingOffset,lightingOrigin;
    int flags,index,skin,body,hitbox;
    std::uint16_t instance,padding;
};
static_assert(sizeof(DrawState)==28 && offsetof(ModelInfo,index)==48 && sizeof(ModelInfo)==68);
using EngineDraw=void(__thiscall*)(void*,const DrawState&,const ModelInfo&,Matrix3x4*);
using StudioDraw=void(__thiscall*)(void*,void*,const void*,Matrix3x4*,float*,float*,const Vec3&,int);
inline std::atomic<void(*)(std::uint32_t,int)> modelObserver{nullptr};
inline EngineDraw engineOriginal=nullptr;
inline StudioDraw studioOriginal=nullptr;
inline void** engineTable=nullptr;
inline void** studioTable=nullptr;
inline std::atomic<bool> enabled{false},bound{false};
inline std::atomic<unsigned> boneLimit{128};
inline std::atomic<bool> compatibility{true};
inline std::shared_ptr<const EntitySelection> selection=std::make_shared<const EntitySelection>();
inline std::shared_ptr<const ProximitySettings> proximity=std::make_shared<const ProximitySettings>();
inline std::atomic<bool> proximityActive{false};
inline std::atomic<unsigned> proximityCount{0},proximitySources{0};
inline std::shared_ptr<const ProximityResult> proximityTargets=std::make_shared<const ProximityResult>();
inline std::shared_ptr<const AnimationSettings> animation=std::make_shared<const AnimationSettings>();
inline std::atomic<ULONGLONG> startedAt{0};
inline std::atomic<unsigned long> drawn{0},rejected{0};
inline std::atomic<unsigned long> engineCalls{0},identified{0},studioCalls{0};
inline std::atomic<const char*> identifyReason{"No target draw yet"},poseReason{"No associated studio draw yet"};
inline std::atomic<const char*> status{"Waiting for game modules"};
inline HMODULE studioModule=nullptr;
struct Context {bool valid=false;DrawIdentity identity{};std::uint32_t header{},renderable{};std::uint64_t age{};int classId=-1;};
inline thread_local Context* context=nullptr;

template<std::size_t N> bool samples(HMODULE module,const profile::Sample(&list)[N]) {
    for(const auto& sample:list) {
        const auto p=reinterpret_cast<const unsigned char*>(module)+sample.rva;
        if(!source_camera::readable(p,sample.length)){source_camera::log("Render fingerprint unreadable RVA="+std::to_string(sample.rva));return false;}
        for(unsigned i=0;i<sample.length;++i)
            if((p[i]&static_cast<unsigned char>(sample.mask[i]))!=
               (static_cast<unsigned char>(sample.bytes[i])&static_cast<unsigned char>(sample.mask[i]))){source_camera::log("Render fingerprint mismatch RVA="+std::to_string(sample.rva)+" byte="+std::to_string(i));return false;}
    }
    return true;
}
inline bool patch(void** table,unsigned slot,void* expected,void* replacement) {
    DWORD previous{};
    if(!VirtualProtect(table+slot,sizeof(void*),PAGE_READWRITE,&previous))return false;
    auto actual=InterlockedCompareExchangePointer(table+slot,replacement,expected);
    DWORD ignored{};VirtualProtect(table+slot,sizeof(void*),previous,&ignored);
    return actual==expected;
}
struct IndexedFrame {
 std::shared_ptr<const source_entities::Frame> frame;
 TargetIndex<source_entities::Frame,source_entities::profile::MaxNetworked> index;
 std::unordered_map<std::uint32_t,source_entities::ClassInfo> classes;
 std::shared_ptr<const EntitySelection> selected;
 std::shared_ptr<const ProximitySettings> experiment;
 std::shared_ptr<const ProximityResult> spatial;
 void update(std::shared_ptr<const source_entities::Frame> next){
  auto options=std::atomic_load(&selection);
  auto settings=std::atomic_load(&proximity);auto targets=std::atomic_load(&proximityTargets);
  if(frame==next && selected==options && experiment==settings && spatial==targets)return;
  if(frame!=next)classes.clear();
  frame=std::move(next);selected=std::move(options);experiment=std::move(settings);spatial=std::move(targets);
  const bool spatialReady=spatial->epoch==frame->epoch&&spatial->tick==frame->tick&&spatial->options==experiment;
  auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_camera::clientModule))+source_entities::profile::Object;
  auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
  index.rebuild(*frame,[&](unsigned i){source_entities::wire::Entry entry{};
   if(!source_entities::wire::load(source_entities::reader,entries+i*sizeof(entry),entry))entry={};return entry;},
   [&](const auto& r){return (!experiment->enabled||spatialReady)&&selectedForEffect(r,*selected,*experiment,*spatial);});
 }
};
inline thread_local IndexedFrame indexed;
inline Context identify(const DrawState& state,const ModelInfo& info) {
    using namespace source_entities;
    Context result;
    if(info.index<0 || info.index>=int(source_entities::profile::MaxNetworked) ||
       !state.header || !state.renderable || state.renderable!=info.renderable){identifyReason.store("invalid index/header/renderable",std::memory_order_relaxed);return result;}
    const auto camera=std::atomic_load(&source_camera::snapshot);
    const auto frame=std::atomic_load(&snapshot);
    const auto now=GetTickCount64();
    if(!camera->active || camera->epoch!=frame->epoch || now<frame->tick ||
       now-frame->tick>250){identifyReason.store("inactive camera / map epoch / stale snapshot",std::memory_order_relaxed);return result;}
    indexed.update(frame);
    const auto* record=indexed.index.records[info.index];
    if(!record){identifyReason.store("snapshot lacks model identity",std::memory_order_relaxed);return result;}
    const auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_camera::clientModule))+source_entities::profile::Object;
    wire::Entry entry;std::uint32_t net{};
    const auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
    if(!wire::load(reader,entries+info.index*sizeof(entry),entry)||!entry.entity||
       !wire::load(reader,object+source_entities::profile::CacheOffset+info.index*8,net)||!net){identifyReason.store("entity entry or networkable unreadable",std::memory_order_relaxed);return result;}
    const auto handle=wire::handle(info.index,entry.serial);
    if(!handle){identifyReason.store("invalid handle",std::memory_order_relaxed);return result;}
    if(record->handle!=*handle){identifyReason.store("current entity serial mismatch",std::memory_order_relaxed);return result;}
    // Decode the profiled tiny getters; do not execute entity virtual methods.
    std::uint32_t getter{};
    if(!method(state.renderable,0,getter)){identifyReason.store("renderable getter unavailable",std::memory_order_relaxed);return result;}
    auto base=wire::addressGetter(reader,getter,state.renderable);
    if(!base || *base!=entry.entity){identifyReason.store("renderable getter does not map to entity base",std::memory_order_relaxed);return result;}
    if(!method(net,source_entities::profile::DormantSlot,getter)){identifyReason.store("dormancy getter unavailable",std::memory_order_relaxed);return result;}
    auto dormant=wire::byteGetter(reader,getter,net);
    if(!dormant || *dormant){identifyReason.store("dormant or unknown dormancy",std::memory_order_relaxed);return result;}
    auto address=classAddress(net);if(!address){identifyReason.store("class getter unavailable",std::memory_order_relaxed);return result;}
    auto known=indexed.classes.find(*address);
    if(known==indexed.classes.end())known=indexed.classes.emplace(*address,readClass(*address)).first;
    const auto& cls=known->second;
    if(cls.name!=record->className || cls.id!=record->classId){identifyReason.store("current class mismatch",std::memory_order_relaxed);return result;}
    if(!indexed.experiment->enabled&&survivorPlayerClass(record->className)){
        auto team=field<int>(entry.entity,cls,"m_iTeamNum",0);
        if(!team||!record->team||*team!=*record->team){identifyReason.store("current player team mismatch",std::memory_order_relaxed);return result;}
        if(*team==2){auto character=field<int>(entry.entity,cls,"m_survivorCharacter",0);
            if(!character||!record->character||*character!=*record->character){identifyReason.store("current survivor character mismatch",std::memory_order_relaxed);return result;}}
        if(*team==3){auto zombie=field<int>(entry.entity,cls,"m_zombieClass",0);
            if(!zombie||!record->zombieClass||*zombie!=*record->zombieClass){identifyReason.store("current special infected type mismatch",std::memory_order_relaxed);return result;}}
    }
    const auto model=field<std::int16_t>(entry.entity,cls,"m_nModelIndex",0);
    if(!model || *model!=*record->modelIndex){identifyReason.store("current model index mismatch",std::memory_order_relaxed);return result;}
    identifyReason.store("Entity matched",std::memory_order_relaxed);result.valid=true;result.identity={frame->epoch,*handle,*model};
    result.header=state.header;result.renderable=state.renderable;
    result.age=now-frame->tick;result.classId=record->classId;
    return result;
}
// Some engine-internal/shadow paths dispatch Studio directly instead of the
// public DrawModelExecute vtable. Match that draw's real renderable to the same
// serial-checked raw entity entry, then reuse all identify checks.
inline Context identifyStudio(std::uint32_t header,std::uint32_t renderable) {
    Context empty;std::uint32_t getter{};
    if(!header || !renderable || !source_entities::method(renderable,0,getter))return empty;
    auto base=source_entities::wire::addressGetter(source_entities::reader,getter,renderable);
    if(!base)return empty;
    auto frame=std::atomic_load(&source_entities::snapshot);
    auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_camera::clientModule))+source_entities::profile::Object;
    auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
    indexed.update(frame);
    const auto found=indexed.index.bases.find(*base);
    if(found!=indexed.index.bases.end()) {
        const auto* r=indexed.index.records[found->second];
        source_entities::wire::Entry entry;
        if(r && source_entities::wire::load(source_entities::reader,entries+r->index*sizeof(entry),entry) &&
           sameRenderableTarget(*r,entry.entity,entry.serial,*base)) {
            DrawState state{};state.header=header;state.renderable=renderable;
            ModelInfo info{};info.index=int(r->index);info.renderable=renderable;
            return identify(state,info);
        }
    }
    return empty;
}
struct TargetStats {
 std::atomic<unsigned long> accepted{0},rejected{0},direct{0};
 std::atomic<const char*> reason{"not drawn"};std::string model;int classId{},modelIndex{};std::uint32_t epoch{};
};
inline std::mutex targetMutex;
inline std::map<std::uint64_t,TargetStats> targets;
inline void targetResult(const Context& target,bool accepted,bool direct,const std::string& model) {
    // Shared registry is only touched on the first draw of each class/model on a thread.
    const auto key=(std::uint64_t(target.identity.epoch)<<32)|(std::uint64_t(std::uint16_t(target.classId))<<16)|std::uint16_t(target.identity.modelIndex);
    static thread_local std::unordered_map<std::uint64_t,TargetStats*> slots;
    auto found=slots.find(key);
    if(found==slots.end()) {
        std::lock_guard<std::mutex> lock(targetMutex);
        if(targets.size()>=4096)return; // Bound diagnostic memory; rendering continues.
        auto& stat=targets[key];stat.classId=target.classId;stat.modelIndex=target.identity.modelIndex;stat.epoch=target.identity.epoch;
        if(stat.model.empty())stat.model=model;
        found=slots.emplace(key,&stat).first;
    }
    auto& stat=*found->second;
    if(accepted)stat.accepted.fetch_add(1,std::memory_order_relaxed);
    else stat.rejected.fetch_add(1,std::memory_order_relaxed);
    if(direct)stat.direct.fetch_add(1,std::memory_order_relaxed);
    stat.reason.store(accepted?"accepted":poseReason.load(std::memory_order_relaxed),std::memory_order_relaxed);
}
inline std::string targetSummary() {
    std::lock_guard<std::mutex> lock(targetMutex);std::string out;
    for(const auto& pair:targets){const auto& s=pair.second;
        out+=" [epoch="+std::to_string(s.epoch)+" classId="+std::to_string(s.classId)+" modelIndex="+std::to_string(s.modelIndex)+" model="+s.model+
        " accepted="+std::to_string(s.accepted.load(std::memory_order_relaxed))+
        " rejected="+std::to_string(s.rejected.load(std::memory_order_relaxed))+
        " direct="+std::to_string(s.direct.load(std::memory_order_relaxed))+" reason="+s.reason.load(std::memory_order_relaxed)+"]";
    }
    return out;
}
inline void __fastcall engineHook(void* self,void*,const DrawState& state,const ModelInfo& info,Matrix3x4* bones) {
    engineCalls.fetch_add(1,std::memory_order_relaxed);
    if(auto observe=modelObserver.load(std::memory_order_acquire))try{observe(state.header,info.index);}catch(...){}
    Context current;
    if(enabled.load())try{current=identify(state,info);}catch(...){rejected.fetch_add(1,std::memory_order_relaxed);}
    if(current.valid)identified.fetch_add(1,std::memory_order_relaxed);
    struct Scope {Context* previous;~Scope(){context=previous;}} scope{context};
    context=&current;
    // Keep the original engine pose intact, including hitbox/debug consumers.
    engineOriginal(self,state,info,bones);
}
template<std::size_t Capacity> __declspec(noinline) void studioTargetDraw(
    void* self,void* results,const void* info,Matrix3x4* bones,float* flex,float* delayed,
    const Vec3& origin,int flags,Context* target,bool direct) {
    BasicPoseCopy<Capacity> source,copy;
    std::uint32_t header{},renderable{};
    const auto ptr=std::uint32_t(reinterpret_cast<std::uintptr_t>(info));const StudioPose* studio=nullptr;
    static thread_local StudioCache cache;
    const char* studioFailure="Studio metadata rejected";bool changed=false;
    try {
        if(enabled.load() && (target && target->valid) && (poseReason.store("Missing draw bone pointer",std::memory_order_relaxed),bones) &&
           source_entities::wire::load(source_entities::reader,ptr,header) &&
           source_entities::wire::load(source_entities::reader,ptr+24,renderable) &&
           (poseReason.store("Studio header/renderable context mismatch",std::memory_order_relaxed),header==target->header && renderable==target->renderable) &&
           (poseReason.store("Studio header / bone names / hierarchy rejected",std::memory_order_relaxed), (studio=cache.find(source_entities::reader,header,target->identity.epoch,&studioFailure))) &&
           (poseReason.store("Model exceeds selected bone limit",std::memory_order_relaxed),studio->count<=Capacity) &&
           (poseReason.store("Bone matrix memory unreadable",std::memory_order_relaxed),readDrawPose(source_entities::reader,std::uint32_t(reinterpret_cast<std::uintptr_t>(bones)),*studio,source))) {
            poseReason.store("Foot anchor invalid",std::memory_order_relaxed);float anchor{};
            if(poseAnchor(*studio,source.bones.data(),anchor)) {
                const auto elapsed=GetTickCount64()-startedAt.load();
                auto parameters=std::atomic_load(&animation);
                RenderInput input{target->identity,target->identity,source.bones.data(),studio->count,
                                  anchor,elapsed,target->age,true,true,false,true,true,parameters->periodMs,parameters->minTenths,parameters->maxTenths,parameters->preset};
                poseReason.store("Nonfinite bone matrix / draw contract rejected",std::memory_order_relaxed);changed=prepareDraw(input,copy);
                if(changed)poseReason.store("Pose accepted",std::memory_order_relaxed);
                if(changed)preserveFeet(*studio,source.bones.data(),copy);
            }
            if(!changed)rejected.fetch_add(1,std::memory_order_relaxed);
        }
    }catch(...){rejected.fetch_add(1,std::memory_order_relaxed);}
    if(!changed && std::strcmp(poseReason.load(),"Studio header / bone names / hierarchy rejected")==0)poseReason.store(studioFailure,std::memory_order_relaxed);
    static const std::string emptyModel;
    if(enabled.load() && target && target->valid)targetResult(*target,changed,direct,studio?studio->name:emptyModel);
    // The verified studio implementation copies non-render-arena matrices for
    // queued draws (66e50). Our local copy survives until that copy completes.
    studioOriginal(self,results,info,changed?copy.bones.data():bones,flex,delayed,origin,flags);
    if(changed) {
        if(drawn.fetch_add(1,std::memory_order_relaxed)==0)source_camera::log("First visual stretch submitted; original entity and bone cache untouched");
        status="Stretch draws submitted; inspect feet and animation in game";
    }
}
inline void __fastcall studioHook(void* self,void*,void* results,const void* info,Matrix3x4* bones,
                                  float* flex,float* delayed,const Vec3& origin,int flags) {
    studioCalls.fetch_add(1,std::memory_order_relaxed);
    if(!enabled.load()){studioOriginal(self,results,info,bones,flex,delayed,origin,flags);return;}
    Context fallback;Context* target=context;bool direct=false;
    std::uint32_t header{},renderable{};
    auto ptr=std::uint32_t(reinterpret_cast<std::uintptr_t>(info));
    try {
        if((!target || !target->valid) &&
           source_entities::wire::load(source_entities::reader,ptr,header) &&
           source_entities::wire::load(source_entities::reader,ptr+24,renderable)) {
            fallback=identifyStudio(header,renderable);
            if(fallback.valid){target=&fallback;direct=true;identified.fetch_add(1,std::memory_order_relaxed);}
        }
    }catch(...){rejected.fetch_add(1,std::memory_order_relaxed);target=nullptr;}
    // Ordinary props/non-target draws must not zero two 12KB pose buffers.
    if(!target || !target->valid){studioOriginal(self,results,info,bones,flex,delayed,origin,flags);return;}
    if(boneLimit.load(std::memory_order_relaxed)==256)
        studioTargetDraw<256>(self,results,info,bones,flex,delayed,origin,flags,target,direct);
    else studioTargetDraw<128>(self,results,info,bones,flex,delayed,origin,flags,target,direct);
}

inline void connect() {
    using namespace source_camera;
    if(bound.load())return;
    auto engineModule=source_camera::engineModule;
    auto clientModule=source_camera::clientModule;
    if(!studioModule)studioModule=GetModuleHandleW(L"studiorender.dll");
    if(!engineModule || !clientModule || !studioModule){status="Waiting for client/engine/studiorender modules";return;}
    status="Render ABI check failed; original drawing retained";
    if(!matchesImage(engineModule,EngineTimestamp,EngineImageSize,"visual engine") ||
       !matchesImage(clientModule,ClientTimestamp,ClientImageSize,"visual client") ||
       !matchesImage(studioModule,1782844886,6631424,"visual studio"))return;
    if(!verifiedModuleFile(engineModule,"8a6d468059394bf4cfce32f11dcba12ef45de88da1373084d5a2916d0bf7ba86") ||
       !verifiedModuleFile(clientModule,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f") ||
       !verifiedModuleFile(studioModule,"03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6")) {
        status="Disk game DLL hash mismatch; render hooks refused";return;
    }
    if(!samples(engineModule,visual::profile::engineSamples) ||
       !samples(clientModule,visual::profile::clientSamples) ||
       !samples(studioModule,visual::profile::studioSamples))
        log("Live code differs from verified disk image; retaining executable callback chain");
    using Factory=void*(__cdecl*)(const char*,int*);
    auto ef=reinterpret_cast<Factory>(GetProcAddress(engineModule,"CreateInterface"));
    auto sf=reinterpret_cast<Factory>(GetProcAddress(studioModule,"CreateInterface"));
    if(!ef || !sf){status="CreateInterface export missing";return;}
    auto e=ef("VEngineModel016",nullptr),s=sf("VStudioRender026",nullptr);
    auto eb=reinterpret_cast<std::uintptr_t>(engineModule),sb=reinterpret_cast<std::uintptr_t>(studioModule);
    if(reinterpret_cast<std::uintptr_t>(e)!=eb+0x604248 || reinterpret_cast<std::uintptr_t>(s)!=sb+0x437018 ||
       !readable(e,4)||!readable(s,4)){status="Render interface object mismatch";return;}
    auto et=*reinterpret_cast<void***>(e),st=*reinterpret_cast<void***>(s);
    if(reinterpret_cast<std::uintptr_t>(et)!=eb+0x373864 || reinterpret_cast<std::uintptr_t>(st)!=sb+0x7f03c ||
       !readable(et,20*4)||!readable(st,26*4)){status="Render vtable mismatch";return;}
    // The original file's full hash proves the profiled ABI. Chain the live
    // targets, including existing vtable hooks / inline detours, without bypass.
    if(!executable(et[19]) || !executable(st[25]) ||
       !executable(reinterpret_cast<void*>(eb+0xe0600)) ||
       !executable(reinterpret_cast<void*>(sb+0x6a150))) {
        status="Render callback is not executable; hooks refused";return;
    }
    log("Chaining engine callback owner="+targetOwner(et[19])+" studio owner="+targetOwner(st[25]));
    engineOriginal=reinterpret_cast<EngineDraw>(et[19]);studioOriginal=reinterpret_cast<StudioDraw>(st[25]);
    engineTable=et;studioTable=st;
    if(!patch(st,25,reinterpret_cast<void*>(studioOriginal),reinterpret_cast<void*>(&studioHook)))return;
    if(!patch(et,19,reinterpret_cast<void*>(engineOriginal),reinterpret_cast<void*>(&engineHook))) {
        patch(st,25,reinterpret_cast<void*>(&studioHook),reinterpret_cast<void*>(studioOriginal));return;
    }
    bound=true;status="Render hooks installed; awaiting enabled target draws";
    log("Visual DrawModelExecute[19] + Studio DrawModel[25] installed (offline-verified ABI; game test pending)");
}
inline void stop() {
    enabled=false;
    if(engineTable && source_camera::readable(engineTable,20*4))
        patch(engineTable,19,reinterpret_cast<void*>(&engineHook),reinterpret_cast<void*>(engineOriginal));
    if(studioTable && source_camera::readable(studioTable,26*4))
        patch(studioTable,25,reinterpret_cast<void*>(&studioHook),reinterpret_cast<void*>(studioOriginal));
    bound=false;
}
}
