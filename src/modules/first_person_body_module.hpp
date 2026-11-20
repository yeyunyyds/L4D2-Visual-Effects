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
 if(id!=860&&id!=862&&id!=863&&id!=864&&id!=865)return std::nullopt;
if(id==860){auto s=std::atomic_load(&visual::firstperson::options);return "\"Evo 风格 · 第一人称身体设置\" {"+
  item(s->enabled?u8"身体渲染：[开]":u8"身体渲染：[关]",861)+
  item(s->tltpvCompatibility?u8"TLTPV 第三人称兼容：[开]":u8"TLTPV 第三人称兼容：[关]（默认）",886)+
  item(s->hideHead?u8"头部骨骼隐藏：[开]":u8"头部骨骼隐藏：[关]",884)+
  item(s->hideArms?u8"第三人称手臂隐藏：[开]":u8"第三人称手臂隐藏：[关]",885)+
  item(s->dynamicAnchor?u8"动态身体定位 [开] · 校准":u8"动态身体定位 [关] · 校准",865)+
  item(s->clipEnabled?u8"旧GPU高度裁剪：[开]（实验）":u8"旧GPU高度裁剪：[关]（实验）",862)+
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
   item(s->followCameraYaw?u8"胸肩朝向跟随视角：[开]（实验）":u8"胸肩朝向跟随视角：[关]",888)+
   row(u8"本机 MOD："+modelName)+row(saved)+
   row(u8"眼位失效回退高度："+value(s->fallbackEyeHeight))+
   item(u8"回退高度 +1",889)+item(u8"回退高度 -1",890)+
   row(u8"手动朝向补偿："+value(s->yawTrim)+u8" 度")+
   item(u8"朝向 +5 度",891)+item(u8"朝向 -5 度",892)+
   item(u8"绑定当前XYZ和朝向到本MOD",894)+
   item(u8"重置当前MOD独立校准",895)+item(u8"保存设置",101)+"}";
 }
 if(id==862){auto s=std::atomic_load(&visual::firstperson::options);
  return "\"第一人称身体 · 高度裁剪\" {"+
  item(s->clipEnabled?u8"高度裁剪：[开]":u8"高度裁剪：[关]",870)+
  row(u8"裁剪高度："+value(s->clipHeight)+u8" 单位（相对模型原点）")+
  item(u8"裁剪高度 -1",871)+item(u8"裁剪高度 +1",872)+
  item(u8"裁剪高度 -5",873)+item(u8"裁剪高度 +5",874)+
  item(u8"恢复裁剪默认高度 65",875)+item(u8"保存设置",101)+"}";}
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
  row(std::string("TLTPV: ")+visual::tltpv::status.load()+
      " c_thirdpersonshoulder="+std::to_string(visual::tltpv::shoulderValue.load()))+
  row("Native gate active/enables/restores/failures: "+std::to_string(visual::firstperson::native::active.load())+"/"+std::to_string(visual::firstperson::native::enables.load())+"/"+std::to_string(visual::firstperson::native::restores.load())+"/"+std::to_string(visual::firstperson::native::failures.load()))+
  row("Body Studio matches/shifts/rejects: "+std::to_string(visual::firstperson::effects::matched.load())+"/"+std::to_string(visual::firstperson::effects::shifted.load())+"/"+std::to_string(visual::firstperson::effects::shiftRejected.load()))+
  row("Head branch applied/rejected: "+std::to_string(visual::firstperson::effects::headHidden.load())+"/"+std::to_string(visual::firstperson::effects::headRejected.load()))+
  row("Arm branch applied/rejected: "+std::to_string(visual::firstperson::effects::armsHidden.load())+"/"+std::to_string(visual::firstperson::effects::armsRejected.load()))+
  row("Dynamic applied/rejected: "+std::to_string(visual::firstperson::effects::dynamicApplied.load())+"/"+std::to_string(visual::firstperson::effects::dynamicRejected.load()))+
  row("Live camera success/failure: "+std::to_string(visual::firstperson::effects::sameFrameCamera.load())+"/"+std::to_string(visual::firstperson::effects::liveCameraRejected.load()))+
  row(visual::firstperson::effects::dynamicStatus.load())+row(visual::firstperson::effects::dynamicSource.load())+
  row(visual::firstperson::effects::headStatus.load())+row(visual::firstperson::effects::armStatus.load())+
  row("Legacy GPU clip set/rejected: "+std::to_string(visual::firstperson::effects::clipSet.load())+"/"+std::to_string(visual::firstperson::effects::clipRejected.load()))+
  row(visual::firstperson::effects::clipStatus.load())+row(visual::firstperson::effects::offsetStatus.load())+
  row("ShouldDraw local/true/false: "+std::to_string(visual::firstperson::localCalls.load())+"/"+std::to_string(visual::firstperson::nativeTrue.load())+"/"+std::to_string(visual::firstperson::nativeFalse.load()))+
  row(u8"渲染效果与第三方MOD仍需实机确认")+"}";}

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
 if((id>=870&&id<=893)){
  auto s=*std::atomic_load(&visual::firstperson::options);
  if(visual::firstperson::adjust(s,id)){
   std::atomic_store(&visual::firstperson::options,std::make_shared<const visual::firstperson::Settings>(s));
   saveMessage=u8"第一人称身体隐藏/裁剪/位置已更新，设置未保存";
   return true;
  }
 }
 if(id==861){auto s=*std::atomic_load(&visual::firstperson::options);s.enabled=!s.enabled;std::atomic_store(&visual::firstperson::options,std::make_shared<const visual::firstperson::Settings>(s));if(!s.enabled)visual::firstperson::native::disable();saveMessage=s.enabled?u8"第一人称身体已请求开启，正在验证本机玩家；可通过诊断查看状态":u8"第一人称身体已关闭，原生可见性恢复";note(s.enabled?"FirstPersonBody menu toggle: ON requested":"FirstPersonBody menu toggle: OFF requested");return true;}
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
 m.needsEntities=[] {return std::atomic_load(&firstperson::options)->enabled;};
 m.frame=[](const dango::GameCameraFrame& f){
  if(freecamera::enabled()){firstperson::native::disable();return;}
  firstperson::onFrame(f);
 };
 m.deviceCreated=[](IDirect3DDevice9* d){firstperson::effects::setDevice(d);};
 // At this point the shared Studio pipeline has already applied any entity
 // deformation. First-person effects decorate the FINAL draw only.
 m.studioDraw=[](const void* info,Matrix3x4* bones,const dango::Vec3& origin,
                  void* context,modules::StudioNext next){
  if(freecamera::enabled()){next(context,bones,origin);return;}
  firstperson::effects::draw(info,bones,origin,[&](Matrix3x4* transformed,const dango::Vec3& adjusted){
   next(context,transformed,adjusted);
  });
 };
 m.diagnostics=[] {return "body="+std::to_string(std::atomic_load(&firstperson::options)->enabled)+
  " TLTPV="+std::string(tltpv::label(tltpv::mode.load()))+
  " native="+std::to_string(firstperson::native::active.load())+
  " aligned="+std::to_string(firstperson::effects::dynamicApplied.load())+
  " rejected="+std::to_string(firstperson::effects::dynamicRejected.load());};
 m.quiesce=[] {firstperson::native::disable();};
 m.stop=[] {firstperson::stop();firstperson::effects::stop();};
 return m;
}
}
