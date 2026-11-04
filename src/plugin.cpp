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
static_assert(sizeof(void*)==4,"L4N requires Windows x86");
namespace {
std::atomic<bool> requested{false},saveRequested{false};
std::atomic<const char*> saveMessage{""};
std::mutex appearanceMutex;
dango::hud::Appearance appearance;
void note(const std::string& s){visual::diagnostics::write(s);}
void onFrame(const dango::GameCameraFrame& frame){visual::modelpath::apply(requested&&frame.active);source_entities::onFrame(frame);}
void toggle(){bool value=!requested.load();requested=value;source_entities::browseRequested=value;visual::render::startedAt=GetTickCount64();visual::render::enabled=value;note(value?"Stretch enabled":"Stretch disabled");}
std::string row(const std::string& label,uintptr_t id=0){return "\""+label+"\" { \"user_data\" \""+std::to_string(id)+"\" }\n";}
// Native menu parser expects hexadecimal action values.
std::string item(const std::string& label,uintptr_t id){std::ostringstream s;s<<"0x"<<std::hex<<id;return "\""+label+"\" { \"user_data\" \""+s.str()+"\" }\n";}
std::string decimal(int tenths){return std::to_string(tenths/10)+"."+std::to_string(tenths%10);}
std::string buildMenu(uintptr_t id){
 auto parameters=std::atomic_load(&visual::render::animation);
 auto f=std::atomic_load(&source_entities::snapshot);
 if(id==102){return "\"动画细调\" {"+
 item(u8"动画预设："+std::string(visual::presetName(parameters->preset)),116)+
 item(u8"往复周期 -0.1 秒："+decimal(int(parameters->periodMs/100))+u8" 秒",110)+
 item(u8"往复周期 +0.1 秒",111)+
 item(u8"最小比例 -0.1："+decimal(parameters->minTenths),112)+item(u8"最小比例 +0.1",113)+
 item(u8"最大比例 -0.1："+decimal(parameters->maxTenths),114)+item(u8"最大比例 +0.1",115)+
 item(u8"恢复动画默认：0.5秒 / 0.5～1.1",117)+item(u8"保存设置",101)+"}";}
 if(id==24){std::string body=row(visual::render::status.load());body+=row("Camera: "+std::string(source_camera::status()));body+=row("Entities: "+f->status);body+=row("Identify: "+std::string(visual::render::identifyReason.load()));body+=row("Pose: "+std::string(visual::render::poseReason.load()));body+=row("Engine/Studio: "+std::to_string(visual::render::engineCalls.load())+" / "+std::to_string(visual::render::studioCalls.load()));body+=row("Matched/Drawn: "+std::to_string(visual::render::identified.load())+" / "+std::to_string(visual::render::drawn.load()));body+=row(visual::diagnostics::writable?u8"日志已写入 L4D2Visual/L4D2Visual.log":u8"日志写入失败，请检查目录权限");return "\"L4D2Visual · 诊断\" {"+body+"}";}
 return "\"L4D2Visual 0.4.0\" {"+item(requested?u8"拉伸测试：开":u8"拉伸测试：关",100)+item(u8"动画预设与细调",102)+row(u8"周期 "+decimal(int(parameters->periodMs/100))+u8" 秒；比例 "+decimal(parameters->minTenths)+" - "+decimal(parameters->maxTenths))+row(u8"保留双脚和脚趾姿态")+item(u8"接口与日志诊断",24)+item(u8"菜单外观",9000)+item(u8"保存设置",101)+row(u8"已拉伸绘制："+std::to_string(visual::render::drawn.load()))+"}";
}
void action(uintptr_t id){if(id==100)toggle();if(id>=110&&id<=117){auto a=*std::atomic_load(&visual::render::animation);a=visual::adjustAnimation(a,unsigned(id));std::atomic_store(&visual::render::animation,std::make_shared<const visual::AnimationSettings>(a));visual::render::startedAt=GetTickCount64();saveMessage=u8"动画设置未保存";}if(id==101){saveRequested=true;saveMessage=u8"已提交保存";}}
void closed(){}
std::string notice(){return saveMessage.load();}
void appearanceChanged(const dango::hud::Appearance& a){std::lock_guard<std::mutex> l(appearanceMutex);appearance=a;saveMessage=u8"菜单外观未保存";}
const char* __cdecl openMenu(void*){visual::hudruntime::hud.open();return nullptr;}
class Plugin final:public IL4NPlugin {
 std::atomic<bool> stopping{false};std::thread worker;
 void connect(){source_camera::logger=note;source_camera::frameListener=onFrame;if(!source_camera::clientModule)source_camera::clientModule=GetModuleHandleW(L"client.dll");if(!source_camera::engineModule)source_camera::engineModule=GetModuleHandleW(L"engine.dll");source_camera::connect();visual::render::connect();}
 void diagnostics(){
  std::string previous;ULONGLONG last=0;
  while(!stopping){
   if(saveRequested.exchange(false)){try{dango::hud::Appearance a;{std::lock_guard<std::mutex> l(appearanceMutex);a=appearance;}auto anim=std::atomic_load(&visual::render::animation);nlohmann::json j={{"preset",a.preset},{"step",a.step},{"opacity",a.opacity},{"font",a.font},{"x",a.x},{"y",a.y},{"f10",a.f10},{"centerY",a.centerY}};visual::writeAnimationSettings(j,*anim);auto path=visual::diagnostics::path.parent_path()/L"hud.json";auto tmp=path;tmp+=L".tmp";{std::ofstream o(tmp);o<<j.dump(2);o.flush();if(!o)throw std::runtime_error("HUD settings write failed");}if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("HUD settings rename failed");saveMessage=u8"菜单设置已保存";note("HUD appearance saved");}catch(const std::exception& e){saveMessage=u8"保存失败，请查看日志";note(e.what());}}
   auto f=std::atomic_load(&source_entities::snapshot);std::ostringstream state;state<<"enabled="<<requested<<" render="<<visual::render::status.load()<<" camera="<<source_camera::status()<<" entities="<<f->status<<" records="<<f->records.size()<<" modelLookup="<<f->modelLookup<<" engine="<<visual::render::engineCalls<<" studio="<<visual::render::studioCalls<<" matched="<<visual::render::identified<<" drawn="<<visual::render::drawn<<" identify="<<visual::render::identifyReason.load()<<" pose="<<visual::render::poseReason.load()<<" fastpath="<<visual::modelpath::current<<" periodMs="<<std::atomic_load(&visual::render::animation)->periodMs<<" HUD="<<visual::hudruntime::paintStatus.load()<<" presents="<<visual::hudruntime::presents<<" hudFrames="<<visual::hudruntime::frames;
   auto now=GetTickCount64();if(now-last>=2000 && (requested||state.str()!=previous)){note(state.str());for(const auto& check:f->bindingChecks)note("Entity binding: "+check);std::map<std::string,unsigned> inventory;
    for(const auto& r:f->records)if(visual::eligibleRecord(r))++inventory[r.className];
    std::string classes;for(const auto& pair:inventory)classes+=" "+pair.first+"="+std::to_string(pair.second);
    note("Special-infected inventory:"+classes);
    note("Per-target draw stats:"+visual::render::targetSummary());previous=state.str();last=now;}
   std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
 }
public:
 Plugin(){note("=== L4D2Visual 0.4.0 startup ===");note("DLL directory: "+visual::diagnostics::path.parent_path().parent_path().u8string());source_camera::logger=note;visual::hudruntime::hud.configure(buildMenu,action,closed,notice,note,appearanceChanged);
  try{std::ifstream file(visual::diagnostics::path.parent_path()/L"hud.json");if(file){nlohmann::json j;file>>j;std::atomic_store(&visual::render::animation,std::make_shared<const visual::AnimationSettings>(visual::readAnimationSettings(j)));appearance.preset=std::min(3u,j.value("preset",0u));appearance.step=std::clamp(j.value("step",8u),1u,64u);appearance.opacity=std::clamp(j.value("opacity",.48f),.05f,1.f);appearance.font=std::clamp(j.value("font",22.f),12.f,48.f);appearance.x=j.value("x",8.f);appearance.y=j.value("y",14.f);appearance.f10=j.value("f10",false);appearance.centerY=j.value("centerY",true);visual::hudruntime::hud.loadAppearance(appearance);}}catch(const std::exception& e){note(std::string("HUD settings load failed: ")+e.what());}
  worker=std::thread([this]{diagnostics();});}
 const char* GetName()override{return "L4D2Visual";}
 const char* GetVersion()override{return "0.4.0";}
 void OnModuleLoaded(const char* name,uintptr_t handle)override{if(!name)return;std::string n(name);if(n=="client"||n=="client.dll")source_camera::clientModule=reinterpret_cast<HMODULE>(handle);if(n=="engine"||n=="engine.dll")source_camera::engineModule=reinterpret_cast<HMODULE>(handle);note("Module loaded: "+n);connect();}
 void OnGameLaunch()override{note("OnGameLaunch");connect();}
 void OnD3DDeviceCreated(void* d,bool dxvk)override{note(std::string("OnD3DDeviceCreated dxvk=")+(dxvk?"yes":"no"));visual::hudruntime::created(static_cast<IDirect3DDevice9*>(d));}
 const char* RequestHudMenu(bool title)override{if(title)return "L4D2Visual 0.4.0";visual::hudruntime::hud.open();static std::string entry;std::ostringstream address;address<<"0x"<<std::hex<<reinterpret_cast<uintptr_t>(&openMenu);entry="\"L4D2Visual\" { \"打开拉伸控制面板\" { \"callback\" \""+address.str()+"\" } }";return entry.c_str();}
 void stop(){stopping=true;if(worker.joinable())worker.join();requested=false;visual::modelpath::apply(false);visual::hudruntime::stop();visual::render::stop();source_entities::browseRequested=false;source_camera::frameListener=nullptr;source_camera::stop();}
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
