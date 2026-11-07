#pragma once
#include "tank_cvars.hpp"
#include "diagnostics.hpp"
#include "tank_sequence.hpp"
#include "tank_event_runtime.hpp"
#include "tank_observed_timer.hpp"
#include "tank_layer_attack.hpp"
#include "tank_visual_state.hpp"
#include "entity_menu.hpp"
#include <mutex>
namespace visual::tank {
inline std::atomic<bool> faulted{false};
inline std::atomic<unsigned long> engineObservations{0},studioObservations{0},tankHeaders{0},renderableMatches{0};
inline std::shared_ptr<const Result> emptyResult=std::make_shared<const Result>();
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::shared_ptr<const Result> snapshot=std::make_shared<const Result>();
struct Animation{std::uint32_t epoch=0,handle=0;int sequence=-1;SequenceName names;std::string route,reason,model;std::map<int,SequenceName> layerNames;std::map<int,std::string> layerReasons;std::uint32_t header=0;int localSequenceCount=0;ModelIncludes includes{};};
inline auto emptyObserved=std::make_shared<const std::map<std::uint32_t,ObservedTimer>>();
inline std::shared_ptr<const std::map<std::uint32_t,ObservedTimer>> observedSnapshot=emptyObserved;
inline auto emptyLayers=std::make_shared<const std::map<std::uint32_t,LayerAttack>>();
inline std::shared_ptr<const std::map<std::uint32_t,LayerAttack>> layerSnapshot=emptyLayers;
inline std::mutex animationMutex;inline std::map<std::uint32_t,Animation> animations;inline std::map<std::uint32_t,RenderAnchor> renderAnchors;
// CPU metadata only, called from the already verified model callback. No D3D,
// SetupBones, simulation changes or additional engine method dispatch.
inline bool tankModel(std::uint32_t header){
 static thread_local std::uint32_t epoch=0;static thread_local std::map<std::uint32_t,bool> known;
 auto camera=std::atomic_load(&source_camera::snapshot);if(epoch!=camera->epoch){known.clear();epoch=camera->epoch;}
 auto i=known.find(header);if(i!=known.end())return i->second;
 auto name=dango::entitywire::text(source_entities::reader,header+12,64);for(char& c:name)if(c>='A'&&c<='Z')c=char(c+32);
 bool accepted=name.find("hulk")!=std::string::npos||name.find("tank")!=std::string::npos;if(known.size()>=512)known.clear();known[header]=accepted;return accepted;
}
inline void observeRecord(std::uint32_t header,const Record& record,std::uint32_t epoch,const char* route,const Vec3& origin){
 if(!record.sequence)return;std::lock_guard<std::mutex> lock(animationMutex);RenderAnchor pose{epoch,record.handle,GetTickCount64(),origin};if(record.networkOrigin&&validRenderAnchor(pose,epoch,record.handle,pose.tick,*record.networkOrigin)){if(renderAnchors.size()>16)renderAnchors.clear();renderAnchors[record.handle]=pose;}auto old=animations.find(record.handle);
 std::string routes=route;
 if(old!=animations.end()&&old->second.epoch==epoch&&old->second.sequence==*record.sequence&&old->second.header==header){
  std::set<int> unique;for(const auto& layer:record.animationLayers)unique.insert(layer.sequence);bool sameLayers=old->second.layerReasons.size()==unique.size();for(const auto& layer:record.animationLayers)if(!old->second.layerReasons.count(layer.sequence))sameLayers=false;
  if(old->second.route.find(route)!=std::string::npos&&sameLayers)return;
  routes=old->second.route;if(routes.find(route)==std::string::npos)routes+="+"+std::string(route);if(!old->second.names.label.empty()&&sameLayers){old->second.route=routes;return;}
 }
 if(animations.size()>16)animations.clear();const char* reason="not attempted";auto name=sequenceName(source_entities::reader,header,*record.sequence,&reason);
 animations[record.handle]={epoch,record.handle,*record.sequence,name?*name:SequenceName{},routes,reason,dango::entitywire::text(source_entities::reader,header+12,64),{}, {}};
 auto& current=animations[record.handle];current.header=header;current.includes=modelIncludes(source_entities::reader,header);dango::entitywire::load(source_entities::reader,header+188,current.localSequenceCount);for(const auto& layer:record.animationLayers){const char* why="not attempted";auto n=sequenceName(source_entities::reader,header,layer.sequence,&why);current.layerReasons[layer.sequence]=why;if(n)current.layerNames[layer.sequence]=*n;}
}
inline void observeModel(std::uint32_t header,int index,const Vec3& origin){
 if(faulted||!std::atomic_load(&options)->enabled)return;++engineObservations;auto frame=std::atomic_load(&source_entities::combatSnapshot);auto camera=std::atomic_load(&source_camera::snapshot);auto now=GetTickCount64();
 if(!camera->active||camera->epoch!=frame->epoch||now<frame->tick||now-frame->tick>250)return;
 for(const auto& r:frame->records)if(r.index==unsigned(index)&&tank(r)&&tankModel(header)){
  if(!source_entities::list)break;auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_entities::list));auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);source_entities::wire::Entry entry;
  if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(entry),entry)&&entry.entity&&entry.serial==r.serial)observeRecord(header,r,frame->epoch,"engine",origin);break;
 }
}
// The fast model path can bypass DrawModelExecute. Observe the existing Studio
// hook too, even while stretching is off. Resolve only Tank renderables via the
// already-validated tiny getter and full entity serial. Never issue GPU commands.
inline void observeStudio(std::uint32_t header,std::uint32_t renderable,const Vec3& origin){
 if(faulted||!std::atomic_load(&options)->enabled)return;++studioObservations;
 auto camera=std::atomic_load(&source_camera::snapshot);auto frame=std::atomic_load(&source_entities::combatSnapshot);auto now=GetTickCount64();
 if(!camera->active||frame->epoch!=camera->epoch||now<frame->tick||now-frame->tick>250||!source_entities::list)return;
 if(!header||!renderable||!tankModel(header))return;++tankHeaders;
 std::uint32_t getter=0;if(!source_entities::method(renderable,0,getter))return;
 auto base=source_entities::wire::addressGetter(source_entities::reader,getter,renderable);if(!base)return;
 auto object=std::uint32_t(reinterpret_cast<std::uintptr_t>(source_entities::list));
 auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 for(const auto& r:frame->records)if(tank(r)){
  source_entities::wire::Entry entry;if(source_entities::wire::load(source_entities::reader,entries+r.index*sizeof(entry),entry)&&entry.entity==*base&&entry.serial==r.serial){++renderableMatches;observeRecord(header,r,frame->epoch,"studio",origin);break;}
 }
}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 auto eventCfg=std::atomic_load(&options);events::update(!faulted&&eventCfg->enabled&&eventCfg->logging,camera);
 if(faulted)return;
 static Predictor predictor;static std::shared_ptr<const Frame> previous;
 static std::uint32_t timerEpoch=0;static std::map<std::uint32_t,SwingStopwatch> timers;static std::map<std::uint32_t,LayerAttackDetector> layerDetectors;
 auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::combatSnapshot);
 if(!cfg->enabled||!camera.active){predictor.reset();previous.reset();timers.clear();layerDetectors.clear();std::atomic_store(&layerSnapshot,emptyLayers);if(std::atomic_load(&observedSnapshot)!=emptyObserved)std::atomic_store(&observedSnapshot,emptyObserved);if(std::atomic_load(&snapshot)!=emptyResult)std::atomic_store(&snapshot,emptyResult);return;}
 if(previous==frame||frame->epoch!=camera.epoch||camera.tick<frame->tick||camera.tick-frame->tick>250)return;previous=frame;
 auto out=predictor.update(*frame,*std::atomic_load(&parameters),*cfg);
 if(timerEpoch!=out.epoch){timers.clear();layerDetectors.clear();timerEpoch=out.epoch;}
 std::map<std::uint32_t,SwingStopwatch> nextTimers;std::map<std::uint32_t,ObservedTimer> readings;
 std::map<std::uint32_t,LayerAttackDetector> nextDetectors;std::map<std::uint32_t,LayerAttack> layerReadings;
 {std::lock_guard<std::mutex> lock(animationMutex);for(auto& o:out.tanks){auto a=animations.find(o.handle);std::optional<SequenceName> names;
  if(a!=animations.end()&&a->second.epoch==frame->epoch&&o.sequence&&a->second.sequence==*o.sequence&&!a->second.names.label.empty()){names=a->second.names;o.action=classify(*names)+": "+names->label;}
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
 layerDetectors=std::move(nextDetectors);std::atomic_store(&layerSnapshot,std::make_shared<const std::map<std::uint32_t,LayerAttack>>(std::move(layerReadings)));
 timers=std::move(nextTimers);std::atomic_store(&observedSnapshot,std::make_shared<const std::map<std::uint32_t,ObservedTimer>>(std::move(readings)));
 std::atomic_store(&snapshot,std::make_shared<const Result>(std::move(out)));
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
inline std::string menu(std::uintptr_t page=320){auto cfg=std::atomic_load(&options);auto f=std::atomic_load(&snapshot);auto p=std::atomic_load(&parameters);auto v=std::atomic_load(&visualSettings);auto observed=std::atomic_load(&observedSnapshot);
 auto number=[](std::optional<float> n){return n?std::to_string(int(*n*10)/10.f):std::string(u8"未知");};
 auto displayLabel=!cfg->enabled?u8"显示内容（待开启总开关）":!cfg->draw?u8"显示内容（画面关）":(!v->showBar&&!v->showRange&&!v->showDirection&&!v->showTarget)?u8"显示内容（全部关闭）":u8"显示内容（画面开）";
 if(page==320)return menuPage(u8"Tank 预测",
  menuItem(cfg->enabled?u8"总开关：开":u8"总开关：关（默认）",330)+menuItem(displayLabel,322)+menuItem(u8"样式与位置预览",321)+menuItem(u8"当前 Tank 状态",323)+menuItem(u8"高级与诊断",324)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101)+menuRow(u8"当前检测 Tank："+std::to_string(f->tanks.size()))+menuRow(u8"计时为估算，不代表真实命中边界"));
 if(page==322)return menuPage(u8"Tank · 显示内容",
  menuItem(cfg->draw?u8"画面显示：开":u8"画面显示：关",331)+menuItem(v->showBar?u8"攻击计时条：开":u8"攻击计时条：关",372)+menuItem(v->showDirection?u8"朝向箭头：开":u8"朝向箭头：关",370)+menuItem(v->showRange?u8"攻击范围：开":u8"攻击范围：关",371)+menuItem(v->showTarget?u8"目标连线：开":u8"目标连线：关",373)+menuItem(u8"样式与位置预览",321)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101)+menuRow(u8"实际显示须开启总开关；不必开启诊断"));
 if(page==323){std::string rows=menuRow(u8"当前检测 Tank："+std::to_string(f->tanks.size()));for(const auto& o:f->tanks){auto label=o.action.rfind("attack-layer:",0)==0?u8"正在出拳":o.action.rfind("throw-animation",0)==0?u8"正在投石":o.action.rfind("movement-animation",0)==0?u8"移动":o.action=="unknown"?u8"动作尚未识别":u8"待机或其他动作";rows+=menuRow("Tank #"+std::to_string(o.handle&4095)+" / "+label);auto e=observed->find(o.handle);rows+=menuRow(u8"估算剩余："+number(e==observed->end()?std::optional<float>{}:e->second.remaining)+u8" 秒");if(o.target)rows+=menuRow(std::string(o.target->source=="network-enemy"?u8"敌对目标：":o.target->source=="network-look-at"?u8"注视目标（仅参考）：":u8"朝向推测：")+o.target->name);if(o.nearVerticalAim)rows+=menuRow(u8"竖直瞄准：暂不显示地面范围");}return menuPage(u8"Tank · 当前状态",rows+menuItem(u8"保存设置",101));}
 return menuPage(u8"Tank · 高级与诊断",menuItem(cfg->logging?u8"诊断记录：开":u8"诊断记录：关（默认）",332)+menuItem(u8"位置前瞻："+std::to_string(cfg->horizonMs)+u8" 毫秒（切换）",333)+menuRow(u8"前摇："+number(p->windup)+u8" / 普通间隔："+number(p->interval))+menuRow(u8"手长参数："+number(p->range))+menuRow(p->source)+menuRow(display::gateReason.load())+menuRow(display::stage.load())+menuRow(u8"诊断仅用于反馈，不必为显示开启")+menuItem(u8"保存设置",101));
}
inline bool action(std::uintptr_t id){if(id<330||id>333)return false;auto s=*std::atomic_load(&options);if(id==330){source_entities::resetRequested=true;s.enabled=!s.enabled;if(s.enabled){faulted=false;display::resumeRequested=true;}}if(id==331){s.draw=!s.draw;if(s.draw)display::resumeRequested=true;}if(id==332)s.logging=!s.logging;if(id==333)s.horizonMs=s.horizonMs>=500?0:s.horizonMs+100;std::atomic_store(&options,std::make_shared<const Settings>(s));return true;}
inline void poll(std::uint64_t now)try{events::poll();if(faulted)return;auto cfg=std::atomic_load(&options);pollCvars(now,cfg->enabled);static std::uint64_t next=0,last=0;
 if(!cfg->enabled||!cfg->logging||now<next)return;next=now+100;auto f=std::atomic_load(&snapshot);if(!f->tick||f->tick==last)return;last=f->tick;
 nlohmann::json j={{"schema",9},{"epoch",f->epoch},{"tick",f->tick},{"parameterSource",f->parameters.source},{"tanks",nlohmann::json::array()}};
 j["eventCapture"]=events::summary();
 j["display"]={{"enabled",cfg->enabled},{"drawing",cfg->draw},{"paused",display::paused.load()},{"foreground",display::foreground.load()},{"gate",display::gateReason.load()},{"stage",display::stage.load()},{"calls",display::calls.load()},{"submitted",display::submitted.load()},{"barProjected",display::barProjected.load()},{"barRejected",display::barRejected.load()},{"visibleBars",display::visibleBars.load()},{"renderAnchorHits",display::renderAnchorHits.load()},{"networkAnchorFallbacks",display::networkAnchorFallbacks.load()},{"tick",display::tick.load()},{"result",display::result.load()}};writeVisual(j,*std::atomic_load(&visualSettings));auto displayCamera=std::atomic_load(&source_camera::snapshot);j["display"]["cameraEpoch"]=displayCamera->epoch;j["display"]["cameraTick"]=displayCamera->tick;j["display"]["worldToClip"]=displayCamera->worldToClip;j["display"]["barWorldUnit"]=BarWorldUnit;j["display"]["viewportWidth"]=display::viewportWidth.load();j["display"]["viewportHeight"]=display::viewportHeight.load();
 j["observationDiagnostics"]={{"engine",engineObservations.load()},{"studio",studioObservations.load()},{"tankHeaders",tankHeaders.load()},{"renderableMatches",renderableMatches.load()}};
 j["clockSourceHandle"]=f->clock.source;j["clockReason"]=f->clock.reason;if(f->clock.time)j["observedGameTime"]=*f->clock.time;
 if(f->parameters.range)j["range"]=*f->parameters.range;if(f->parameters.arc)j["arc"]=*f->parameters.arc;if(f->parameters.windup)j["windup"]=*f->parameters.windup;if(f->parameters.duration)j["duration"]=*f->parameters.duration;if(f->parameters.interval)j["interval"]=*f->parameters.interval;if(f->parameters.missInterval)j["missInterval"]=*f->parameters.missInterval;
 for(const auto& o:f->tanks){nlohmann::json r={{"handle",o.handle},{"position",{o.origin.x,o.origin.y,o.origin.z}},{"yaw",o.heading},{"pitch",o.pitch},{"nearVerticalAim",o.nearVerticalAim},{"eyeAngle",o.eye},{"headingRate",o.headingRate},{"possibleHeadingLock",o.headingStalled},{"action",o.action},{"geometry",o.geometry},{"reason",o.reason}};
  {std::lock_guard<std::mutex> lock(animationMutex);auto probe=animations.find(o.handle);
   if(probe!=animations.end()&&probe->second.epoch==f->epoch){r["animationProbe"]={{"route",probe->second.route},{"reason",probe->second.reason},{"model",probe->second.model},{"sequence",probe->second.sequence},{"localSequenceCount",probe->second.localSequenceCount},{"includedModels",probe->second.includes.names},{"includeReason",probe->second.includes.reason},{"label",probe->second.names.label},{"activity",probe->second.names.activity}};}
   else r["animationProbe"]={{"reason","no matching model observation"}};}
  auto detectedLayers=std::atomic_load(&layerSnapshot);auto detected=detectedLayers->find(o.handle);if(detected!=detectedLayers->end()){const auto& l=detected->second;r["attackLayer"]={{"sampled",l.sampled},{"active",l.active},{"onset",l.onset},{"index",l.layer},{"sequence",l.sequence},{"cycle",l.cycle},{"weight",l.weight},{"label",l.names.label},{"activity",l.names.activity}};}
  r["rawFieldCooldownReason"]=o.timing.reason;r["rawFieldPartial"]=o.timing.partial;if(o.timing.weaponDeadline)r["rawWeaponDeadline"]=*o.timing.weaponDeadline;if(o.timing.actorDeadline)r["rawActorDeadline"]=*o.timing.actorDeadline;
  auto estimates=std::atomic_load(&observedSnapshot);auto estimate=estimates->find(o.handle);if(estimate!=estimates->end()){const auto& e=estimate->second;r["stopwatch"]={{"eventId",e.eventId},{"source",e.source},{"reason",e.reason},{"triggered",e.triggered},{"expired",e.expired},{"throwing",e.throwing}};if(e.startedAt)r["stopwatch"]["startedAt"]=*e.startedAt;if(e.span)r["stopwatch"]["span"]=*e.span;if(e.sinceThrow)r["stopwatch"]["sinceThrowEnd"]=*e.sinceThrow;}
  if(o.cooldown)r["estimatedCooldown"]=*o.cooldown;if(o.sequence)r["sequence"]=*o.sequence;if(o.cycle)r["cycle"]=*o.cycle;if(o.target)r["target"]={{"handle",o.target->handle},{"name",o.target->name},{"source",o.target->source}};j["tanks"].push_back(r);
 }
 auto actors=std::atomic_load(&source_entities::combatSnapshot);j["actors"]=nlohmann::json::array();j["actorTick"]=actors->tick;j["resultTick"]=f->tick;j["fieldSchemaHints"]=actors->getterHints;j["animationEventSchemas"]=actors->animationEventSchemas;
 if(actors->epoch==f->epoch)for(const auto& a:actors->records){nlohmann::json r={{"handle",a.handle},{"class",a.className}};
  auto vector=[&](const char* key,const std::optional<Vec3>& v){if(v)r[key]={v->x,v->y,v->z};};vector("position",a.networkOrigin);vector("bodyAngles",a.networkAngles);vector("eyeAngles",a.eyeAngles);vector("collisionMins",a.collisionMins);vector("collisionMaxs",a.collisionMaxs);
  if(!a.animationLayerReason.empty()){r["animationLayerReason"]=a.animationLayerReason;r["animationLayers"]=nlohmann::json::array();std::lock_guard<std::mutex> lock(animationMutex);auto probe=animations.find(a.handle);for(const auto& layer:a.animationLayers){nlohmann::json l={{"index",layer.index},{"sequence",layer.sequence},{"cycle",layer.cycle},{"previousCycle",layer.previousCycle},{"weight",layer.weight},{"order",layer.order}};if(probe!=animations.end()&&probe->second.epoch==actors->epoch){auto n=probe->second.layerNames.find(layer.sequence);if(n!=probe->second.layerNames.end()){l["label"]=n->second.label;l["activity"]=n->second.activity;}auto why=probe->second.layerReasons.find(layer.sequence);if(why!=probe->second.layerReasons.end())l["resolution"]=why->second;}r["animationLayers"].push_back(l);}}
  if(a.weaponOwner)r["weaponOwner"]=*a.weaponOwner;if(a.owner)r["owner"]=*a.owner;if(a.animationCycle)r["animationCycle"]=*a.animationCycle;
  if(a.health)r["health"]=*a.health;if(a.lookAtHandle)r["lookAtHandle"]=*a.lookAtHandle;if(a.enemyHandle)r["enemyHandle"]=*a.enemyHandle;if(a.activeWeapon)r["activeWeapon"]=*a.activeWeapon;if(a.nextPrimaryAttack)r["nextPrimaryAttack"]=*a.nextPrimaryAttack;if(a.nextAttack)r["nextAttack"]=*a.nextAttack;if(a.simulationTime)r["simulationTime"]=*a.simulationTime;j["actors"].push_back(r);
 }
 auto path=visual::diagnostics::path.parent_path()/L"tank-prediction.jsonl";std::error_code e;auto size=std::filesystem::file_size(path,e);if(!e&&size>=4u*1024u*1024u){auto old=path;old.replace_filename(L"tank-prediction.previous.jsonl");if(!MoveFileExW(path.c_str(),old.c_str(),MOVEFILE_REPLACE_EXISTING))return;}
 std::ofstream file(path,std::ios::app|std::ios::binary);file<<j.dump(-1,' ',false,nlohmann::json::error_handler_t::replace)<<'\n';
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
}
