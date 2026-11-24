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
   item(freecamera::requested.load()?u8"FreeCamera：[开启]":freecamera::externalSession.load()?"FreeCamera: AFK pending (cancel)":u8"FreeCamera：[关闭]",931)+
   item(std::string(u8"快捷键启用：[")+(freecamera::f6Binding.load()?u8"开]":u8"关]（默认）")+"  "+freecamera::hotkey::label(freecamera::hotkeyVk.load()),937)+
   row(freecamera::hotkeyConfigValid.load()?u8"按键可在 hud.json 修改；外部会话中允许快捷键退出":u8"快捷键配置无效：已禁用，请检查 hud.json")+
   item(freecamera::hideViewmodel.load()?u8"隐藏第一人称手臂和武器：[开]":u8"隐藏第一人称手臂和武器：[关]",935)+
   item(freecamera::externalPermission.load()?u8"外部接口（ABI）：[允许]":u8"外部接口（ABI）：[关闭]（默认）",938)+
   row(u8"放开键鼠后交还外部（1-30秒）："+std::to_string(freecamera::externalIdleSeconds())+"s")+
   item(u8"交还等待 +1 秒",939)+item(u8"交还等待 -1 秒",940)+
   row(u8"同一相机：快捷键或菜单关闭即取消外部请求；键鼠优先")+
   row(std::string(u8"当前控制权：")+
       (freecamera::currentOwner.load()==freecamera::external::Owner::External?u8"Dango/外部":
        freecamera::currentOwner.load()==freecamera::external::Owner::User?u8"用户 FreeCamera":u8"原生游戏"))+

   row(u8"最高飞行速度："+std::to_string(int(freecamera::movementSpeed.load())))+
   item(u8"最高速度 +50",932)+item(u8"最高速度 -50",933)+
   item(u8"运行数据与诊断",936)+
      item(u8"紧急关闭",934)+item(u8"保存设置",101)+"}";
 }
 // Observational data is collected by existing camera/render callbacks,
 // independent of which native HUD page is open; only its display moves.
 return "\"FreeCamera · 运行数据与诊断\" {"+
  row(u8"快捷键通过 hud.json 指定；WASD/鼠标移动，Space 上升，Shift 下降")+
  row(std::string("Runtime phase: ")+freecamera::activation::label(freecamera::currentPhase()))+
  row(std::string("ViewModel: ")+freecamera::viewModelHookStatus.load())+
  row("ViewModel checks/native-visible/hidden: "+
      std::to_string(freecamera::viewModelCalls.load())+"/"+
      std::to_string(freecamera::viewModelNativeTrue.load())+"/"+
      std::to_string(freecamera::viewModelHidden.load()))+
  row(std::string("Hook: ")+(freecamera::bound?"ready":"unavailable"))+
  row("External ABI: "+std::string(freecamera::externalPermission?"allowed":"blocked")+
      " / owner="+std::to_string(static_cast<unsigned>(freecamera::currentOwner.load()))+
      " / idle="+std::to_string(freecamera::externalIdleSeconds())+"s")+

  row(std::string("State: ")+freecamera::message.load())+
  row(std::string("Locator: ")+freecamera::locateDetail.load())+
  row("Camera layout: "+std::string(freecamera::detectedLayout.load()))+
  row("Menu/Hotkey poll/Hotkey edge: "+std::to_string(freecamera::menuToggles.load())+"/"+
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
  row("Mouse samples/zero: "+std::to_string(freecamera::mouseSamples.load())+"/"+std::to_string(freecamera::mouseZeroSamples.load()))+
  row("Server cmds / extra mouse samples / extra with input: "+
      std::to_string(freecamera::serverCmdSamples.load())+"/"+
      std::to_string(freecamera::extraMouseSamples.load())+"/"+
      std::to_string(freecamera::extraMouseNonzero.load()))+
  row("Mouse X received/applied/dropped: "+std::to_string(freecamera::mouseReceivedX.load())+"/"+std::to_string(freecamera::mouseAppliedX.load())+"/"+std::to_string(freecamera::mouseDroppedX.load()))+
  row("Mouse Y received/applied/dropped: "+std::to_string(freecamera::mouseReceivedY.load())+"/"+std::to_string(freecamera::mouseAppliedY.load())+"/"+std::to_string(freecamera::mouseDroppedY.load()))+
  row("Mouse pending X/Y: "+std::to_string(freecamera::queuedMouse().x)+"/"+std::to_string(freecamera::queuedMouse().y))+
  row("Speed current / max: "+std::to_string(int(freecamera::currentMovementSpeed.load()))+
      " / "+std::to_string(int(freecamera::movementSpeed.load())))+
  item(u8"返回 FreeCamera 设置",930)+"}";
}
inline bool action(std::uintptr_t id){
 if(id==931){++freecamera::menuToggles;freecamera::toggle();saveMessage=u8"FreeCamera 菜单开关已执行；请核对本页状态";return true;}
 if(id==932){freecamera::movementSpeed=std::min(2500.f,freecamera::movementSpeed.load()+50.f);return true;}
 if(id==933){freecamera::movementSpeed=std::max(30.f,freecamera::movementSpeed.load()-50.f);return true;}
 if(id==934){freecamera::disable("Emergency off");return true;}
 if(id==938){freecamera::allowExternal(!freecamera::externalPermission.load());saveMessage=u8"外部接口授权已更改；保存设置后持久化";return true;}
 if(id==939){freecamera::setExternalIdleSeconds(freecamera::externalIdleSeconds()+1);return true;}
 if(id==940){freecamera::setExternalIdleSeconds(freecamera::externalIdleSeconds()-1);return true;}
 if(id==937){freecamera::f6Binding=!freecamera::f6Binding.load();saveMessage=u8"快捷键启用状态已更新，保存设置后持久化";return true;}
 if(id==935){
  freecamera::hideViewmodel=!freecamera::hideViewmodel.load();
  saveMessage=u8"已更改第一人称手臂/武器隐藏选项；保存设置后持久化";
  return true;
 }
 return false;
}
inline modules::Module create(){
 modules::Module m;m.id="free-camera";m.menuOrder=4;
 m.rootItem=[] {return item(freecamera::enabled()?u8"FreeCamera [运行中]":u8"FreeCamera [关]",930);};
 m.page=page;m.action=action;
 m.frame=freecamera::frame;
 m.load=[](const nlohmann::json& j){
  bool hide=true,binding=false,valid=true,externalAllowed=false;
  unsigned idleSeconds=3;
  int keycode=freecamera::hotkey::defaultVk;
  auto root=j.find("freeCamera");
  if(root!=j.end()&&root->is_object()){
   auto keybind=root->find("f6Binding");
   if(keybind!=root->end()&&keybind->is_boolean())binding=keybind->get<bool>();
   auto value=root->find("hideViewmodel");
   if(value!=root->end()&&value->is_boolean())hide=value->get<bool>();
   auto allowed=root->find("externalEnabled");
   if(allowed!=root->end()&&allowed->is_boolean())externalAllowed=allowed->get<bool>();
   auto timeout=root->find("externalIdleSeconds");
   if(timeout!=root->end()&&timeout->is_number_integer()){
    const auto value=timeout->get<int>();idleSeconds=static_cast<unsigned>(std::clamp(value,1,30));
   }
   auto configured=root->find("hotkey");
   if(configured!=root->end()){
    const auto parsed=configured->is_string()?
      freecamera::hotkey::parse(configured->get<std::string>()):std::optional<int>{};
    if(parsed)keycode=*parsed;
    else valid=false; // Unknown bindings must never silently activate F6.
   }
  }
  freecamera::hideViewmodel=hide;
  freecamera::hotkeyVk=keycode;
  freecamera::hotkeyConfigValid=valid;
  freecamera::f6Binding=binding;
  freecamera::setExternalIdleSeconds(idleSeconds);
  freecamera::allowExternal(externalAllowed);
 };
 m.save=[](nlohmann::json& j){
  if(!j.contains("freeCamera")||!j["freeCamera"].is_object())
   j["freeCamera"]=nlohmann::json::object();
  j["freeCamera"]["hideViewmodel"]=freecamera::hideViewmodel.load();
  j["freeCamera"]["f6Binding"]=freecamera::f6Binding.load();
  j["freeCamera"]["externalEnabled"]=freecamera::externalPermission.load();
  j["freeCamera"]["externalIdleSeconds"]=freecamera::externalIdleSeconds();
  j["freeCamera"]["hotkey"]=freecamera::hotkeyConfigValid.load()?
   freecamera::hotkey::normalize(freecamera::hotkey::label(freecamera::hotkeyVk.load())):
   "invalid";
 };
 m.pollOverlay=[](const dango::GameCameraFrame&,bool escape,bool menu,bool foreground){freecamera::poll(escape,menu,foreground);};
 m.diagnostics=[] {return "hook="+std::to_string(freecamera::bound.load())+
  " active="+std::to_string(freecamera::active.load())+
  " externalAllowed="+std::to_string(freecamera::externalPermission.load())+
  " owner="+std::to_string(static_cast<unsigned>(freecamera::currentOwner.load()))+
  " externalIdleSeconds="+std::to_string(freecamera::externalIdleSeconds())+
  " hideVM="+std::to_string(freecamera::hideViewmodel.load())+
  " hotkeyEnabled="+std::to_string(freecamera::f6Binding.load())+
  " hotkey="+freecamera::hotkey::label(freecamera::hotkeyVk.load())+
  " hotkeyValid="+std::to_string(freecamera::hotkeyConfigValid.load())+
  " vmHook="+std::to_string(freecamera::viewModelHookBound.load())+
  " extraSamples="+std::to_string(freecamera::extraMouseSamples.load())+
  " extraNonzero="+std::to_string(freecamera::extraMouseNonzero.load())+
  " serverCmd="+std::to_string(freecamera::serverCmdSamples.load())+
  " mouseRX="+std::to_string(freecamera::mouseReceivedX.load())+
  " mouseAX="+std::to_string(freecamera::mouseAppliedX.load())+
  " mouseDX="+std::to_string(freecamera::mouseDroppedX.load())+
  " mouseRY="+std::to_string(freecamera::mouseReceivedY.load())+
  " mouseAY="+std::to_string(freecamera::mouseAppliedY.load())+
  " mouseDY="+std::to_string(freecamera::mouseDroppedY.load())+
  " vmCalls="+std::to_string(freecamera::viewModelCalls.load())+
  " vmHidden="+std::to_string(freecamera::viewModelHidden.load())+
  " view="+std::to_string(freecamera::viewCount.load())+
  " move="+std::to_string(freecamera::cmdCount.load());};
 m.quiesce=[] {freecamera::disable("Module quiesced");};
 m.stop=freecamera::stop;
 return m;
}
}
