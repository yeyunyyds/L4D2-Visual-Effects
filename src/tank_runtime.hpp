#pragma once
#include "tank_cvars.hpp"
#include "tank_sequence.hpp"
#include "tank_sequence_catalog.hpp"
#include "tank_observed_timer.hpp"
#include "tank_layer_attack.hpp"
#include "tank_visual_state.hpp"
#include "entity_menu.hpp"
#include <mutex>
namespace visual::tank {
inline std::atomic<bool> faulted{false};
inline std::shared_ptr<const Result> emptyResult=std::make_shared<const Result>();
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::shared_ptr<const Result> snapshot=std::make_shared<const Result>();
inline auto emptyObserved=std::make_shared<const std::map<std::uint32_t,ObservedTimer>>();
inline std::shared_ptr<const std::map<std::uint32_t,ObservedTimer>> observedSnapshot=emptyObserved;
inline std::mutex animationMutex;
inline std::map<std::uint32_t,RenderAnchor> renderAnchors;
// Render callbacks supply only entity-following positions. Animation semantics
// come directly from the known sequence catalog and sampled network overlays.
inline void observeAnchor(const Record& record,std::uint32_t epoch,const Vec3& origin){
 auto now=GetTickCount64();RenderAnchor pose{epoch,record.handle,now,origin};
 if(!record.networkOrigin||!validRenderAnchor(pose,epoch,record.handle,now,*record.networkOrigin))return;
 std::lock_guard<std::mutex> lock(animationMutex);if(renderAnchors.size()>16)renderAnchors.clear();renderAnchors[record.handle]=pose;
}
inline void observeModel(std::uint32_t,int index,const Vec3& origin){
 if(faulted||!std::atomic_load(&options)->enabled)return;auto frame=std::atomic_load(&source_entities::combatSnapshot);auto camera=std::atomic_load(&source_camera::snapshot);auto now=GetTickCount64();
 if(!camera->active||camera->epoch!=frame->epoch||now<frame->tick||now-frame->tick>250)return;
 for(const auto& r:frame->records)if(r.index==unsigned(index)&&tank(r)){
  if(!source_entities::list)break;auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_entities::list));auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);source_entities::wire::Entry entry;
  if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(entry),entry)&&entry.entity&&entry.serial==r.serial)observeAnchor(r,frame->epoch,origin);break;
 }
}
// The fast model path can bypass DrawModelExecute. Observe the existing Studio
// hook too, even while stretching is off. Resolve only Tank renderables via the
// already-validated tiny getter and full entity serial. Never issue GPU commands.
inline void observeStudio(std::uint32_t,std::uint32_t renderable,const Vec3& origin){
 if(faulted||!std::atomic_load(&options)->enabled)return;
 auto camera=std::atomic_load(&source_camera::snapshot);auto frame=std::atomic_load(&source_entities::combatSnapshot);auto now=GetTickCount64();
 if(!camera->active||frame->epoch!=camera->epoch||now<frame->tick||now-frame->tick>250||!source_entities::list)return;
 if(!renderable)return;
 // Cache only the decoded getter result, never a class/serial decision. Models
 // may have arbitrary filenames; identity comes from the matched Tank entity.
 struct CachedBase{std::uint32_t base=0;std::uint64_t tick=0;};static thread_local std::uint32_t cacheEpoch=0;static thread_local std::map<std::uint32_t,CachedBase> bases;
 if(cacheEpoch!=frame->epoch){bases.clear();cacheEpoch=frame->epoch;}auto found=bases.find(renderable);
 if(found==bases.end()||now<found->second.tick||now-found->second.tick>250){std::uint32_t getter=0;std::optional<std::uint32_t> decoded;if(source_entities::method(renderable,0,getter))decoded=source_entities::wire::addressGetter(source_entities::reader,getter,renderable);if(bases.size()>=2048)bases.clear();found=bases.insert_or_assign(renderable,CachedBase{decoded.value_or(0),now}).first;}
 if(!found->second.base)return;
 struct Identity{const Record* record;};static thread_local std::shared_ptr<const Frame> indexed;static thread_local std::map<std::uint32_t,Identity> tanks;
 auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_entities::list));auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 if(indexed!=frame){indexed=frame;tanks.clear();for(const auto& r:frame->records)if(tank(r)){source_entities::wire::Entry e;if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(e),e)&&e.entity&&e.serial==r.serial)tanks[e.entity]={&r};}}
 auto match=tanks.find(found->second.base);if(match==tanks.end())return;auto& r=*match->second.record;source_entities::wire::Entry entry;
 if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(entry),entry)&&entry.entity==found->second.base&&entry.serial==r.serial){observeAnchor(r,frame->epoch,origin);}

}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 if(faulted)return;
 static Predictor predictor;static std::shared_ptr<const Frame> previous;
 static std::uint32_t timerEpoch=0;static std::map<std::uint32_t,SwingStopwatch> timers;static std::map<std::uint32_t,LayerAttackDetector> layerDetectors;
 auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::combatSnapshot);
 if(!cfg->enabled||!camera.active){predictor.reset();previous.reset();timers.clear();layerDetectors.clear();{std::lock_guard<std::mutex> lock(animationMutex);renderAnchors.clear();}if(std::atomic_load(&observedSnapshot)!=emptyObserved)std::atomic_store(&observedSnapshot,emptyObserved);if(std::atomic_load(&snapshot)!=emptyResult)std::atomic_store(&snapshot,emptyResult);return;}
 if(previous==frame||frame->epoch!=camera.epoch||camera.tick<frame->tick||camera.tick-frame->tick>250)return;previous=frame;
 auto out=predictor.update(*frame,*std::atomic_load(&parameters),*cfg);
 if(timerEpoch!=out.epoch){timers.clear();layerDetectors.clear();timerEpoch=out.epoch;}
 std::map<std::uint32_t,SwingStopwatch> nextTimers;std::map<std::uint32_t,ObservedTimer> readings;
 std::map<std::uint32_t,LayerAttackDetector> nextDetectors;
 for(auto& o:out.tanks){auto names=o.sequence?knownTankSequence(*o.sequence):std::nullopt;
  if(names)o.action=classify(*names)+": "+names->label;
  auto timer=timers.find(o.handle);SwingStopwatch watch=timer==timers.end()?SwingStopwatch{}:timer->second;
  const Record* actor=nullptr;for(const auto& r:frame->records)if(r.handle==o.handle){actor=&r;break;}
  std::vector<LayerSample> layers;if(actor)for(const auto& l:actor->animationLayers){LayerSample sample{l.index,l.sequence,l.cycle,l.weight,{}};
   sample.names=knownTankSequence(l.sequence);layers.push_back(std::move(sample));
  }
  auto oldDetector=layerDetectors.find(o.handle);auto detector=oldDetector==layerDetectors.end()?LayerAttackDetector{}:oldDetector->second;
  auto layer=detector.update(out.clock.time,actor&&actor->animationLayerReason==dango::animation::SampledReason,layers);
  nextDetectors[o.handle]=std::move(detector);if(layer.active)o.action="attack-layer: "+layer.names.label;
  auto reading=watch.update(out.clock.time,out.tick,o.sequence.value_or(-1),o.cycle,names,out.parameters.interval,out.parameters.missInterval,false,layer.onset,out.parameters.windup);
   // The visible value is based on observed onset and live attack parameters.
  o.cooldown=reading.remaining;readings[o.handle]=reading;nextTimers[o.handle]=watch;
 }
 layerDetectors=std::move(nextDetectors);
 timers=std::move(nextTimers);std::atomic_store(&observedSnapshot,std::make_shared<const std::map<std::uint32_t,ObservedTimer>>(std::move(readings)));
 std::atomic_store(&snapshot,std::make_shared<const Result>(std::move(out)));
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
inline std::string menu(std::uintptr_t page=320){auto cfg=std::atomic_load(&options);auto f=std::atomic_load(&snapshot);auto p=std::atomic_load(&parameters);auto v=std::atomic_load(&visualSettings);auto observed=std::atomic_load(&observedSnapshot);
 auto number=[](std::optional<float> n){return n?std::to_string(int(*n*10)/10.f):std::string(u8"未知");};
 auto displayLabel=!cfg->enabled?u8"显示内容（待开启总开关）":!cfg->draw?u8"显示内容（画面关）":(!v->showBar&&!v->showRange&&!v->showDirection&&!v->showTarget)?u8"显示内容（全部关闭）":u8"显示内容（画面开）";
 if(page==320)return menuPage(u8"Tank 预测",
  menuItem(cfg->enabled?u8"总开关：开":u8"总开关：关（默认）",330)+menuItem(displayLabel,322)+menuItem(u8"样式与位置预览",321)+menuItem(u8"当前 Tank 状态",323)+menuItem(u8"参数设置",324)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101)+menuRow(u8"当前检测 Tank："+std::to_string(f->tanks.size()))+menuRow(u8"计时为估算，不代表真实命中边界"));
 if(page==322)return menuPage(u8"Tank · 显示内容",
  menuItem(cfg->draw?u8"画面显示：开":u8"画面显示：关",331)+menuItem(v->showBar?u8"攻击计时条：开":u8"攻击计时条：关",372)+menuItem(v->showDirection?u8"朝向箭头：开":u8"朝向箭头：关",370)+menuItem(v->showRange?u8"攻击范围：开":u8"攻击范围：关",371)+menuItem(v->showTarget?u8"目标连线：开":u8"目标连线：关",373)+menuItem(u8"样式与位置预览",321)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101)+menuRow(u8"实际显示须开启总开关"));
 if(page==323){std::string rows=menuRow(u8"当前检测 Tank："+std::to_string(f->tanks.size()));for(const auto& o:f->tanks){auto label=o.action.rfind("attack-layer:",0)==0?u8"正在出拳":o.action.rfind("throw-animation",0)==0?u8"正在投石":o.action.rfind("movement-animation",0)==0?u8"移动":o.action=="unknown"?u8"动作尚未识别":u8"待机或其他动作";rows+=menuRow("Tank #"+std::to_string(o.handle&4095)+" / "+label);auto e=observed->find(o.handle);rows+=menuRow(u8"估算剩余："+number(e==observed->end()?std::optional<float>{}:e->second.remaining)+u8" 秒");if(o.target)rows+=menuRow(std::string(o.target->source=="network-enemy"?u8"敌对目标：":o.target->source=="network-look-at"?u8"注视目标（仅参考）：":u8"朝向推测：")+o.target->name);if(o.nearVerticalAim)rows+=menuRow(u8"竖直瞄准：暂不显示地面范围");}return menuPage(u8"Tank · 当前状态",rows+menuItem(u8"保存设置",101));}
 return menuPage(u8"Tank · 参数设置",menuItem(u8"位置前瞻："+std::to_string(cfg->horizonMs)+u8" 毫秒（切换）",333)+menuRow(u8"前摇："+number(p->windup)+u8" / 普通间隔："+number(p->interval))+menuRow(u8"手长参数："+number(p->range))+menuRow(u8"按游戏参数估算，不代表实际命中或安全窗口")+menuItem(u8"保存设置",101));
}
inline bool action(std::uintptr_t id){if(id!=330&&id!=331&&id!=333)return false;auto s=*std::atomic_load(&options);if(id==330){source_entities::resetRequested=true;s.enabled=!s.enabled;if(s.enabled){faulted=false;display::resumeRequested=true;}}if(id==331){s.draw=!s.draw;if(s.draw)display::resumeRequested=true;}if(id==333)s.horizonMs=s.horizonMs>=500?0:s.horizonMs+100;std::atomic_store(&options,std::make_shared<const Settings>(s));return true;}
inline void poll(std::uint64_t now){pollCvars(now,!faulted&&std::atomic_load(&options)->enabled);}
}
