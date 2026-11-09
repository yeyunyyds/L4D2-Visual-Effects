#pragma once
#include "aim_selection.hpp"
#include "local_player.hpp"
#include "image_trace.hpp"
#include <memory>
namespace visual::aimruntime {
struct Request{std::uint64_t id=0,tick=0;std::uint32_t epoch=0;bool hands=false;};
inline std::shared_ptr<const Request> pending;
inline std::shared_ptr<const AimResult> completed=std::make_shared<const AimResult>();
inline std::atomic<std::uint64_t> sequence{0},failed{0};
inline std::atomic<bool> stopped{false};
inline bool traceFaulted=false;inline std::uint32_t traceEpoch=0;
inline bool hasPending(){return bool(std::atomic_load(&pending));}
inline void cancel(AimSession& session){auto queued=std::atomic_load(&pending);if(queued&&queued->id==session.request)std::atomic_compare_exchange_strong(&pending,&queued,std::shared_ptr<const Request>{});session.clear();}
inline void stop(){stopped=true;std::atomic_store(&pending,std::shared_ptr<const Request>{});}
inline void start(AimSession& session){
 cancel(session);if(stopped){session.aimStatus=u8"插件已停止";return;}
 auto camera=std::atomic_load(&source_camera::snapshot);auto tick=GetTickCount64();auto id=++sequence;
 session.begin(id,camera->epoch,tick);
 std::atomic_store(&pending,std::make_shared<const Request>(Request{id,tick,camera->epoch,session.handsOnly}));
}
inline void poll(AimSession& session,std::uint32_t epoch){
 if(session.aimEpoch!=epoch)cancel(session);session.validateEpoch(epoch);if(session.pending&&failed.load()==session.request){session.clear();session.aimStatus=u8"识别失败，请重新尝试";}auto result=std::atomic_load(&completed);session.accept(*result);
 auto now=GetTickCount64();if(session.pending&&now>=session.requestedAt&&now-session.requestedAt>1500){session.clear();session.aimStatus=u8"识别等待超时，请返回游戏后重试";}
}
inline bool __fastcall hit(void* self,void*,void* entity,int);
inline int __fastcall everything(void*,void*){return 0;}
inline void* filterMethods[]={reinterpret_cast<void*>(&hit),reinterpret_cast<void*>(&everything)};
struct Filter{void** table=filterMethods;std::set<std::uint32_t> ignored;};
inline bool __fastcall hit(void* self,void*,void* entity,int){return !static_cast<Filter*>(self)->ignored.count(std::uint32_t(reinterpret_cast<uintptr_t>(entity)));}
// Source/trace calls happen only in the existing game-thread frame listener.
// HUD input only queues a request and later consumes an immutable result.
inline void onFrame(const dango::GameCameraFrame& camera){
 auto request=std::atomic_exchange(&pending,std::shared_ptr<const Request>{});if(!request||stopped)return;
 try{
 AimResult result;result.id=request->id;result.epoch=request->epoch;result.tick=camera.tick;
 auto finish=[&](){std::atomic_store(&completed,std::make_shared<const AimResult>(std::move(result)));};
 auto frame=std::atomic_load(&source_entities::snapshot);
 if(!camera.active||request->epoch!=camera.epoch||camera.tick<request->tick||camera.tick-request->tick>1500||frame->epoch!=camera.epoch||camera.tick<frame->tick||camera.tick-frame->tick>250){result.status=u8"游戏或实体快照不可用，请重新识别";finish();return;}
 auto object=std::uint32_t(reinterpret_cast<uintptr_t>(source_entities::list));auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 std::array<source_entities::wire::Entry,source_entities::profile::MaxNetworked> identities{};
 if(!object||!source_entities::reader.read(entries,identities.data(),sizeof(identities))){result.status=u8"实体句柄校验不可用";finish();return;}
 auto verified=currentAimFrame(*frame,identities);
 AimQuery query;query.localHandle=localplayer::handle(verified);query.handsOnly=request->hands;
 if(query.localHandle==0xffffffffu){result.status=u8"本机玩家身份不可用，未执行识别";finish();return;}
 if(!query.handsOnly){
  if(traceEpoch!=camera.epoch){traceEpoch=camera.epoch;traceFaulted=false;}
  if(traceFaulted||picturetrace::faulted){result.status=u8"射线校验失败，本地图停止场景识别";finish();return;}
  dango::CameraPose eye;if(!dango::cameraPose(camera,eye)||!picturetrace::connect()){result.status=u8"射线接口未验证，未执行识别";finish();return;}
  Filter filter;auto links=aimOwnerLinks(verified);
  for(const auto& r:verified.records)if(r.index<identities.size()&&identities[r.index].serial==r.serial&&(r.handle==query.localHandle||aimActorOwner(verified,r,&links)==query.localHandle))filter.ignored.insert(identities[r.index].entity);
  static bool pinned=false;if(!pinned){HMODULE module=nullptr;pinned=GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<const wchar_t*>(&pending),&module)!=FALSE;}if(!pinned){result.status=u8"识别模块未就绪";finish();return;}
  auto end=eye.eye+eye.forward*query.limit;auto ray=dango::trace::ray(eye.eye,end);dango::trace::GuardedResult trace;
  using Call=void(__thiscall*)(void*,const dango::trace::Ray&,unsigned,void*,dango::trace::Result*);
  auto table=*reinterpret_cast<void***>(picturetrace::object);reinterpret_cast<Call>(table[5])(picturetrace::object,ray,0x4600400b,&filter,&trace.result);
  if(!trace.intact()||!dango::trace::valid(trace.result,eye.eye,end)){traceFaulted=true;result.status=u8"射线校验失败，本地图停止场景识别";finish();return;}
  if(trace.result.startSolid||trace.result.allSolid){result.status=u8"射线起点在实体内部，请换位置";finish();return;}
  query.limit=std::max(.01f,1200*trace.result.fraction+1);
  if(trace.result.fraction<1){
   query.blocked=true;
   for(const auto& r:verified.records)if(r.index&&r.index<identities.size()&&identities[r.index].serial==r.serial&&identities[r.index].entity==trace.result.entity){query.hitHandle=r.handle;query.blocked=false;break;}
  }
 }
 result.candidates=identifyAimCandidates(verified,camera,camera.tick,query);
 result.status=result.candidates.empty()?u8"未命中可识别实体，墙体遮挡或角色资料不足":query.handsOnly?u8"已列出本机手持实体，请手动选择":u8"已识别本体，可切换候选或附属实体";
 finish();
}catch(...){failed=request->id;}
}
}
