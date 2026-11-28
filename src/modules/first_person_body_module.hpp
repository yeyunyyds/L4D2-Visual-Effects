#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../first_person_body_effects.hpp"
#include "../free_camera_runtime.hpp"
#include "../diagnostics.hpp"
#include <cstdio>
#include <optional>
#include <string>
#include <atomic>
#include <memory>
#include <utility>

namespace visual::firstpersonfeature {
using namespace visual::ui;
inline std::string value(float number){char buffer[32]{};std::snprintf(buffer,sizeof(buffer),"%.1f",double(number));return buffer;}
inline void note(const std::string& line){diagnostics::write(line);}
inline std::optional<std::string> page(std::uintptr_t id){
 if(id!=860&&id!=863&&id!=864&&id!=865)return std::nullopt;
if(id==860){auto s=std::atomic_load(&visual::firstperson::options);return "\"第一人称身体 · 设置\" {"+
  item(s->enabled?u8"身体渲染：[开]":u8"身体渲染：[关]",861)+
  item(s->thirdPersonCompatibility?u8"第三人称视角兼容：[开]":u8"第三人称视角兼容：[关]",886)+
  item(s->hideHead?u8"头部骨骼隐藏：[开]":u8"头部骨骼隐藏：[关]",884)+
  item(s->hideArms?u8"第三人称手臂隐藏：[开]":u8"第三人称手臂隐藏：[关]",885)+
  item(s->dynamicAnchor?u8"动态身体定位 [开] · 校准":u8"动态身体定位 [关] · 校准",865)+
  item(u8"模型位置 · 前后/左右/高度",863)+
  item(u8"运行状态与诊断",864)+item(u8"保存设置",101)+"}";}
 if(id==865){auto s=std::atomic_load(&visual::firstperson::options);
  auto model=visual::firstperson::effects::activeModel();
  std::string modelName=model?model->name:std::string(u8"尚未识别本机MOD");
  if(modelName.size()>54)modelName=modelName.substr(modelName.size()-54);
  auto p=model?visual::firstperson::profile(*s,model->name,model->checksum):nullptr;
  std::string saved=p?u8"本MOD校准已保存":u8"本MOD没有单独校准";
  return "\"第一人称身体 · 动态锚点\" {"+
   item(s->dynamicAnchor?u8"动态身体定位：[开]":u8"动态身体定位：[关]",887)+
   item(s->followCameraYaw?u8"胸肩朝向跟随视角：[开]":u8"胸肩朝向跟随视角：[关]",888)+
   row(u8"本机 MOD："+modelName)+row(saved)+
   row(u8"眼位失效回退高度："+value(s->fallbackEyeHeight))+
   item(u8"回退高度 +1",889)+item(u8"回退高度 -1",890)+
   row(u8"手动朝向补偿："+value(s->yawTrim)+u8" 度")+
   item(u8"朝向 +5 度",891)+item(u8"朝向 -5 度",892)+
   item(u8"绑定当前XYZ和朝向到本MOD",894)+
   item(u8"重置当前MOD独立校准",895)+item(u8"保存设置",101)+"}";
 }
 if(id==863){auto s=std::atomic_load(&visual::firstperson::options);
  return "\"第一人称身体 · 模型位置\" {"+
  row(u8"当前前后/左右/高度："+value(s->forward)+" / "+value(s->right)+" / "+value(s->up))+
  item(u8"前移 +"+value(s->step),876)+item(u8"后移 -"+value(s->step),877)+
  item(u8"右移 +"+value(s->step),878)+item(u8"左移 -"+value(s->step),879)+
  item(u8"抬高 +"+value(s->step),880)+item(u8"降低 -"+value(s->step),881)+
  item(u8"调节步长："+value(s->step),882)+item(u8"重置模型位置",883)+item(u8"保存设置",101)+"}";}
 if(id==864){auto s=std::atomic_load(&visual::firstperson::options);
  return "\"第一人称身体 · 运行诊断\" {"+
  row(visual::firstperson::status.load())+
  row(std::string("View: ")+visual::viewcompat::status.load()+
      " shoulder="+std::to_string(visual::viewcompat::shoulderValue.load())+
      " xy="+std::to_string(visual::viewcompat::horizontalUnits.load())+
      " eyeZ="+std::to_string(visual::viewcompat::eyeHeightUnits.load()))+
  row("FreeCamera world-anchored body: "+std::to_string(visual::firstperson::worldBodyActive.load()))+
  row("Native gate active/enables/restores/failures: "+std::to_string(visual::firstperson::native::active.load())+"/"+std::to_string(visual::firstperson::native::enables.load())+"/"+std::to_string(visual::firstperson::native::restores.load())+"/"+std::to_string(visual::firstperson::native::failures.load()))+
  row("Native gate phase: "+std::string(visual::firstperson::gatepolicy::label(visual::firstperson::native::phase.load())))+
  row("Native gate restoration verified: "+std::to_string(visual::firstperson::native::restoredVerified.load()))+
  row(std::string("Native gate detail: ")+visual::firstperson::native::state.load())+
  row("Body Studio matches/shifts/rejects: "+std::to_string(visual::firstperson::effects::matched.load())+"/"+std::to_string(visual::firstperson::effects::shifted.load())+"/"+std::to_string(visual::firstperson::effects::shiftRejected.load()))+
  row("Head branch applied/rejected: "+std::to_string(visual::firstperson::effects::headHidden.load())+"/"+std::to_string(visual::firstperson::effects::headRejected.load()))+
  row("Arm branch applied/rejected: "+std::to_string(visual::firstperson::effects::armsHidden.load())+"/"+std::to_string(visual::firstperson::effects::armsRejected.load()))+
  row("Dynamic applied/rejected: "+std::to_string(visual::firstperson::effects::dynamicApplied.load())+"/"+std::to_string(visual::firstperson::effects::dynamicRejected.load()))+
  row("Live camera success/failure: "+std::to_string(visual::firstperson::effects::sameFrameCamera.load())+"/"+std::to_string(visual::firstperson::effects::liveCameraRejected.load()))+
  row(visual::firstperson::effects::dynamicStatus.load())+row(visual::firstperson::effects::dynamicSource.load())+
  row(visual::firstperson::effects::headStatus.load())+row(visual::firstperson::effects::armStatus.load())+
  row(visual::firstperson::effects::offsetStatus.load())+
  row(u8"模型MOD与游戏版本的兼容性取决于实际环境")+"}";}

 return std::nullopt;
}
inline bool action(std::uintptr_t id){
 if(id==894||id==895){
  auto model=visual::firstperson::effects::activeModel();
  if(!model){saveMessage=u8"未识别到当前本机MOD；先进入游戏";return true;}
  auto s=*std::atomic_load(&visual::firstperson::options);
  bool ok=id==894?visual::firstperson::storeForModel(s,model->name,model->checksum):
                 visual::firstperson::eraseForModel(s,model->name,model->checksum);
  if(ok){
   std::atomic_store(&visual::firstperson::options,
     std::make_shared<const visual::firstperson::Settings>(std::move(s)));
   saveMessage=id==894?u8"当前XYZ与朝向校准已绑定到本MOD；请再点保存设置":
                       u8"已重置当前MOD的独立校准；请再点保存设置";
  }else saveMessage=u8"没有可更新的当前MOD校准";
  return true;
 }
 if(id>=876&&id<=893){
  auto s=*std::atomic_load(&visual::firstperson::options);
  if(visual::firstperson::adjust(s,id)){
   if(id==886||id==887){++firstperson::cosmeticGeneration;note(id==886?(s.thirdPersonCompatibility?"FPB_COMPAT ON":"FPB_COMPAT OFF"):(s.dynamicAnchor?"FPB_DYNAMIC ON":"FPB_DYNAMIC OFF"));}
   std::atomic_store(&visual::firstperson::options,std::make_shared<const visual::firstperson::Settings>(s));
   saveMessage=u8"第一人称身体设置已更新，尚未保存";
   return true;
  }
 }
 if(id==861){auto s=*std::atomic_load(&visual::firstperson::options);s.enabled=!s.enabled;
  std::atomic_store(&visual::firstperson::options,std::make_shared<const visual::firstperson::Settings>(s));
  ++firstperson::cosmeticGeneration;
  bool restored=true;if(!s.enabled)restored=visual::firstperson::native::disable();
  saveMessage=s.enabled?u8"第一人称身体已请求开启，正在验证本机玩家":
    restored?u8"第一人称身体已关闭，原生绘制状态已恢复":
    u8"身体功能已关闭，但原生补丁恢复未确认；请查看诊断并完全退出游戏";
  note(s.enabled?"FirstPersonBody menu toggle: ON requested":"FirstPersonBody menu toggle: OFF requested");return true;
 }
 return false;
}
inline modules::Module create(){
 modules::Module m;m.id="first-person-body";m.menuOrder=1;
 m.rootItem=[]{
  return item(std::atomic_load(&firstperson::options)->enabled?
   u8"第一人称身体 [开] · 设置":u8"第一人称身体 [关] · 设置",860);
 };
 m.page=page;
 m.action=action;
 m.load=[](const nlohmann::json& j){
  std::atomic_store(&firstperson::options,
   std::make_shared<const firstperson::Settings>(firstperson::readSettings(j)));
 };
 m.save=[](nlohmann::json& j){
  firstperson::writeSettings(j,*std::atomic_load(&firstperson::options));
 };
 // Local player data is requested separately; FPB does not need the full inventory.
 m.frame=[](const dango::GameCameraFrame& frame) {
  // Keep the real player entity rendered while FreeCamera observes it.
  // onFrame still verifies the local handle/model/life state before opening
  // the original DrawModel gate; no extra entities or render calls are made.
  firstperson::onFrame(frame,freecamera::requested.load(std::memory_order_acquire),freecamera::requested.load()&&freecamera::externalSession.load()&&freecamera::bodyClear.load());
 };
 // At this point the shared Studio pipeline has already applied any entity
 // deformation. First-person effects decorate the FINAL draw only.
 m.studioDraw=[](const void* info,Matrix3x4* bones,const dango::Vec3& origin,
                  void* context,modules::StudioNext next){
  // No model metadata, pointer reads or pose processing while FPB is off.
  if(!std::atomic_load(&firstperson::options)->enabled||
     !firstperson::native::active.load(std::memory_order_acquire)){
   next(context,bones,origin);return;
  }
  if(firstperson::freecamerabody::passOriginalStudio(
        freecamera::requested.load(std::memory_order_acquire))){
   // Scheme A: do not move, rotate or hide the native player skeleton to
   // match the detached camera. Keep the model at its true game-world pose.
   next(context,bones,origin);return;
  }
  firstperson::effects::draw(info,bones,origin,[&](Matrix3x4* transformed,const dango::Vec3& adjusted){
   next(context,transformed,adjusted);
  });
 };
 m.diagnostics=[] {return "body="+std::to_string(std::atomic_load(&firstperson::options)->enabled)+
  " view="+std::string(viewcompat::label(viewcompat::mode.load()))+
  " native="+std::to_string(firstperson::native::active.load())+
  " worldBody="+std::to_string(firstperson::worldBodyActive.load())+
  " aligned="+std::to_string(firstperson::effects::dynamicApplied.load())+
  " rejected="+std::to_string(firstperson::effects::dynamicRejected.load());};
 m.quiesce=[] {firstperson::native::disable();};
 m.stop=[] {firstperson::stop();};
 return m;
}
}
