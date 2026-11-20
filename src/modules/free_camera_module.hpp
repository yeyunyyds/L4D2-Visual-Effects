#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../free_camera_runtime.hpp"
#include <optional>
#include <string>
namespace visual::freecamerafeature {
using namespace visual::ui;
inline std::optional<std::string> page(std::uintptr_t id){
 if(id!=930&&id!=936)return std::nullopt;
 if(id==930){
  // Keep the frequently adjusted options on one small page.
  return "\"FreeCamera · 设置\" {"+
   item(freecamera::enabled()?u8"自由摄像机：[运行中]":freecamera::requested.load()?u8"自由摄像机：[启动中]":u8"自由摄像机：[关闭]",931)+
   item(freecamera::f6Binding.load()?u8"F6 快捷键绑定：[开]":u8"F6 快捷键绑定：[关]（默认）",937)+
   item(freecamera::hideViewmodel.load()?u8"隐藏第一人称手臂和武器：[开]":u8"隐藏第一人称手臂和武器：[关]",935)+
   row(u8"最高飞行速度："+std::to_string(int(freecamera::movementSpeed.load())))+
   item(u8"最高速度 +50",932)+item(u8"最高速度 -50",933)+
   item(u8"运行数据与诊断",936)+
      item(u8"紧急关闭",934)+item(u8"保存设置",101)+"}";
 }
 // Observational data is collected by existing camera/render callbacks,
 // independent of which native HUD page is open; only its display moves.
 return "\"FreeCamera · 运行数据与诊断\" {"+
  row(u8"F6 可选绑定；WASD/鼠标移动，Space 上升，Shift 下降")+
  row(std::string("Runtime phase: ")+freecamera::activation::label(freecamera::currentPhase()))+
  row(std::string("ViewModel: ")+freecamera::viewModelHookStatus.load())+
  row("ViewModel checks/native-visible/hidden: "+
      std::to_string(freecamera::viewModelCalls.load())+"/"+
      std::to_string(freecamera::viewModelNativeTrue.load())+"/"+
      std::to_string(freecamera::viewModelHidden.load()))+
  row(std::string("Hook: ")+(freecamera::bound?"ready":"unavailable"))+
  row(std::string("State: ")+freecamera::message.load())+
  row(std::string("Locator: ")+freecamera::locateDetail.load())+
  row("Camera layout: "+std::string(freecamera::detectedLayout.load()))+
  row("Menu/F6 poll/F6 edge: "+std::to_string(freecamera::menuToggles.load())+"/"+
      std::to_string(freecamera::keyPolls.load())+"/"+
      std::to_string(freecamera::keyToggles.load()))+
  row("View/Move: "+std::to_string(freecamera::viewCount.load())+"/"+
      std::to_string(freecamera::cmdCount.load()))+
  row("Rejected view/cmd: "+std::to_string(freecamera::viewRejected.load())+"/"+
      std::to_string(freecamera::cmdRejected.load()))+
  row("Render steps/repeats/gaps: "+std::to_string(freecamera::renderSteps.load())+"/"+
      std::to_string(freecamera::renderRepeats.load())+"/"+
      std::to_string(freecamera::renderGaps.load()))+
  row("Render dt (ms): "+std::to_string(freecamera::renderDeltaMs.load()))+
  row("Speed current / max: "+std::to_string(int(freecamera::currentMovementSpeed.load()))+
      " / "+std::to_string(int(freecamera::movementSpeed.load())))+
  item(u8"返回 FreeCamera 设置",930)+"}";
}
inline bool action(std::uintptr_t id){
 if(id==931){++freecamera::menuToggles;freecamera::toggle();saveMessage=u8"FreeCamera 菜单开关已执行；请核对本页状态";return true;}
 if(id==932){freecamera::movementSpeed=std::min(2500.f,freecamera::movementSpeed.load()+50.f);return true;}
 if(id==933){freecamera::movementSpeed=std::max(30.f,freecamera::movementSpeed.load()-50.f);return true;}
 if(id==934){freecamera::disable("Emergency off");return true;}
 if(id==937){freecamera::f6Binding=!freecamera::f6Binding.load();saveMessage=u8"F6 绑定已更新，保存设置后持久化";return true;}
 if(id==935){
  freecamera::hideViewmodel=!freecamera::hideViewmodel.load();
  saveMessage=u8"已更改第一人称手臂/武器隐藏选项；保存设置后持久化";
  return true;
 }
 return false;
}
inline modules::Module create(){
 modules::Module m;m.id="free-camera";m.menuOrder=2;
 m.rootItem=[] {return item(freecamera::enabled()?u8"FreeCamera [运行中]":u8"FreeCamera [关]",930);};
 m.page=page;m.action=action;
 m.frame=freecamera::frame;
 m.load=[](const nlohmann::json& j){
  bool hide=true,binding=false;
  auto root=j.find("freeCamera");
  if(root!=j.end()&&root->is_object()){
   auto keybind=root->find("f6Binding");
    if(keybind!=root->end()&&keybind->is_boolean())binding=keybind->get<bool>();
    auto value=root->find("hideViewmodel");
   if(value!=root->end()&&value->is_boolean())hide=value->get<bool>();
  }
  freecamera::hideViewmodel=hide;
  freecamera::f6Binding=binding;
 };
 m.save=[](nlohmann::json& j){
  if(!j.contains("freeCamera")||!j["freeCamera"].is_object())
   j["freeCamera"]=nlohmann::json::object();
  j["freeCamera"]["hideViewmodel"]=freecamera::hideViewmodel.load();
  j["freeCamera"]["f6Binding"]=freecamera::f6Binding.load();
 };
 m.pollOverlay=[](const dango::GameCameraFrame&,bool escape,bool menu,bool foreground){freecamera::poll(escape,menu,foreground);};
 m.diagnostics=[] {return "hook="+std::to_string(freecamera::bound.load())+
  " active="+std::to_string(freecamera::active.load())+
  " hideVM="+std::to_string(freecamera::hideViewmodel.load())+
  " f6Binding="+std::to_string(freecamera::f6Binding.load())+
  " vmHook="+std::to_string(freecamera::viewModelHookBound.load())+
  " vmCalls="+std::to_string(freecamera::viewModelCalls.load())+
  " vmHidden="+std::to_string(freecamera::viewModelHidden.load())+
  " view="+std::to_string(freecamera::viewCount.load())+
  " move="+std::to_string(freecamera::cmdCount.load());};
 m.quiesce=[] {freecamera::disable("Module quiesced");};
 m.stop=freecamera::stop;
 return m;
}
}
