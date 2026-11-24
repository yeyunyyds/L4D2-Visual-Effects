#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../source_render.hpp"
#include "../hud_runtime.hpp"
#include "../entity_menu.hpp"
#include "../aim_runtime.hpp"
#include "../animation_config.hpp"
namespace visual::animationfeature {
using namespace visual::ui;

inline void note(const std::string& text){visual::diagnostics::write(text);}
class Runtime {
 std::atomic<bool> requested{false};
 std::shared_ptr<const source_entities::Frame> previous;
 std::shared_ptr<const visual::EntitySelection> previousSelection;
 std::shared_ptr<const visual::ProximitySettings> previousProximity;
 std::uint32_t previousEpoch=0;bool previousGameActive=false,selected=false;
inline void toggle(){bool value=!requested.load();requested=value;visual::render::startedAt=GetTickCount64();visual::render::enabled=value;note(value?"Stretch enabled":"Stretch disabled");}
visual::EntityMenuState entityMenus;
inline void storeProximity(visual::ProximitySettings settings){
 std::atomic_store(&visual::render::proximity,std::make_shared<const visual::ProximitySettings>(std::move(settings)));
 saveMessage=u8"范围触发设置未保存";
}
inline bool entityAction(uintptr_t id){
 auto frame=std::atomic_load(&source_entities::snapshot);entityMenus.observe(*frame,*std::atomic_load(&visual::render::selection));visual::aimruntime::poll(entityMenus,frame->epoch);
 if(id==122){visual::aimruntime::start(entityMenus);saveMessage=u8"已提交识别，等待游戏帧";return true;}
 if(id==126){visual::aimruntime::cancel(entityMenus);return true;}
 if(id==127){entityMenus.handsOnly=!entityMenus.handsOnly;visual::aimruntime::cancel(entityMenus);saveMessage=u8"已切换模式，请手动识别";return true;}
 if(id==128){entityMenus.next();saveMessage=u8"已切换候选，请核对类型";return true;}

 if((id>=180&&id<=186)||id==190||id==191||(id>=0x20000&&id<=0x21000)||(id>=0x40000&&id<0x40000+visual::BuiltinEntityClassCount)){
  auto settings=*std::atomic_load(&visual::render::proximity);
  if(id==180){settings.enabled=!settings.enabled;if(settings.enabled&&!requested)toggle();}
  if(id>=181&&id<=184){int delta=id==181?-100:id==182?100:id==183?-10:10;settings.radius=unsigned(std::clamp(int(settings.radius)+delta,100,5000));}
  if(id==185)settings.includeCommon=!settings.includeCommon;
  if(id==186)settings=visual::ProximitySettings{};
  if(id==190){settings.triggerClassId=-1;settings.triggerName="Tank";}
  if(id==191){if(entityMenus.aimed.classId<0)return true;auto r=visual::aimRecord(entityMenus.aimed);bool role=visual::specialCharacter(r)>=0||visual::survivorCharacter(r)>=0;settings.triggerClassId=role?-1:entityMenus.aimed.classId;settings.triggerName=role?visual::aimLabel(entityMenus.aimed):entityMenus.aimed.name;}
  if(id>=0x20000&&id<=0x21000){int key=int(id-0x20000);auto label=entityMenus.known.find(key);if(label==entityMenus.known.end())return true;settings.triggerClassId=key;settings.triggerName=label->second;}
  if(id>=0x40000&&id<0x40000+visual::BuiltinEntityClassCount){settings.triggerClassId=-1;settings.triggerName=visual::BuiltinEntityClasses[id-0x40000];}
  storeProximity(std::move(settings));return true;
 }
 if(id>=0x30000&&id<0x30000+visual::BuiltinEntityClassCount){
  auto selection=*std::atomic_load(&visual::render::selection);
  for(const auto& c:entityMenus.known)selection.names[c.first]=c.second;
  std::string name=visual::BuiltinEntityClasses[id-0x30000];
  auto known=std::find_if(entityMenus.known.begin(),entityMenus.known.end(),[&](const auto& c){return c.second==name;});
  bool current=known==entityMenus.known.end()?selection.selectedType(name):selection.selected(known->first,name);
  selection.setType(name,!current);std::atomic_store(&visual::render::selection,std::make_shared<const visual::EntitySelection>(std::move(selection)));
  saveMessage=u8"实体类型设置未保存";return true;
 }
 if(id>=0x60000&&id<0x60000+visual::SpecialCount){
  auto selection=*std::atomic_load(&visual::render::selection);int role=int(id-0x60000);
  selection.specials[role]=!selection.selectedSpecial(role);
  std::atomic_store(&visual::render::selection,std::make_shared<const visual::EntitySelection>(std::move(selection)));
  saveMessage=u8"特感类型设置未保存";return true;
 }
 if(id>=0x50000&&id<0x50000+visual::SurvivorCount){
  auto selection=*std::atomic_load(&visual::render::selection);int role=int(id-0x50000);
  selection.survivors[role]=!selection.selectedSurvivor(role);
  std::atomic_store(&visual::render::selection,std::make_shared<const visual::EntitySelection>(std::move(selection)));
  saveMessage=u8"求生者角色设置未保存";return true;
 }
 bool aimed=id==124||id==125;
 if(aimed&&entityMenus.aimed.classId<0)return true;
 bool group=id>=130&&id<130+visual::EntityGroupCount*2;
 if(id!=121&&!aimed&&!group&&!(id>=0x10000&&id<=0x11000))return false;
 auto selection=*std::atomic_load(&visual::render::selection);
 if(id==121){
  selection.setAll(false);auto settings=*std::atomic_load(&visual::render::proximity);settings.enabled=false;storeProximity(std::move(settings));
  requested=false;visual::render::enabled=false;visual::render::proximityActive=false;
 }else if(group){
  for(const auto& c:entityMenus.known)selection.names[c.first]=c.second;
  selection.setGroup(visual::EntityGroup((id-130)/2),(id-130)%2==0);
 }else if(aimed){
  if(!selection.setAim(entityMenus.aimed,id==124)){saveMessage=u8"角色资料不足，请重新识别";return true;}
 }else{
  int classId=int(id-0x10000);auto label=entityMenus.known.find(classId);
  if(label==entityMenus.known.end())return true;
  bool value=!selection.selected(classId,label->second);selection.setClass(classId,label->second,value);
 }
 std::atomic_store(&visual::render::selection,std::make_shared<const visual::EntitySelection>(std::move(selection)));
 saveMessage=u8"实体类别设置未保存";return true;
}
inline std::optional<std::string> page(uintptr_t id){
 if(!(id==300||id==103||id==104||id==108||id==109||id==107||id==102||id==24||(id>=1000&&id<1000+visual::EntityGroupCount)))return std::nullopt;
 auto parameters=std::atomic_load(&visual::render::animation);
 auto f=std::atomic_load(&source_entities::snapshot);
 auto selection=std::atomic_load(&visual::render::selection);entityMenus.observe(*f,*selection);visual::aimruntime::poll(entityMenus,f->epoch);
 if(id==103)return entityMenus.root();
 if(id==104)return entityMenus.recognition(*selection,std::atomic_load(&visual::render::proximity)->enabled);
 if(id>=1000&&id<1000+visual::EntityGroupCount)return entityMenus.group(visual::EntityGroup(id-1000),*selection);
 if(id==108)return visual::proximityMenu(*std::atomic_load(&visual::render::proximity),visual::render::proximityActive.load(),visual::render::proximityCount.load(),visual::render::proximitySources.load());
 if(id==109)return entityMenus.triggers();
 if(id==107)return "\"性能与兼容\" {"+
  item(u8"骨骼上限："+std::to_string(visual::render::boneLimit.load())+u8"（切换128/256）",118)+
  item(visual::render::compatibility.load()?u8"渲染模式：完整兼容":u8"渲染模式：保留快速路径",119)+
  row(u8"快速模式可能漏掉部分主模型")+item(u8"保存设置",101)+"}";
 if(id==102){return "\"动画细调\" {"+
 item(u8"动画预设："+std::string(visual::presetName(parameters->preset)),116)+
 item(u8"往复周期 -0.1 秒："+decimal(int(parameters->periodMs/100))+u8" 秒",110)+
 item(u8"往复周期 +0.1 秒",111)+
 item(u8"最小比例 -0.1："+decimal(parameters->minTenths),112)+item(u8"最小比例 +0.1",113)+
 item(u8"最大比例 -0.1："+decimal(parameters->maxTenths),114)+item(u8"最大比例 +0.1",115)+
 item(u8"恢复动画默认：0.5秒 / 0.5～1.1",117)+item(u8"保存设置",101)+"}";}
 if(id==24){std::string body=row(visual::render::status.load());body+=row(std::string(u8"Studio配置：")+visual::render::studioProfile.load());body+=row("Camera: "+std::string(source_camera::status()));body+=row("Entities: "+f->status);body+=row("Identify: "+std::string(visual::render::identifyReason.load()));body+=row("Pose: "+std::string(visual::render::poseReason.load()));body+=row("Engine/Studio: "+std::to_string(visual::render::engineCalls.load())+" / "+std::to_string(visual::render::studioCalls.load()));body+=row("Matched/Drawn: "+std::to_string(visual::render::identified.load())+" / "+std::to_string(visual::render::drawn.load()));body+=row(visual::diagnostics::writable?u8"日志已写入 L4D2Visual/L4D2Visual.log":u8"日志写入失败，请检查目录权限");return "\"L4D2Visual · 诊断\" {"+body+"}";}
 auto proximity=std::atomic_load(&visual::render::proximity);
 std::string proximityLabel=!proximity->enabled?u8"范围触发：关":visual::render::proximityActive.load()?u8"范围触发：已触发":u8"范围触发：待机";
 if(id==300)return "\"邦友的酒\" {"+item(requested?u8"邦友的酒：开":u8"邦友的酒：关",100)+
 item(u8"实体分类",103)+item(u8"手动识别与开关",104)+item(u8"动画预设与细调",102)+item(u8"性能与兼容",107)+
 item(proximityLabel,108)+item(u8"接口与运行诊断",24)+item(u8"保存设置",101)+"}";
 return {};
}
inline bool action(uintptr_t id){if(entityAction(id))return true;if(id==100)toggle();if(id>=110&&id<=117){auto a=*std::atomic_load(&visual::render::animation);a=visual::adjustAnimation(a,unsigned(id));std::atomic_store(&visual::render::animation,std::make_shared<const visual::AnimationSettings>(a));visual::render::startedAt=GetTickCount64();saveMessage=u8"动画设置未保存";}if(id==119){visual::render::compatibility=!visual::render::compatibility.load();saveMessage=u8"渲染模式未保存";}if(id==118){visual::render::boneLimit=visual::nextBoneLimit(visual::render::boneLimit.load());saveMessage=u8"骨骼上限未保存";}return id==100||(id>=110&&id<=119);}
inline void onFrame(const dango::GameCameraFrame& frame){
 // Disabled animation must not evaluate inventories requested by other modules.
 // Drop the old snapshot once; re-enabling always rebuilds the selection.
 if(!requested.load(std::memory_order_acquire)){
  if(previous){
   previous.reset();previousSelection.reset();previousProximity.reset();selected=false;
   visual::render::enabled=false;visual::render::proximityActive=false;
   visual::render::proximityCount=0;visual::render::proximitySources=0;
   std::atomic_store(&visual::render::proximityTargets,std::make_shared<const visual::ProximityResult>());
  }
  return;
 }
 auto selection=std::atomic_load(&visual::render::selection);
 auto proximity=std::atomic_load(&visual::render::proximity);
 auto inventory=std::atomic_load(&source_entities::snapshot);
 if(previous!=inventory||previousProximity!=proximity||previousEpoch!=frame.epoch||previousGameActive!=bool(frame.active)){
  auto targets=std::make_shared<visual::ProximityResult>(visual::evaluateProximity(*inventory,frame,*proximity,frame.tick));
  targets->options=proximity;
  std::atomic_store(&visual::render::proximityTargets,std::shared_ptr<const visual::ProximityResult>(std::move(targets)));
 }
 auto targets=std::atomic_load(&visual::render::proximityTargets);
 bool proximityTriggered=requested&&frame.active&&targets->epoch==frame.epoch&&frame.tick>=targets->tick&&frame.tick-targets->tick<=250&&proximity->enabled&&targets->active;
 bool wasSpatial=visual::render::proximityActive.exchange(proximityTriggered,std::memory_order_relaxed);
 if(proximityTriggered&&!wasSpatial)visual::render::startedAt=GetTickCount64();
 visual::render::proximityCount.store(unsigned(targets->inRange.size()),std::memory_order_relaxed);
 visual::render::proximitySources.store(targets->sources,std::memory_order_relaxed);
 if(previous!=inventory||previousSelection!=selection||previousProximity!=proximity||previousEpoch!=frame.epoch||previousGameActive!=bool(frame.active)){
  previous=inventory;previousSelection=selection;previousProximity=proximity;previousEpoch=frame.epoch;previousGameActive=bool(frame.active);selected=false;
  for(const auto& r:inventory->records)if(visual::eligibleRecord(r)&&visual::selectedForEffect(r,*selection,*proximity,*targets)){selected=true;break;}
 }
 bool active=requested&&frame.active&&inventory->epoch==frame.epoch&&frame.tick>=inventory->tick&&frame.tick-inventory->tick<=250&&selected;
 visual::render::enabled=active;
}
public:
inline modules::Module bindings(){
 modules::Module m;m.id="animation";m.menuOrder=0;
 // Menu parent lives in entity scaling; this remains a separate module.
 m.page=[this](std::uintptr_t id){return page(id);};m.action=[this](std::uintptr_t id){return action(id);};m.frame=[this](const dango::GameCameraFrame& frame){onFrame(frame);};
 m.needsEntities=[this] {if(!requested.load(std::memory_order_acquire))return false;auto selection=std::atomic_load(&render::selection);auto proximity=std::atomic_load(&render::proximity);return selection->anyEnabled()||proximity->enabled;};
 m.diagnostics=[this]{return "requested="+std::to_string(requested.load())+" proximity="+std::to_string(render::proximityActive.load())+" proximityCount="+std::to_string(render::proximityCount.load())+" proximitySources="+std::to_string(render::proximitySources.load());};
 m.needsCompatibility=[] {return render::enabled.load();};
 m.save=[](nlohmann::json& j){writeAnimationSettings(j,*std::atomic_load(&render::animation));writeBoneLimit(j,render::boneLimit);writeRenderCompatibility(j,render::compatibility);writeEntitySelection(j,*std::atomic_load(&render::selection));writeProximity(j,*std::atomic_load(&render::proximity));};
 m.load=[](const nlohmann::json& j){std::atomic_store(&render::proximity,std::make_shared<const ProximitySettings>(readProximity(j)));render::compatibility=readRenderCompatibility(j);std::atomic_store(&render::selection,std::make_shared<const EntitySelection>(readEntitySelection(j)));render::boneLimit=readBoneLimit(j);std::atomic_store(&render::animation,std::make_shared<const AnimationSettings>(readAnimationSettings(j)));};
 m.quiesce=[this] {requested=false;render::enabled=false;};
 m.stop=[this] {requested=false;render::enabled=false;render::proximityActive=false;};return m;
}
};
inline modules::Module create(){auto owner=std::make_shared<Runtime>();auto module=owner->bindings();module.stateOwner=owner;return module;}

}
