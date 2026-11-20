#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../free_camera_runtime.hpp"
#include <optional>
#include <string>
namespace visual::freecamerafeature {
using namespace visual::ui;
inline std::optional<std::string> page(std::uintptr_t id){
 if(id!=930)return std::nullopt;
 return "\"FreeCamera · 实验\" {"+
 item(freecamera::enabled()?u8"自由摄像机：[运行中]":freecamera::requested.load()?u8"自由摄像机：[启动中]":u8"自由摄像机：[关闭]",931)+
 row(std::string("Hook: ")+(freecamera::bound?"ready":"unavailable"))+
 row(std::string("State: ")+freecamera::message.load())+
 row("Camera layout: "+std::string(freecamera::detectedLayout.load()))+
 row("Menu/F6 poll/F6 edge: "+std::to_string(freecamera::menuToggles.load())+"/"+std::to_string(freecamera::keyPolls.load())+"/"+std::to_string(freecamera::keyToggles.load()))+
 row("View/Move: "+std::to_string(freecamera::viewCount.load())+"/"+std::to_string(freecamera::cmdCount.load()))+
 row("Rejected view/cmd: "+std::to_string(freecamera::viewRejected.load())+"/"+std::to_string(freecamera::cmdRejected.load()))+
 row(u8"F6 切换；WASD/鼠标/Space/Ctrl/Shift")+
 row("Speed: "+std::to_string(int(freecamera::movementSpeed.load())))+
 item(u8"加速 +50",932)+item(u8"减速 -50",933)+
 item(u8"紧急关闭",934)+item(u8"保存设置",101)+"}";
}
inline bool action(std::uintptr_t id){
 if(id==931){++freecamera::menuToggles;freecamera::toggle();saveMessage=u8"FreeCamera 菜单开关已执行；请核对本页状态";return true;}
 if(id==932){freecamera::movementSpeed=std::min(2500.f,freecamera::movementSpeed.load()+50.f);return true;}
 if(id==933){freecamera::movementSpeed=std::max(30.f,freecamera::movementSpeed.load()-50.f);return true;}
 if(id==934){freecamera::disable("Emergency off");return true;}
 return false;
}
inline modules::Module create(){
 modules::Module m;m.id="free-camera";m.menuOrder=2;
 m.rootItem=[] {return item(freecamera::enabled()?u8"FreeCamera [运行中] · 实验":u8"FreeCamera [关] · 实验",930);};
 m.page=page;m.action=action;m.frame=freecamera::frame;
 m.pollOverlay=[](const dango::GameCameraFrame&,bool escape,bool menu,bool foreground){freecamera::poll(escape,menu,foreground);};
 m.diagnostics=[] {return "hook="+std::to_string(freecamera::bound.load())+
  " active="+std::to_string(freecamera::active.load())+
  " view="+std::to_string(freecamera::viewCount.load())+
  " move="+std::to_string(freecamera::cmdCount.load());};
 m.quiesce=[] {freecamera::disable("Module quiesced");};
 m.stop=freecamera::stop;
 return m;
}
}
