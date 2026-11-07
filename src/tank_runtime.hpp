#pragma once
#include "tank_cvars.hpp"
#include "diagnostics.hpp"
#include "tank_sequence.hpp"
#include "tank_virtual_sequence.hpp"
#include "tank_sequence_fallback.hpp"
#include "tank_event_runtime.hpp"
#include "tank_observed_timer.hpp"
#include "tank_layer_attack.hpp"
#include "tank_visual_state.hpp"
#include "entity_menu.hpp"
#include <mutex>
namespace visual::tank {
inline std::atomic<bool> faulted{false};
inline std::atomic<bool> originalBackup{false}; // Session-only testing, never saved.
inline std::atomic<unsigned long> engineObservations{0},studioObservations{0},tankHeaders{0},renderableMatches{0};
inline std::shared_ptr<const Result> emptyResult=std::make_shared<const Result>();
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::shared_ptr<const Result> snapshot=std::make_shared<const Result>();
struct Animation{std::uint32_t epoch=0,handle=0;int sequence=-1;SequenceName names;std::string route,reason,model;std::map<int,SequenceName> layerNames;std::map<int,std::string> layerReasons;std::uint32_t header=0;int localSequenceCount=0;ModelIncludes includes{};ModelKey key{};WrapperProbe probe{};bool backupMode=false;std::set<int> backupLayers;std::uint32_t wrapper=0;std::uint64_t resolvedAt=0,lastProbe=0,generation=0,observedAt=0;};
inline auto emptyObserved=std::make_shared<const std::map<std::uint32_t,ObservedTimer>>();
inline std::shared_ptr<const std::map<std::uint32_t,ObservedTimer>> observedSnapshot=emptyObserved;
inline auto emptyLayers=std::make_shared<const std::map<std::uint32_t,LayerAttack>>();
inline std::shared_ptr<const std::map<std::uint32_t,LayerAttack>> layerSnapshot=emptyLayers;
inline std::mutex animationMutex;inline std::map<std::uint32_t,Animation> animations;inline std::map<std::uint32_t,RenderAnchor> renderAnchors;
// CPU metadata only, called from the already verified model callback. No D3D,
// SetupBones, simulation changes or additional engine method dispatch.
inline std::atomic<std::uint64_t> modelGeneration{0};
inline void observeRecord(std::uint32_t header,const Record& record,std::uint32_t epoch,const char* route,const Vec3& origin,std::uint32_t entity){
 if(!record.sequence)return;auto key=modelKey(source_entities::reader,header);if(!key)return;
 std::lock_guard<std::mutex> lock(animationMutex);auto now=GetTickCount64();RenderAnchor pose{epoch,record.handle,now,origin};if(record.networkOrigin&&validRenderAnchor(pose,epoch,record.handle,pose.tick,*record.networkOrigin)){if(renderAnchors.size()>16)renderAnchors.clear();renderAnchors[record.handle]=pose;}
 auto old=animations.find(record.handle);bool sameModel=old!=animations.end()&&old->second.epoch==epoch&&old->second.key==*key&&old->second.backupMode==originalBackup.load();
 std::set<int> sequences{*record.sequence};for(const auto& layer:record.animationLayers)sequences.insert(layer.sequence);
 if(sameModel)old->second.observedAt=now;
 if(sameModel&&old->second.sequence==*record.sequence){std::set<int> previous{old->second.sequence};for(const auto& l:old->second.layerReasons)previous.insert(l.first);bool complete=!old->second.names.label.empty();for(const auto& l:record.animationLayers)complete=complete&&old->second.layerNames.count(l.sequence);
  if(previous==sequences&&now>=old->second.resolvedAt&&now-old->second.resolvedAt<(complete?1000:100)){if(old->second.route.find(route)==std::string::npos)old->second.route+="+"+std::string(route);return;}
 }
 Animation next;next.epoch=epoch;next.handle=record.handle;next.sequence=*record.sequence;next.header=header;next.key=*key;next.backupMode=originalBackup.load();if(sameModel)next.probe=old->second.probe;next.localSequenceCount=key->count;next.model=key->name;next.route=route;next.resolvedAt=now;next.observedAt=now;next.generation=sameModel?old->second.generation:++modelGeneration;next.wrapper=sameModel?old->second.wrapper:0;next.lastProbe=sameModel?old->second.lastProbe:0;next.includes=modelIncludes(source_entities::reader,header);
 bool shared=key->includes>0;int probe=*record.sequence;for(auto seq:sequences)if(seq>=key->count){shared=true;probe=seq;break;}
 if(shared&&next.wrapper&&!studioWrapper(source_entities::reader,next.wrapper,header))next.wrapper=0;
 if(shared&&!next.wrapper&&(!next.lastProbe||now-next.lastProbe>=1000)){next.wrapper=findStudioWrapper(source_entities::reader,entity,header,probe,&next.probe);next.lastProbe=now;}
 auto resolve=[&](int seq,const char** why)->std::optional<SequenceName>{if(shared){auto mapped=virtualSequence(source_entities::reader,next.wrapper,header,seq,why);if(mapped){*why="resolved via validated virtual-model group cache";return mapped->names;}if(!next.wrapper)*why="model container not located";return {};}return sequenceName(source_entities::reader,header,seq,why);};
 const char* why="not attempted";auto main=resolve(next.sequence,&why);next.reason=why;if(main)next.names=*main;
 for(const auto& l:record.animationLayers){auto n=resolve(l.sequence,&why);if(!n&&next.backupMode){n=originalLayerBackup(*key,l.sequence);if(n){why="EXPERIMENTAL vanilla global-layer index backup; not descriptor-resolved";next.backupLayers.insert(l.sequence);}}next.layerReasons[l.sequence]=why;if(n)next.layerNames[l.sequence]=*n;}
 // Same address/checksum can still be reused by unusual model reloaders. A
 // periodic name refresh must invalidate onset history when semantics change.
 if(sameModel){bool changed=old->second.sequence==next.sequence&&!old->second.names.label.empty()&&!next.names.label.empty()&&(old->second.names.label!=next.names.label||old->second.names.activity!=next.names.activity);for(const auto& l:next.layerNames){auto before=old->second.layerNames.find(l.first);if(before!=old->second.layerNames.end()&&(before->second.label!=l.second.label||before->second.activity!=l.second.activity))changed=true;}if(changed)next.generation=++modelGeneration;}
 if(animations.size()>16)animations.clear();animations[record.handle]=std::move(next);
}
inline void observeModel(std::uint32_t header,int index,const Vec3& origin){
 if(faulted||!std::atomic_load(&options)->enabled)return;++engineObservations;auto frame=std::atomic_load(&source_entities::combatSnapshot);auto camera=std::atomic_load(&source_camera::snapshot);auto now=GetTickCount64();
 if(!camera->active||camera->epoch!=frame->epoch||now<frame->tick||now-frame->tick>250)return;
 for(const auto& r:frame->records)if(r.index==unsigned(index)&&tank(r)){
  if(!source_entities::list)break;auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_entities::list));auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);source_entities::wire::Entry entry;
  if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(entry),entry)&&entry.entity&&entry.serial==r.serial)observeRecord(header,r,frame->epoch,"engine",origin,entry.entity);break;
 }
}
// The fast model path can bypass DrawModelExecute. Observe the existing Studio
// hook too, even while stretching is off. Resolve only Tank renderables via the
// already-validated tiny getter and full entity serial. Never issue GPU commands.
inline void observeStudio(std::uint32_t header,std::uint32_t renderable,const Vec3& origin){
 if(faulted||!std::atomic_load(&options)->enabled)return;++studioObservations;
 auto camera=std::atomic_load(&source_camera::snapshot);auto frame=std::atomic_load(&source_entities::combatSnapshot);auto now=GetTickCount64();
 if(!camera->active||frame->epoch!=camera->epoch||now<frame->tick||now-frame->tick>250||!source_entities::list)return;
 if(!header||!renderable)return;
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
 if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(entry),entry)&&entry.entity==found->second.base&&entry.serial==r.serial){++tankHeaders;++renderableMatches;observeRecord(header,r,frame->epoch,"studio",origin,entry.entity);}

}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 auto eventCfg=std::atomic_load(&options);events::update(!faulted&&eventCfg->enabled&&eventCfg->logging,camera);
 if(faulted)return;
 static Predictor predictor;static std::shared_ptr<const Frame> previous;
 static std::uint32_t timerEpoch=0;static std::map<std::uint32_t,SwingStopwatch> timers;static std::map<std::uint32_t,LayerAttackDetector> layerDetectors;static std::map<std::uint32_t,std::uint64_t> generations;
 auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::combatSnapshot);
 if(!cfg->enabled||!camera.active){predictor.reset();previous.reset();timers.clear();layerDetectors.clear();generations.clear();std::atomic_store(&layerSnapshot,emptyLayers);if(std::atomic_load(&observedSnapshot)!=emptyObserved)std::atomic_store(&observedSnapshot,emptyObserved);if(std::atomic_load(&snapshot)!=emptyResult)std::atomic_store(&snapshot,emptyResult);return;}
 if(previous==frame||frame->epoch!=camera.epoch||camera.tick<frame->tick||camera.tick-frame->tick>250)return;previous=frame;
 auto out=predictor.update(*frame,*std::atomic_load(&parameters),*cfg);
 if(timerEpoch!=out.epoch){timers.clear();layerDetectors.clear();generations.clear();timerEpoch=out.epoch;}
 std::map<std::uint32_t,SwingStopwatch> nextTimers;std::map<std::uint32_t,ObservedTimer> readings;
 std::map<std::uint32_t,LayerAttackDetector> nextDetectors;std::map<std::uint32_t,LayerAttack> layerReadings;
 {std::lock_guard<std::mutex> lock(animationMutex);for(auto& o:out.tanks){auto a=animations.find(o.handle);std::optional<SequenceName> names;
  if(a!=animations.end()&&a->second.epoch==frame->epoch&&o.sequence&&a->second.sequence==*o.sequence&&!a->second.names.label.empty()){names=a->second.names;o.action=classify(*names)+": "+names->label;}
  if(a!=animations.end()&&a->second.epoch==frame->epoch&&generations[o.handle]!=a->second.generation){timers.erase(o.handle);layerDetectors.erase(o.handle);generations[o.handle]=a->second.generation;}
  auto timer=timers.find(o.handle);SwingStopwatch watch=timer==timers.end()?SwingStopwatch{}:timer->second;
  const Record* actor=nullptr;for(const auto& r:frame->records)if(r.handle==o.handle){actor=&r;break;}
  std::vector<LayerSample> layers;if(actor)for(const auto& l:actor->animationLayers){LayerSample sample{l.index,l.sequence,l.cycle,l.weight,{}};
   if(a!=animations.end()&&a->second.epoch==frame->epoch){auto n=a->second.layerNames.find(l.sequence);if(n!=a->second.layerNames.end())sample.names=n->second;}layers.push_back(std::move(sample));
  }
  auto oldDetector=layerDetectors.find(o.handle);auto detector=oldDetector==layerDetectors.end()?LayerAttackDetector{}:oldDetector->second;
  auto layer=detector.update(out.clock.time,actor&&actor->animationLayerReason==dango::animation::SampledReason,layers);
  layerReadings[o.handle]=layer;nextDetectors[o.handle]=std::move(detector);if(layer.active)o.action="attack-layer: "+layer.names.label;
  auto reading=watch.update(out.clock.time,out.tick,o.sequence.value_or(-1),o.cycle,names,out.parameters.interval,out.parameters.missInterval,false,layer.onset,out.parameters.windup);
  // Raw client deadlines are diagnostic evidence only; stale AI fields must not
  // be presented as zero cooldown. The visible value is observation-based.
  o.cooldown=reading.remaining;readings[o.handle]=reading;nextTimers[o.handle]=watch;
 }}
 for(auto i=generations.begin();i!=generations.end();)if(!nextDetectors.count(i->first))i=generations.erase(i);else ++i;
 layerDetectors=std::move(nextDetectors);std::atomic_store(&layerSnapshot,std::make_shared<const std::map<std::uint32_t,LayerAttack>>(std::move(layerReadings)));
 timers=std::move(nextTimers);std::atomic_store(&observedSnapshot,std::make_shared<const std::map<std::uint32_t,ObservedTimer>>(std::move(readings)));
 std::atomic_store(&snapshot,std::make_shared<const Result>(std::move(out)));
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
inline std::string menu(std::uintptr_t page=320){auto cfg=std::atomic_load(&options);auto f=std::atomic_load(&snapshot);auto p=std::atomic_load(&parameters);auto v=std::atomic_load(&visualSettings);auto observed=std::atomic_load(&observedSnapshot);
 auto number=[](std::optional<float> n){return n?std::to_string(int(*n*10)/10.f):std::string(u8"未知");};
 auto displayLabel=!cfg->enabled?u8"显示内容（待开启总开关）":!cfg->draw?u8"显示内容（画面关）":(!v->showBar&&!v->showRange&&!v->showDirection&&!v->showTarget)?u8"显示内容（全部关闭）":u8"显示内容（画面开）";
 if(page==320)return menuPage(u8"Tank 预测",
  menuItem(cfg->enabled?u8"总开关：开":u8"总开关：关（默认）",330)+menuItem(displayLabel,322)+menuItem(u8"样式与位置预览",321)+menuItem(u8"当前 Tank 状态",323)+menuItem(u8"动画读取状态",328)+menuItem(u8"高级与诊断",324)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101)+menuRow(u8"当前检测 Tank："+std::to_string(f->tanks.size()))+menuRow(u8"计时为估算，不代表真实命中边界"));
 if(page==322)return menuPage(u8"Tank · 显示内容",
  menuItem(cfg->draw?u8"画面显示：开":u8"画面显示：关",331)+menuItem(v->showBar?u8"攻击计时条：开":u8"攻击计时条：关",372)+menuItem(v->showDirection?u8"朝向箭头：开":u8"朝向箭头：关",370)+menuItem(v->showRange?u8"攻击范围：开":u8"攻击范围：关",371)+menuItem(v->showTarget?u8"目标连线：开":u8"目标连线：关",373)+menuItem(u8"样式与位置预览",321)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101)+menuRow(u8"实际显示须开启总开关；不必开启诊断"));
 if(page==328){
  std::string rows=menuItem(u8"刷新动画状态",328)+menuRow(cfg->enabled?u8"动画采集：开":u8"动画采集：关；先开启 Tank 总开关");
  rows+=menuItem(originalBackup.load()?u8"原版序号备用识别：开（实验）":u8"原版序号备用识别：关（默认）",334);
  rows+=menuRow(u8"备用仅本次运行生效；序号推测不等于动作解析");
  rows+=menuRow(u8"此页无需画面显示或诊断记录；重新进入刷新");
  if(!cfg->enabled)return menuPage(u8"Tank · 动画读取状态",rows);
  auto frame=std::atomic_load(&source_entities::combatSnapshot);auto now=GetTickCount64();
  auto layers=std::atomic_load(&layerSnapshot);std::lock_guard<std::mutex> lock(animationMutex);unsigned count=0;
  for(const auto& actor:frame->records)if(tank(actor)){
   ++count;rows+=menuRow("Tank #"+std::to_string(actor.handle&4095));
   auto a=animations.find(actor.handle);
   bool fresh=a!=animations.end()&&a->second.epoch==frame->epoch&&now>=a->second.observedAt&&now-a->second.observedAt<=1500;
   if(!fresh){rows+=menuRow(u8"模型回调：未读取或已过期；让 Tank 进入视野");continue;}
   const auto& state=a->second;if(originalBackup.load())rows+=menuRow(originalBackupCompatible(state.key)?u8"备用模型校验：匹配":u8"备用模型校验：不匹配（不使用）");rows+=menuRow(u8"模型回调：已读取 / "+state.route);
   rows+=menuRow(u8"模型校验值："+std::to_string(state.key.checksum)+u8" / 局部序列数："+std::to_string(state.key.count));
   rows+=menuRow(state.wrapper?u8"共享容器：已定位 / 实体偏移 "+std::to_string(state.probe.offset):state.key.includes?u8"共享容器：未定位":u8"共享容器：不需要");
   if(state.key.includes&&!state.wrapper)rows+=menuRow(u8"扫描候选："+std::to_string(state.probe.candidates)+u8" / 根模型匹配："+std::to_string(state.probe.rootMatches));
   rows+=menuRow(u8"主动作解析："+std::string(state.names.label.empty()?u8"未读取":state.names.label)+" / #"+std::to_string(state.sequence));
   rows+=menuRow(state.reason);
   if(state.key.includes&&!state.wrapper)rows+=menuRow(state.probe.reason);
   bool sampled=actor.animationLayerReason==dango::animation::SampledReason;
   rows+=menuRow(sampled?u8"动画层数据：已读取":u8"动画层数据：未读取");unsigned active=0;
   for(const auto& l:actor.animationLayers)if(l.weight>.05f){++active;auto n=state.layerNames.find(l.sequence);bool backup=state.backupLayers.count(l.sequence);
    rows+=menuRow(u8"活动层 #"+std::to_string(l.index)+" / seq "+std::to_string(l.sequence)+" / "+(backup?u8"备用推测":n!=state.layerNames.end()?u8"已解析":u8"未解析"));
    if(n!=state.layerNames.end())rows+=menuRow(n->second.label+" / "+n->second.activity);
    else {auto reason=state.layerReasons.find(l.sequence);if(reason!=state.layerReasons.end())rows+=menuRow(reason->second);}
   }
   if(!active)rows+=menuRow(u8"当前没有活动动画层（不代表读取失败）");
   auto layer=layers->find(actor.handle);rows+=menuRow(layer!=layers->end()&&layer->second.active?(layer->second.names.label.rfind("experimental_",0)==0?u8"当前攻击识别：是（实验备用）":u8"当前攻击识别：是"):u8"当前攻击识别：否");
   auto timer=observed->find(actor.handle);rows+=menuRow(u8"本轮攻击计时编号："+std::to_string(timer==observed->end()?0:timer->second.eventId));
  }
  if(!count)rows+=menuRow(u8"当前没有检测到 Tank");
  return menuPage(u8"Tank · 动画读取状态",rows);
 }
 if(page==323){std::string rows=menuRow(u8"当前检测 Tank："+std::to_string(f->tanks.size()));for(const auto& o:f->tanks){auto label=o.action.rfind("attack-layer:",0)==0?u8"正在出拳":o.action.rfind("throw-animation",0)==0?u8"正在投石":o.action.rfind("movement-animation",0)==0?u8"移动":o.action=="unknown"?u8"动作尚未识别":u8"待机或其他动作";rows+=menuRow("Tank #"+std::to_string(o.handle&4095)+" / "+label);auto e=observed->find(o.handle);rows+=menuRow(u8"估算剩余："+number(e==observed->end()?std::optional<float>{}:e->second.remaining)+u8" 秒");if(o.target)rows+=menuRow(std::string(o.target->source=="network-enemy"?u8"敌对目标：":o.target->source=="network-look-at"?u8"注视目标（仅参考）：":u8"朝向推测：")+o.target->name);if(o.nearVerticalAim)rows+=menuRow(u8"竖直瞄准：暂不显示地面范围");}return menuPage(u8"Tank · 当前状态",rows+menuItem(u8"保存设置",101));}
 return menuPage(u8"Tank · 高级与诊断",menuItem(u8"动画读取状态",328)+menuItem(cfg->logging?u8"诊断记录：开":u8"诊断记录：关（默认）",332)+menuItem(u8"位置前瞻："+std::to_string(cfg->horizonMs)+u8" 毫秒（切换）",333)+menuRow(u8"前摇："+number(p->windup)+u8" / 普通间隔："+number(p->interval))+menuRow(u8"手长参数："+number(p->range))+menuRow(p->source)+menuRow(display::gateReason.load())+menuRow(display::stage.load())+menuRow(u8"诊断仅用于反馈，不必为显示开启")+menuItem(u8"保存设置",101));
}
inline bool action(std::uintptr_t id){if(id==334){originalBackup=!originalBackup.load();return true;}if(id<330||id>333)return false;auto s=*std::atomic_load(&options);if(id==330){source_entities::resetRequested=true;s.enabled=!s.enabled;if(s.enabled){faulted=false;display::resumeRequested=true;}}if(id==331){s.draw=!s.draw;if(s.draw)display::resumeRequested=true;}if(id==332)s.logging=!s.logging;if(id==333)s.horizonMs=s.horizonMs>=500?0:s.horizonMs+100;std::atomic_store(&options,std::make_shared<const Settings>(s));return true;}
inline void poll(std::uint64_t now)try{events::poll();if(faulted)return;auto cfg=std::atomic_load(&options);pollCvars(now,cfg->enabled);static std::uint64_t next=0,last=0;
 if(!cfg->enabled||!cfg->logging||now<next)return;next=now+100;auto f=std::atomic_load(&snapshot);if(!f->tick||f->tick==last)return;last=f->tick;
 nlohmann::json j={{"schema",10},{"epoch",f->epoch},{"tick",f->tick},{"parameterSource",f->parameters.source},{"tanks",nlohmann::json::array()}};
 j["eventCapture"]=events::summary();
 j["display"]={{"enabled",cfg->enabled},{"drawing",cfg->draw},{"paused",display::paused.load()},{"foreground",display::foreground.load()},{"gate",display::gateReason.load()},{"stage",display::stage.load()},{"calls",display::calls.load()},{"submitted",display::submitted.load()},{"barProjected",display::barProjected.load()},{"barRejected",display::barRejected.load()},{"visibleBars",display::visibleBars.load()},{"renderAnchorHits",display::renderAnchorHits.load()},{"networkAnchorFallbacks",display::networkAnchorFallbacks.load()},{"tick",display::tick.load()},{"result",display::result.load()}};writeVisual(j,*std::atomic_load(&visualSettings));auto displayCamera=std::atomic_load(&source_camera::snapshot);j["display"]["cameraEpoch"]=displayCamera->epoch;j["display"]["cameraTick"]=displayCamera->tick;j["display"]["worldToClip"]=displayCamera->worldToClip;j["display"]["barWorldUnit"]=BarWorldUnit;j["display"]["viewportWidth"]=display::viewportWidth.load();j["display"]["viewportHeight"]=display::viewportHeight.load();
 j["observationDiagnostics"]={{"engine",engineObservations.load()},{"studio",studioObservations.load()},{"tankHeaders",tankHeaders.load()},{"renderableMatches",renderableMatches.load()}};
 j["clockSourceHandle"]=f->clock.source;j["clockReason"]=f->clock.reason;if(f->clock.time)j["observedGameTime"]=*f->clock.time;
 if(f->parameters.range)j["range"]=*f->parameters.range;if(f->parameters.arc)j["arc"]=*f->parameters.arc;if(f->parameters.windup)j["windup"]=*f->parameters.windup;if(f->parameters.duration)j["duration"]=*f->parameters.duration;if(f->parameters.interval)j["interval"]=*f->parameters.interval;if(f->parameters.missInterval)j["missInterval"]=*f->parameters.missInterval;
 for(const auto& o:f->tanks){nlohmann::json r={{"handle",o.handle},{"position",{o.origin.x,o.origin.y,o.origin.z}},{"yaw",o.heading},{"pitch",o.pitch},{"nearVerticalAim",o.nearVerticalAim},{"eyeAngle",o.eye},{"headingRate",o.headingRate},{"possibleHeadingLock",o.headingStalled},{"action",o.action},{"geometry",o.geometry},{"reason",o.reason}};
  {std::lock_guard<std::mutex> lock(animationMutex);auto probe=animations.find(o.handle);
   if(probe!=animations.end()&&probe->second.epoch==f->epoch){r["animationProbe"]={{"route",probe->second.route},{"reason",probe->second.reason},{"model",probe->second.model},{"sequence",probe->second.sequence},{"localSequenceCount",probe->second.localSequenceCount},{"modelVersion",probe->second.key.version},{"modelChecksum",probe->second.key.checksum},{"generation",probe->second.generation},{"virtualMappingAvailable",probe->second.wrapper!=0},{"containerOffset",probe->second.probe.offset},{"containerProbe",probe->second.probe.reason},{"containerRootMatches",probe->second.probe.rootMatches},{"containerCandidates",probe->second.probe.candidates},{"containerScanBytes",probe->second.probe.scanned},{"originalBackupEnabled",probe->second.backupMode},{"backupLayerCount",probe->second.backupLayers.size()},{"includedModels",probe->second.includes.names},{"includeReason",probe->second.includes.reason},{"label",probe->second.names.label},{"activity",probe->second.names.activity},{"activityId",probe->second.names.activityId}};}
   else r["animationProbe"]={{"reason","no matching model observation"}};}
  auto detectedLayers=std::atomic_load(&layerSnapshot);auto detected=detectedLayers->find(o.handle);if(detected!=detectedLayers->end()){const auto& l=detected->second;r["attackLayer"]={{"sampled",l.sampled},{"experimentalBackup",l.names.label.rfind("experimental_",0)==0},{"active",l.active},{"onset",l.onset},{"index",l.layer},{"sequence",l.sequence},{"cycle",l.cycle},{"weight",l.weight},{"label",l.names.label},{"activity",l.names.activity}};}
  r["rawFieldCooldownReason"]=o.timing.reason;r["rawFieldPartial"]=o.timing.partial;if(o.timing.weaponDeadline)r["rawWeaponDeadline"]=*o.timing.weaponDeadline;if(o.timing.actorDeadline)r["rawActorDeadline"]=*o.timing.actorDeadline;
  auto estimates=std::atomic_load(&observedSnapshot);auto estimate=estimates->find(o.handle);if(estimate!=estimates->end()){const auto& e=estimate->second;r["stopwatch"]={{"eventId",e.eventId},{"source",e.source},{"reason",e.reason},{"triggered",e.triggered},{"expired",e.expired},{"throwing",e.throwing}};if(e.startedAt)r["stopwatch"]["startedAt"]=*e.startedAt;if(e.span)r["stopwatch"]["span"]=*e.span;if(e.sinceThrow)r["stopwatch"]["sinceThrowEnd"]=*e.sinceThrow;}
  if(o.cooldown)r["estimatedCooldown"]=*o.cooldown;if(o.sequence)r["sequence"]=*o.sequence;if(o.cycle)r["cycle"]=*o.cycle;if(o.target)r["target"]={{"handle",o.target->handle},{"name",o.target->name},{"source",o.target->source}};j["tanks"].push_back(r);
 }
 auto actors=std::atomic_load(&source_entities::combatSnapshot);j["actors"]=nlohmann::json::array();j["actorTick"]=actors->tick;j["resultTick"]=f->tick;j["fieldSchemaHints"]=actors->getterHints;j["animationEventSchemas"]=actors->animationEventSchemas;
 if(actors->epoch==f->epoch)for(const auto& a:actors->records){nlohmann::json r={{"handle",a.handle},{"class",a.className}};
  auto vector=[&](const char* key,const std::optional<Vec3>& v){if(v)r[key]={v->x,v->y,v->z};};vector("position",a.networkOrigin);vector("bodyAngles",a.networkAngles);vector("eyeAngles",a.eyeAngles);vector("collisionMins",a.collisionMins);vector("collisionMaxs",a.collisionMaxs);
  if(!a.animationLayerReason.empty()){r["animationLayerReason"]=a.animationLayerReason;r["animationLayers"]=nlohmann::json::array();std::lock_guard<std::mutex> lock(animationMutex);auto probe=animations.find(a.handle);for(const auto& layer:a.animationLayers){nlohmann::json l={{"index",layer.index},{"sequence",layer.sequence},{"cycle",layer.cycle},{"previousCycle",layer.previousCycle},{"weight",layer.weight},{"order",layer.order}};if(probe!=animations.end()&&probe->second.epoch==actors->epoch){auto n=probe->second.layerNames.find(layer.sequence);if(n!=probe->second.layerNames.end()){l["label"]=n->second.label;l["activity"]=n->second.activity;l["activityId"]=n->second.activityId;l["experimentalBackup"]=probe->second.backupLayers.count(layer.sequence)!=0;}auto why=probe->second.layerReasons.find(layer.sequence);if(why!=probe->second.layerReasons.end())l["resolution"]=why->second;}r["animationLayers"].push_back(l);}}
  if(a.weaponOwner)r["weaponOwner"]=*a.weaponOwner;if(a.owner)r["owner"]=*a.owner;if(a.animationCycle)r["animationCycle"]=*a.animationCycle;
  if(a.health)r["health"]=*a.health;if(a.lookAtHandle)r["lookAtHandle"]=*a.lookAtHandle;if(a.enemyHandle)r["enemyHandle"]=*a.enemyHandle;if(a.activeWeapon)r["activeWeapon"]=*a.activeWeapon;if(a.nextPrimaryAttack)r["nextPrimaryAttack"]=*a.nextPrimaryAttack;if(a.nextAttack)r["nextAttack"]=*a.nextAttack;if(a.simulationTime)r["simulationTime"]=*a.simulationTime;j["actors"].push_back(r);
 }
 auto path=visual::diagnostics::path.parent_path()/L"tank-prediction.jsonl";std::error_code e;auto size=std::filesystem::file_size(path,e);if(!e&&size>=4u*1024u*1024u){auto old=path;old.replace_filename(L"tank-prediction.previous.jsonl");if(!MoveFileExW(path.c_str(),old.c_str(),MOVEFILE_REPLACE_EXISTING))return;}
 std::ofstream file(path,std::ios::app|std::ios::binary);file<<j.dump(-1,' ',false,nlohmann::json::error_handler_t::replace)<<'\n';
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
}
