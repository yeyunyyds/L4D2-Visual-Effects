#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mutex>
#include <sstream>
#include <thread>
#include <chrono>
#include "l4n/l4n_plugin.h"
#include "feature_composition.hpp"
#include "model_path.hpp"
#include "log_settings.hpp"
static_assert(sizeof(void*)==4,"L4N requires Windows x86");
namespace {
using namespace visual::ui;
std::mutex appearanceMutex;
dango::hud::Appearance appearance;
void note(const std::string& s){visual::diagnostics::write(s);}
void onFrame(const dango::GameCameraFrame& frame){
 auto& modules=visual::features::host();visual::features::prepareRender();
 source_entities::combatRequested=modules.needsCombat();
 source_entities::browseRequested=modules.needsEntities()||visual::hudruntime::hud.isOpen();
 source_entities::onFrame(frame);modules.frame(frame);visual::features::finishRender();
 visual::modelpath::apply(modules.needsCompatibility()&&visual::render::compatibility.load());
}
std::string buildMenu(std::uintptr_t id){
 if(id==9001)return "\"菜单\" {"+item(u8"外观设置",9000)+
  item(visual::logging::periodicDebugEnabled()?u8"日志级别：Debug（周期详细诊断开启）":u8"日志级别：Info（周期详细诊断关闭）",896)+
  item(u8"保存设置",101)+"}";
 auto page=visual::features::host().page(id);if(page)return *page;
 return "\"L4D2Visual 1.0.0\" {"+visual::features::host().rootItems()+
  item(u8"菜单",9001)+item(u8"保存设置",101)+"}";
}
void action(std::uintptr_t id){
 if(id==896){
  auto next=visual::logging::toggle();
  saveMessage=next==visual::logging::Level::Debug?u8"周期 Debug 诊断已开启，保存设置后持久化":u8"已恢复 Info，周期诊断已关闭；保存后持久化";
  note(next==visual::logging::Level::Debug?"Log level DEBUG: periodic diagnostics enabled":"Log level INFO: periodic diagnostics disabled");
  return;
 }
 if(visual::features::host().action(id))return;
 if(id==101){saveRequested=true;saveMessage=u8"已提交保存";}
}
void closed(){}
std::string notice(){return saveMessage.load();}
void appearanceChanged(const dango::hud::Appearance& a){std::lock_guard<std::mutex> l(appearanceMutex);appearance=a;saveMessage=u8"菜单外观未保存";}
const char* __cdecl openMenu(void*){visual::hudruntime::hud.open();return nullptr;}
class Plugin final:public IL4NPlugin {
 std::atomic<bool> stopping{false},stopStarted{false};std::thread worker;
 void connect(){source_camera::logger=note;source_camera::frameListener=onFrame;if(!source_camera::clientModule)source_camera::clientModule=GetModuleHandleW(L"client.dll");if(!source_camera::engineModule)source_camera::engineModule=GetModuleHandleW(L"engine.dll");source_camera::connect();visual::render::connect();}
 // Periodic per-target and interface snapshots are DEBUG-only; INFO events stay active.
 void periodicDebug(std::string& previous,ULONGLONG& last,ULONGLONG& lastPoll,ULONGLONG now){
   if(now-lastPoll>=200){lastPoll=now;
   auto f=std::atomic_load(&source_entities::snapshot);std::ostringstream state;state<<"enabled="<<visual::features::host().needsCompatibility()<<" render="<<visual::render::status.load()<<" camera="<<source_camera::status()<<" entities="<<f->status<<" records="<<f->records.size()<<" modelLookup="<<f->modelLookup<<" engine="<<visual::render::engineCalls<<" studio="<<visual::render::studioCalls<<" matched="<<visual::render::identified<<" drawn="<<visual::render::drawn<<" identify="<<visual::render::identifyReason.load()<<" pose="<<visual::render::poseReason.load()<<" compatibility="<<visual::render::compatibility.load()<<" boneLimit="<<visual::render::boneLimit.load()<<" fastpath="<<visual::modelpath::current<<" periodMs="<<std::atomic_load(&visual::render::animation)->periodMs<<" HUD="<<visual::hudruntime::paintStatus.load()<<" presents="<<visual::hudruntime::presents<<" hudFrames="<<visual::hudruntime::frames<<visual::features::host().diagnostics();
   if(now-last>=2000 && (visual::features::host().needsCompatibility()||state.str()!=previous)){note(state.str());for(const auto& check:f->bindingChecks)note("Entity binding: "+check);std::map<std::string,unsigned> inventory;
    for(const auto& r:f->records)if(visual::eligibleRecord(r))++inventory[r.className];
    std::string classes;for(const auto& pair:inventory)classes+=" "+pair.first+"="+std::to_string(pair.second);
    note("Modeled entity inventory:"+classes);
    note("Per-target draw stats:"+visual::render::targetSummary());previous=state.str();last=now;}
   }

 }
 void diagnostics(){
  std::string previous;ULONGLONG last=0,lastPoll=0;
  bool previousDebug=false;
  while(!stopping){
   visual::features::host().tick(GetTickCount64());
   if(saveRequested.exchange(false)){try{dango::hud::Appearance a;{std::lock_guard<std::mutex> l(appearanceMutex);a=appearance;}nlohmann::json j={{"preset",a.preset},{"step",a.step},{"opacity",a.opacity},{"font",a.font},{"x",a.x},{"y",a.y},{"f10",a.f10},{"centerY",a.centerY}};visual::features::host().save(j);visual::logging::writeSettings(j);auto path=visual::diagnostics::path.parent_path()/L"hud.json";auto tmp=path;tmp+=L".tmp";{std::ofstream o(tmp);o<<j.dump(2);o.flush();if(!o)throw std::runtime_error("HUD settings write failed");}if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("HUD settings rename failed");saveMessage=u8"菜单设置已保存";note("HUD appearance saved");}catch(const std::exception& e){saveMessage=u8"保存失败，请查看日志";note(e.what());}}
   bool debug=visual::logging::periodicDebugEnabled();
   if(debug){
    if(!previousDebug){last=lastPoll=0;previous.clear();}
    periodicDebug(previous,last,lastPoll,GetTickCount64());
   }
   previousDebug=debug;
   std::this_thread::sleep_for(std::chrono::milliseconds(visual::features::host().pollInterval()));
  }
 }

public:
 Plugin(){note("=== L4D2Visual 1.0.0 startup ===");note("DLL directory: "+visual::diagnostics::path.parent_path().parent_path().u8string());source_camera::logger=note;visual::hudruntime::hud.configure(buildMenu,action,closed,notice,note,appearanceChanged);visual::features::install();
  try{std::ifstream file(visual::diagnostics::path.parent_path()/L"hud.json");if(file){nlohmann::json j;file>>j;visual::logging::level.store(visual::logging::readLevel(j),std::memory_order_relaxed);visual::features::host().load(j);appearance.preset=std::min(3u,j.value("preset",0u));appearance.step=std::clamp(j.value("step",8u),1u,64u);appearance.opacity=std::clamp(j.value("opacity",.48f),.05f,1.f);appearance.font=std::clamp(j.value("font",22.f),12.f,48.f);appearance.x=j.value("x",8.f);appearance.y=j.value("y",14.f);appearance.f10=j.value("f10",false);appearance.centerY=j.value("centerY",true);visual::hudruntime::hud.loadAppearance(appearance);}}catch(const std::exception& e){note(std::string("HUD settings load failed: ")+e.what());}
  worker=std::thread([this]{diagnostics();});}
 const char* GetName()override{return "L4D2Visual";}
 const char* GetVersion()override{return "1.0.0";}
 void OnModuleLoaded(const char* name,uintptr_t handle)override{if(!name)return;std::string n(name);if(n=="client"||n=="client.dll")source_camera::clientModule=reinterpret_cast<HMODULE>(handle);if(n=="engine"||n=="engine.dll")source_camera::engineModule=reinterpret_cast<HMODULE>(handle);note("Module loaded: "+n);connect();}
 void OnGameLaunch()override{note("OnGameLaunch");connect();}
 void OnD3DDeviceCreated(void* d,bool dxvk)override{note(std::string("OnD3DDeviceCreated dxvk=")+(dxvk?"yes":"no"));visual::hudruntime::created(static_cast<IDirect3DDevice9*>(d));visual::features::host().deviceCreated(static_cast<IDirect3DDevice9*>(d));}
 const char* RequestHudMenu(bool title)override{if(title)return "L4D2Visual 1.0.0";visual::hudruntime::hud.open();static std::string entry;std::ostringstream address;address<<"0x"<<std::hex<<reinterpret_cast<uintptr_t>(&openMenu);entry="\"L4D2Visual\" { \"打开视觉效果菜单\" { \"callback\" \""+address.str()+"\" } }";return entry.c_str();}
 void stop(){if(stopStarted.exchange(true))return;visual::features::host().quiesce();stopping=true;if(worker.joinable())worker.join();visual::features::host().stop();visual::features::disconnectRender();source_entities::combatRequested=false;visual::modelpath::apply(false);visual::hudruntime::stop();visual::render::stop();source_entities::browseRequested=false;source_camera::frameListener=nullptr;source_camera::stop();}
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

/* Optional, versioned C ABI: DangoMusic never hooks ClientMode itself.
 * Calls only accept poses after explicit user consent in the Visual menu. */
extern "C" uint32_t L4D2VISUAL_CAMERA_CALL L4D2VisualCameraGetVersion(void){
 return L4D2VISUAL_CAMERA_ABI_VERSION;
}
extern "C" int L4D2VISUAL_CAMERA_CALL L4D2VisualCameraSetExternalActive(int enabled){
 return visual::freecamera::requestExternal(enabled!=0)?1:0;
}
extern "C" int L4D2VISUAL_CAMERA_CALL L4D2VisualCameraSetPose(const L4D2VisualCameraPoseV1* pose){
 return visual::freecamera::publishExternalPose(pose)?1:0;
}
extern "C" int L4D2VISUAL_CAMERA_CALL L4D2VisualCameraGetStatus(L4D2VisualCameraStatusV1* output){
 return visual::freecamera::statusExternal(output)?1:0;
}

extern "C" int L4D2VISUAL_CAMERA_CALL L4D2VisualCameraGetPose(L4D2VisualCameraPoseV1* output){
 return visual::freecamera::getExternalPose(output)?1:0;
}

extern "C" int L4D2VISUAL_CAMERA_CALL L4D2VisualCameraSetInitialIdle(uint32_t seconds){return visual::freecamera::setExternalInitialIdle(seconds)?1:0;}
