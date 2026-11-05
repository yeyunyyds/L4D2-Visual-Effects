#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mutex>
#include <sstream>
#include <thread>
#include <chrono>
#include "l4n/l4n_plugin.h"
#include "dango/source_entities.hpp"
#include "source_render.hpp"
#include "diagnostics.hpp"
#include "hud_runtime.hpp"
#include "animation.hpp"
#include "animation_config.hpp"
#include "model_path.hpp"
#include "aim_selection.hpp"
#include "entity_menu.hpp"
#include "image_menu.hpp"
#include "spray_network.hpp"
static_assert(sizeof(void*)==4,"L4N requires Windows x86");
namespace {
std::atomic<bool> requested{false},saveRequested{false};
std::atomic<const char*> saveMessage{""};
std::mutex appearanceMutex;
dango::hud::Appearance appearance;
void note(const std::string& s){visual::diagnostics::write(s);}
void onFrame(const dango::GameCameraFrame& frame){
 visual::sprayroster::enabled=std::atomic_load(&visual::spraysync::options)->automatic&&std::atomic_load(&visual::spraysync::options)->enabled;visual::sprayroster::capture(frame);
 visual::pictures::onFrame(frame);
 auto selection=std::atomic_load(&visual::render::selection);
 auto proximity=std::atomic_load(&visual::render::proximity);
 auto tankCfg=std::atomic_load(&visual::tank::options);source_entities::combatRequested=tankCfg->enabled&&!visual::tank::faulted;visual::render::modelObserver=(tankCfg->enabled&&!visual::tank::faulted)?visual::tank::observeModel:nullptr;
 source_entities::browseRequested=(tankCfg->enabled&&!visual::tank::faulted)||visual::hudruntime::hud.isOpen()||(requested&&(selection->anyEnabled()||proximity->enabled));
 source_entities::onFrame(frame);
 visual::tank::onFrame(frame);
 auto inventory=std::atomic_load(&source_entities::snapshot);
 static std::shared_ptr<const source_entities::Frame> previous;
 static std::shared_ptr<const visual::EntitySelection> previousSelection;
 static std::shared_ptr<const visual::ProximitySettings> previousProximity;
 static std::uint32_t previousEpoch=0;static bool previousGameActive=false,selected=false;
 if(previous!=inventory||previousProximity!=proximity||previousEpoch!=frame.epoch||previousGameActive!=bool(frame.active)){
  auto targets=std::make_shared<visual::ProximityResult>(visual::evaluateProximity(*inventory,frame,*proximity,frame.tick));
  targets->options=proximity;
  std::atomic_store(&visual::render::proximityTargets,std::shared_ptr<const visual::ProximityResult>(std::move(targets)));
 }
 auto targets=std::atomic_load(&visual::render::proximityTargets);
 bool experimental=requested&&frame.active&&targets->epoch==frame.epoch&&frame.tick>=targets->tick&&frame.tick-targets->tick<=250&&proximity->enabled&&targets->active;
 bool wasSpatial=visual::render::proximityActive.exchange(experimental,std::memory_order_relaxed);
 if(experimental&&!wasSpatial)visual::render::startedAt=GetTickCount64();
 visual::render::proximityCount.store(unsigned(targets->inRange.size()),std::memory_order_relaxed);
 visual::render::proximitySources.store(targets->sources,std::memory_order_relaxed);
 if(previous!=inventory||previousSelection!=selection||previousProximity!=proximity||previousEpoch!=frame.epoch||previousGameActive!=bool(frame.active)){
  previous=inventory;previousSelection=selection;previousProximity=proximity;previousEpoch=frame.epoch;previousGameActive=bool(frame.active);selected=false;
  for(const auto& r:inventory->records)if(visual::eligibleRecord(r)&&visual::selectedForEffect(r,*selection,*proximity,*targets)){selected=true;break;}
 }
 bool active=requested&&frame.active&&inventory->epoch==frame.epoch&&frame.tick>=inventory->tick&&frame.tick-inventory->tick<=250&&selected;
 visual::render::enabled=active;
 visual::modelpath::apply(active&&visual::render::compatibility.load());
}
void toggle(){bool value=!requested.load();requested=value;source_entities::browseRequested=true;visual::render::startedAt=GetTickCount64();visual::render::enabled=value;note(value?"Stretch enabled":"Stretch disabled");}
std::string row(const std::string& label,uintptr_t id=0){return "\""+label+"\" { \"user_data\" \""+std::to_string(id)+"\" }\n";}
// Native menu parser expects hexadecimal action values.
std::string item(const std::string& label,uintptr_t id){std::ostringstream s;s<<"0x"<<std::hex<<id;return "\""+label+"\" { \"user_data\" \""+s.str()+"\" }\n";}
std::string decimal(int tenths){return std::to_string(tenths/10)+"."+std::to_string(tenths%10);}
visual::EntityMenuState entityMenus;
void storeProximity(visual::ProximitySettings settings){
 std::atomic_store(&visual::render::proximity,std::make_shared<const visual::ProximitySettings>(std::move(settings)));
 saveMessage=u8"范围实验设置未保存";
}
bool entityAction(uintptr_t id){
 if(id==122){
  auto frame=std::atomic_load(&source_entities::snapshot);auto camera=std::atomic_load(&source_camera::snapshot);
  entityMenus.aimed=visual::identifyAim(*frame,*camera,GetTickCount64());
  const auto& aim=entityMenus.aimed;
  if(aim.classId>=0){entityMenus.known[aim.classId]=aim.name;saveMessage=u8"已识别，请手动选择开启或关闭";}
  else saveMessage=u8"未识别到目标，请对准实体并稍等";
  return true;
 }
 if(id==126){entityMenus.aimed={};return true;}
 if((id>=180&&id<=186)||id==190||id==191||(id>=0x20000&&id<=0x21000)||(id>=0x40000&&id<0x40000+visual::BuiltinEntityClassCount)){
  auto settings=*std::atomic_load(&visual::render::proximity);
  if(id==180){settings.enabled=!settings.enabled;if(settings.enabled&&!requested)toggle();}
  if(id>=181&&id<=184){int delta=id==181?-100:id==182?100:id==183?-10:10;settings.radius=unsigned(std::clamp(int(settings.radius)+delta,100,5000));}
  if(id==185)settings.includeCommon=!settings.includeCommon;
  if(id==186)settings=visual::ProximitySettings{};
  if(id==190){settings.triggerClassId=-1;settings.triggerName="Tank";}
  if(id==191){if(entityMenus.aimed.classId<0)return true;settings.triggerClassId=entityMenus.aimed.classId;settings.triggerName=entityMenus.aimed.name;}
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
 }else{
  int classId=aimed?entityMenus.aimed.classId:int(id-0x10000);auto label=entityMenus.known.find(classId);
  if(label==entityMenus.known.end())return true;
  bool value=aimed?id==124:!selection.selected(classId,label->second);selection.setClass(classId,label->second,value);
 }
 std::atomic_store(&visual::render::selection,std::make_shared<const visual::EntitySelection>(std::move(selection)));
 saveMessage=u8"实体类别设置未保存";return true;
}
std::string buildMenu(uintptr_t id){
 auto parameters=std::atomic_load(&visual::render::animation);
 auto f=std::atomic_load(&source_entities::snapshot);
 auto selection=std::atomic_load(&visual::render::selection);entityMenus.observe(*f,*selection);
 if(id==320)return visual::tank::menu();
 if(id>=150&&id<=154)return visual::picturemenu::page(id);
 if(id==103)return entityMenus.root();
 if(id==104)return entityMenus.recognition(*selection,std::atomic_load(&visual::render::proximity)->enabled);
 if(id>=1000&&id<1000+visual::EntityGroupCount)return entityMenus.group(visual::EntityGroup(id-1000),*selection);
 if(id==108)return visual::proximityMenu(*std::atomic_load(&visual::render::proximity),visual::render::proximityActive.load(),visual::render::proximityCount.load(),visual::render::proximitySources.load());
 if(id==109)return entityMenus.triggers();
 if(id==107)return "\"性能与兼容\" {"+
  item(u8"骨骼上限："+std::to_string(visual::render::boneLimit.load())+u8"（切换128/256）",118)+
  item(visual::render::compatibility.load()?u8"渲染模式：完整兼容":u8"渲染模式：保留快速路径（实验）",119)+
  row(u8"快速模式可能漏掉部分主模型")+item(u8"保存设置",101)+"}";
 if(id==102){return "\"动画细调\" {"+
 item(u8"动画预设："+std::string(visual::presetName(parameters->preset)),116)+
 item(u8"往复周期 -0.1 秒："+decimal(int(parameters->periodMs/100))+u8" 秒",110)+
 item(u8"往复周期 +0.1 秒",111)+
 item(u8"最小比例 -0.1："+decimal(parameters->minTenths),112)+item(u8"最小比例 +0.1",113)+
 item(u8"最大比例 -0.1："+decimal(parameters->maxTenths),114)+item(u8"最大比例 +0.1",115)+
 item(u8"恢复动画默认：0.5秒 / 0.5～1.1",117)+item(u8"保存设置",101)+"}";}
 if(id==24){std::string body=row(visual::render::status.load());body+=row("Camera: "+std::string(source_camera::status()));body+=row("Entities: "+f->status);body+=row("Identify: "+std::string(visual::render::identifyReason.load()));body+=row("Pose: "+std::string(visual::render::poseReason.load()));body+=row("Engine/Studio: "+std::to_string(visual::render::engineCalls.load())+" / "+std::to_string(visual::render::studioCalls.load()));body+=row("Matched/Drawn: "+std::to_string(visual::render::identified.load())+" / "+std::to_string(visual::render::drawn.load()));body+=row(visual::diagnostics::writable?u8"日志已写入 L4D2Visual/L4D2Visual.log":u8"日志写入失败，请检查目录权限");return "\"L4D2Visual · 诊断\" {"+body+"}";}
 auto proximity=std::atomic_load(&visual::render::proximity);
 std::string experimental=!proximity->enabled?u8"范围实验：关":visual::render::proximityActive.load()?u8"范围实验：已触发":u8"范围实验：待机";
 if(id==300)return "\"邦友的酒\" {"+item(requested?u8"邦友的酒：开":u8"邦友的酒：关",100)+
 item(u8"实体分类",103)+item(u8"手动识别与开关",104)+item(u8"动画预设与细调",102)+item(u8"性能与兼容",107)+
 item(experimental,108)+item(u8"接口与日志诊断",24)+item(u8"保存设置",101)+"}";
 return "\"L4D2Visual 0.7.3-tank-preview\" {"+item(requested?u8"邦友的酒[开]":u8"邦友的酒[关]",300)+
 item(u8"自定义喷漆",150)+item(u8"Tank 行为预测（实验）",320)+item(u8"菜单外观",9000)+item(u8"保存设置",101)+"}";
}
void action(uintptr_t id){if(visual::tank::action(id)){saveMessage=u8"Tank预测设置未保存";return;}if(visual::picturemenu::action(id)){saveMessage=visual::picturemenu::notice.load();return;}if(entityAction(id))return;if(id==100)toggle();if(id>=110&&id<=117){auto a=*std::atomic_load(&visual::render::animation);a=visual::adjustAnimation(a,unsigned(id));std::atomic_store(&visual::render::animation,std::make_shared<const visual::AnimationSettings>(a));visual::render::startedAt=GetTickCount64();saveMessage=u8"动画设置未保存";}if(id==119){visual::render::compatibility=!visual::render::compatibility.load();saveMessage=u8"渲染模式未保存";}if(id==118){visual::render::boneLimit=visual::nextBoneLimit(visual::render::boneLimit.load());saveMessage=u8"骨骼上限未保存";}if(id==101){saveRequested=true;saveMessage=u8"已提交保存";}}
void closed(){}
std::string notice(){return saveMessage.load();}
void appearanceChanged(const dango::hud::Appearance& a){std::lock_guard<std::mutex> l(appearanceMutex);appearance=a;saveMessage=u8"菜单外观未保存";}
const char* __cdecl openMenu(void*){visual::hudruntime::hud.open();return nullptr;}
class Plugin final:public IL4NPlugin {
 std::atomic<bool> stopping{false};std::thread worker;
 void connect(){source_camera::logger=note;source_camera::frameListener=onFrame;if(!source_camera::clientModule)source_camera::clientModule=GetModuleHandleW(L"client.dll");if(!source_camera::engineModule)source_camera::engineModule=GetModuleHandleW(L"engine.dll");source_camera::connect();visual::render::connect();}
 void diagnostics(){
  std::string previous;ULONGLONG last=0,nextAssets=0,nextDiagnostics=0;
  while(!stopping){
   if(GetTickCount64()>=nextAssets||visual::pictures::refreshRequested){visual::pictures::pollAssets();nextAssets=GetTickCount64()+200;}visual::spraysync::network.tick(GetTickCount64());visual::tank::poll(GetTickCount64());
   if(saveRequested.exchange(false)){try{dango::hud::Appearance a;{std::lock_guard<std::mutex> l(appearanceMutex);a=appearance;}auto anim=std::atomic_load(&visual::render::animation);nlohmann::json j={{"preset",a.preset},{"step",a.step},{"opacity",a.opacity},{"font",a.font},{"x",a.x},{"y",a.y},{"f10",a.f10},{"centerY",a.centerY}};visual::writeAnimationSettings(j,*anim);visual::writeBoneLimit(j,visual::render::boneLimit.load());visual::writeRenderCompatibility(j,visual::render::compatibility.load());visual::writeEntitySelection(j,*std::atomic_load(&visual::render::selection));visual::writeProximity(j,*std::atomic_load(&visual::render::proximity));visual::spraysync::writeSettings(j,*std::atomic_load(&visual::spraysync::options));visual::tank::writeSettings(j,*std::atomic_load(&visual::tank::options));visual::pictures::writeSettings(j,*std::atomic_load(&visual::pictures::options));auto path=visual::diagnostics::path.parent_path()/L"hud.json";auto tmp=path;tmp+=L".tmp";{std::ofstream o(tmp);o<<j.dump(2);o.flush();if(!o)throw std::runtime_error("HUD settings write failed");}if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("HUD settings rename failed");saveMessage=u8"菜单设置已保存";note("HUD appearance saved");}catch(const std::exception& e){saveMessage=u8"保存失败，请查看日志";note(e.what());}}
   if(GetTickCount64()>=nextDiagnostics){nextDiagnostics=GetTickCount64()+200;
   auto f=std::atomic_load(&source_entities::snapshot);std::ostringstream state;state<<"enabled="<<requested<<" render="<<visual::render::status.load()<<" camera="<<source_camera::status()<<" entities="<<f->status<<" records="<<f->records.size()<<" modelLookup="<<f->modelLookup<<" engine="<<visual::render::engineCalls<<" studio="<<visual::render::studioCalls<<" matched="<<visual::render::identified<<" drawn="<<visual::render::drawn<<" identify="<<visual::render::identifyReason.load()<<" pose="<<visual::render::poseReason.load()<<" pictures="<<std::atomic_load(&visual::pictures::sceneFrame)->items.size()<<" pictureRender="<<visual::picturerender::status.load()<<" proximity="<<visual::render::proximityActive.load()<<" proximityCount="<<visual::render::proximityCount.load()<<" proximitySources="<<visual::render::proximitySources.load()<<" compatibility="<<visual::render::compatibility.load()<<" boneLimit="<<visual::render::boneLimit.load()<<" fastpath="<<visual::modelpath::current<<" periodMs="<<std::atomic_load(&visual::render::animation)->periodMs<<" HUD="<<visual::hudruntime::paintStatus.load()<<" presents="<<visual::hudruntime::presents<<" hudFrames="<<visual::hudruntime::frames;
   auto now=GetTickCount64();if(now-last>=2000 && (requested||state.str()!=previous)){note(state.str());for(const auto& check:f->bindingChecks)note("Entity binding: "+check);std::map<std::string,unsigned> inventory;
    for(const auto& r:f->records)if(visual::eligibleRecord(r))++inventory[r.className];
    std::string classes;for(const auto& pair:inventory)classes+=" "+pair.first+"="+std::to_string(pair.second);
    note("Modeled entity inventory:"+classes);
    note("Per-target draw stats:"+visual::render::targetSummary());previous=state.str();last=now;}
   }
   std::this_thread::sleep_for(std::chrono::milliseconds((std::atomic_load(&visual::spraysync::options)->enabled||std::atomic_load(&visual::tank::options)->enabled)?20:200));
  }
 }
public:
 Plugin(){note("=== L4D2Visual 0.7.3-tank-preview startup ===");note("DLL directory: "+visual::diagnostics::path.parent_path().parent_path().u8string());source_camera::logger=note;visual::hudruntime::hud.configure(buildMenu,action,closed,notice,note,appearanceChanged);
  try{std::ifstream file(visual::diagnostics::path.parent_path()/L"hud.json");if(file){nlohmann::json j;file>>j;std::atomic_store(&visual::tank::options,std::make_shared<const visual::tank::Settings>(visual::tank::readSettings(j)));std::atomic_store(&visual::spraysync::options,std::make_shared<const visual::spraysync::Settings>(visual::spraysync::readSettings(j)));std::atomic_store(&visual::pictures::options,std::make_shared<const visual::pictures::Settings>(visual::pictures::readSettings(j)));std::atomic_store(&visual::render::proximity,std::make_shared<const visual::ProximitySettings>(visual::readProximity(j)));visual::render::compatibility=visual::readRenderCompatibility(j);std::atomic_store(&visual::render::selection,std::make_shared<const visual::EntitySelection>(visual::readEntitySelection(j)));visual::render::boneLimit=visual::readBoneLimit(j);std::atomic_store(&visual::render::animation,std::make_shared<const visual::AnimationSettings>(visual::readAnimationSettings(j)));appearance.preset=std::min(3u,j.value("preset",0u));appearance.step=std::clamp(j.value("step",8u),1u,64u);appearance.opacity=std::clamp(j.value("opacity",.48f),.05f,1.f);appearance.font=std::clamp(j.value("font",22.f),12.f,48.f);appearance.x=j.value("x",8.f);appearance.y=j.value("y",14.f);appearance.f10=j.value("f10",false);appearance.centerY=j.value("centerY",true);visual::hudruntime::hud.loadAppearance(appearance);}}catch(const std::exception& e){note(std::string("HUD settings load failed: ")+e.what());}
  visual::pictures::folder=visual::diagnostics::path.parent_path()/L"images";
  worker=std::thread([this]{diagnostics();});}
 const char* GetName()override{return "L4D2Visual";}
 const char* GetVersion()override{return "0.7.3-tank-preview";}
 void OnModuleLoaded(const char* name,uintptr_t handle)override{if(!name)return;std::string n(name);if(n=="client"||n=="client.dll")source_camera::clientModule=reinterpret_cast<HMODULE>(handle);if(n=="engine"||n=="engine.dll")source_camera::engineModule=reinterpret_cast<HMODULE>(handle);note("Module loaded: "+n);connect();}
 void OnGameLaunch()override{note("OnGameLaunch");connect();}
 void OnD3DDeviceCreated(void* d,bool dxvk)override{note(std::string("OnD3DDeviceCreated dxvk=")+(dxvk?"yes":"no"));visual::hudruntime::created(static_cast<IDirect3DDevice9*>(d));}
 const char* RequestHudMenu(bool title)override{if(title)return "L4D2Visual 0.7.3-tank-preview";visual::hudruntime::hud.open();static std::string entry;std::ostringstream address;address<<"0x"<<std::hex<<reinterpret_cast<uintptr_t>(&openMenu);entry="\"L4D2Visual\" { \"打开视觉效果菜单\" { \"callback\" \""+address.str()+"\" } }";return entry.c_str();}
 void stop(){stopping=true;if(worker.joinable())worker.join();visual::spraysync::network.stop();requested=false;visual::render::modelObserver=nullptr;source_entities::combatRequested=false;visual::modelpath::apply(false);visual::hudruntime::stop();visual::render::stop();source_entities::browseRequested=false;source_camera::frameListener=nullptr;source_camera::stop();}
 ~Plugin()override{stop();}
};
Plugin& instance(){static Plugin object;return object;}
}
#if defined(__GNUC__) && !defined(_MSC_VER)
// Explicit nine-slot MSVC x86 adapter, matching the reference plugin.
namespace {
void* __fastcall destroy(void* self,void*,unsigned){instance().stop();return self;}
unsigned __fastcall version(void*,void*){return 2;}
const char* __fastcall name(void*,void*){return instance().GetName();}
const char* __fastcall release(void*,void*){return instance().GetVersion();}
void __fastcall module(void*,void*,const char* n,std::uintptr_t h){instance().OnModuleLoaded(n,h);}
void __fastcall launch(void*,void*){instance().OnGameLaunch();}
void __fastcall d3d(void*,void*,void*){}
void __fastcall device(void*,void*,void* d,bool dxvk){instance().OnD3DDeviceCreated(d,dxvk);}
const char* __fastcall menu(void*,void*,bool title){return instance().RequestHudMenu(title);}
void* table[]={reinterpret_cast<void*>(&destroy),reinterpret_cast<void*>(&version),reinterpret_cast<void*>(&name),reinterpret_cast<void*>(&release),reinterpret_cast<void*>(&module),reinterpret_cast<void*>(&launch),reinterpret_cast<void*>(&d3d),reinterpret_cast<void*>(&device),reinterpret_cast<void*>(&menu)};
struct Object{void** vtable;} object{table};
}
extern "C" __declspec(dllexport) IL4NPlugin* GetL4NPluginInstance(){instance();return reinterpret_cast<IL4NPlugin*>(&object);}
#else
extern "C" __declspec(dllexport) IL4NPlugin* GetL4NPluginInstance(){return &instance();}
#endif
