#pragma once
// Render-only first-person body effects. Reuses the existing Studio hook.
#include "first_person_body.hpp"
#include "first_person_body_math.hpp"
#include "studio_cache.hpp"
#include "first_person_body_parts.hpp"
#include "first_person_dynamic_alignment.hpp"
#include "first_person_distance_probe.hpp"
#include <atomic>
#include <memory>
#include <cmath>
#include <cstring>
namespace visual::firstperson::effects {
inline std::atomic<unsigned long> matched{0},shifted{0},shiftRejected{0};
inline std::atomic<const char*> offsetStatus{u8"位置偏移为零"};
inline std::atomic<const char*> headStatus{u8"头部分支隐藏关闭"},armStatus{u8"双臂分支隐藏关闭"};
inline std::atomic<unsigned long> headHidden{0},headRejected{0},armsHidden{0},armsRejected{0};
inline std::atomic<unsigned long> dynamicApplied{0},dynamicRejected{0},sameFrameCamera{0},liveCameraRejected{0};
inline std::atomic<const char*> dynamicStatus{u8"动态定位关闭"},dynamicSource{"unavailable"};
struct ModelIdentity{std::string name;std::uint32_t checksum=0,epoch=0;};
inline std::shared_ptr<const ModelIdentity> modelIdentity=std::make_shared<ModelIdentity>();
inline std::atomic<std::uint64_t> modelSeenAt{0};
inline std::shared_ptr<const ModelIdentity> activeModel() {
 auto current=std::atomic_load(&modelIdentity);
 auto tick=GetTickCount64(),sample=modelSeenAt.load();
 if(!current||current->name.empty()||!visual::firstperson::validatedLocal.load()||
    current->epoch!=visual::firstperson::currentEpoch.load()||
    tick<sample||tick-sample>2000)return nullptr;
 return current;
}
inline void observeModel(const visual::StudioPose& studio,std::uint32_t epoch){
 auto name=visual::canonicalModel(studio.name);
 auto current=std::atomic_load(&modelIdentity);
 if(!current||current->name!=name||current->checksum!=studio.checksum||current->epoch!=epoch){
  std::atomic_store(&modelIdentity,
   std::shared_ptr<const ModelIdentity>(std::make_shared<ModelIdentity>(ModelIdentity{name,studio.checksum,epoch})));
 }
 modelSeenAt=GetTickCount64();
}
struct Context {
 std::shared_ptr<const Settings> options;
 std::shared_ptr<const dango::GameCameraFrame> camera;
 std::uint32_t handle=0xffffffffu;
};
inline bool select(std::uint32_t renderable,Context& ctx) noexcept {
 if(!native::active.load(std::memory_order_acquire)||
    !validatedLocal.load(std::memory_order_acquire))return false;
 auto base=localBase.load(std::memory_order_acquire);
 if(!base||renderable!=base+4)return false;
 ctx.options=std::atomic_load(&options);
 if(!ctx.options->enabled)return false;
 // Diagnostic mode measures all verified local draws, including native third person.
 ctx.camera=std::atomic_load(&source_camera::snapshot);
 if(!ctx.camera->active||ctx.camera->epoch!=currentEpoch.load(std::memory_order_acquire))return false;
 auto tick=GetTickCount64(),sample=localTick.load(std::memory_order_acquire);
 if(tick<sample||tick-sample>500)return false;
 ctx.handle=currentHandle.load(std::memory_order_acquire);
 auto index=ctx.handle&4095u;
 if(ctx.handle==0xffffffffu||index>=source_entities::profile::MaxNetworked||!source_entities::list)return false;
 auto list=std::uint32_t(std::uintptr_t(source_entities::list));
 auto entries=std::uint32_t(std::int64_t(list)+source_entities::profile::EntryBaseDelta);
 source_entities::wire::Entry entry{};
 if(!source_entities::wire::load(source_entities::reader,entries+index*sizeof(entry),entry)||
    entry.entity!=base||entry.serial!=(ctx.handle>>12))return false;
 return true;
}
template<class Original> inline void draw(const void* info,visual::Matrix3x4* bones,
 const dango::Vec3& origin,Original&& original) {
 std::uint32_t renderable=0,header=0;
 auto ptr=std::uint32_t(std::uintptr_t(info));
 if(!info||!source_entities::wire::load(source_entities::reader,ptr,header)||
    !source_entities::wire::load(source_entities::reader,ptr+24,renderable)){
  original(bones,origin);return;
 }
 Context ctx;
 if(!select(renderable,ctx)) {original(bones,origin);return;}
 // View-compatibility camera mode is committed once by the game-frame callback.
 // Do not reclassify/rewrite its state from individual Studio draw calls:
 // doing so flips the native gate during movement and resets alignment.
 ++matched;
 dango::Vec3 shift{};bool moved=false,partChanged=false,dynamicMoved=false;
 visual::Matrix3x4 temporary[256];
 auto settings=ctx.options;
 const bool wantOffset=settings->forward!=0||settings->right!=0||settings->up!=0;
 const bool wantParts=settings->hideHead||settings->hideArms;
 const bool wantDynamic=settings->dynamicAnchor||settings->dynamicAnchor2;
 const visual::StudioPose* studio=nullptr;
 bool ready=false;
 if(bones){
  static thread_local visual::StudioCache cache;
  const char* reason=nullptr;
  studio=cache.find(source_entities::reader,header,ctx.camera->epoch,&reason);
  if(studio&&studio->count>0&&studio->count<=256&&
     source_entities::reader.read(std::uint32_t(std::uintptr_t(bones)),temporary,studio->count*sizeof(visual::Matrix3x4))){
   ready=true;
   for(unsigned i=0;i<studio->count;++i)
    if(!parts::matrixValid(temporary[i])){ready=false;break;}
   if(ready)observeModel(*studio,ctx.camera->epoch);
  }
 }
 // Observe original-to-FPB matrices before modifying any bones, in all views.
 static thread_local meanbody::Calibration mean;
 static thread_local distanceprobe::Recorder recorder;
 static thread_local unsigned meanGeneration=0;
 const auto requestedGeneration=calibrationGeneration.load();
 if(meanGeneration!=requestedGeneration){mean.reset();meanGeneration=requestedGeneration;}
 dango::GameCameraFrame measuredFrame{};dango::CameraPose measuredPose{};
 dynamic::Rig measuredRig{};distanceprobe::Sample measured{};
 const bool measuredCamera=ready&&source_camera::sampleDuringDraw(measuredFrame)&&
     measuredFrame.epoch==ctx.camera->epoch&&dango::cameraPose(measuredFrame,measuredPose);
 if(measuredCamera){
  measuredRig=dynamic::discover(*studio,settings->fallbackEyeHeight);
  measured=distanceprobe::measure(*studio,measuredRig,temporary,measuredPose,localBase.load());
  mean.identify(ctx.camera->epoch,ctx.handle,visual::canonicalModel(studio->name),studio->checksum,
      settings->fallbackEyeHeight,settings->dynamicAnchor2);
  const bool training=settings->dynamicAnchor2&&measured.first&&measured.eyeKnown&&
      (!settings->thirdPersonCompatibility||viewcompat::mode.load()==viewcompat::Mode::First);
  bool learned=mean.observe(measured.stance,training,measured.localCorrection,measuredFrame.tick);
  recorder.record(measured,*studio,mean,measuredFrame.tick,ctx.camera->epoch,ctx.handle,settings->dynamicAnchor2,learned);
  if(learned&&mean.frozen(measured.stance)){
   auto v=mean.frozen(measured.stance)->value;
   char log[250]{};std::snprintf(log,sizeof(log),"FPB_MEAN frozen stance=%s samples=%u local=%.3f/%.3f/%.3f",
    meanbody::label(measured.stance),meanbody::Mean::Required,double(v.x),double(v.y),double(v.z));
   source_camera::log(log);
  }
 }else mean.observe(meanbody::Stance::Unknown,false,{},GetTickCount64());
 // Keep the native skeleton intact when compatibility says not First.
 // Mean mode additionally requires conservative live first-person evidence,
 // even when the old optional compatibility switch is off.
 if((settings->thirdPersonCompatibility&&viewcompat::mode.load()!=viewcompat::Mode::First)||
    (settings->dynamicAnchor2&&(!measuredCamera||!measured.first||!measured.basis))){
  original(bones,origin);return;
 }
 if(settings->dynamicAnchor2){
  // Frozen corrections are in original body axes, not camera/world axes.
  // Never recompute from camera displacement after calibration.
  const auto* frozen=mean.frozen(measured.stance);
  const auto* prof=visual::firstperson::profile(*settings,visual::canonicalModel(studio->name),studio->checksum);
  auto v=frozen?frozen->value:dango::Vec3{};
  v=v+dango::Vec3{settings->forward+(prof?prof->forward:0.f),settings->right+(prof?prof->right:0.f),settings->up+(prof?prof->up:0.f)};
  auto translation=dynamic::bodySpace(measured.front,measured.right,v.x,v.y,v.z);
  const float yaw=std::clamp((settings->yawTrim+(prof?prof->yaw:0.f))*0.01745329251994329577f,-.90f,.90f);
  if(dynamic::valid(translation)&&dango::length(translation)<=96.f&&
     dynamic::transform(temporary,studio->count,measured.anchor,translation,yaw)){
   shift=translation;dynamicMoved=moved=true;
   ++dynamicApplied;dynamicStatus=frozen?u8"动态定位2：使用姿态固定均值":u8"动态定位2：采样中，仅手动偏移";
   dynamicSource=meanbody::label(measured.stance);
  }else {++dynamicRejected;dynamicStatus=u8"动态定位2：修正量无效";}
 }else if(wantDynamic){
  static thread_local dynamic::Track tracking;
  static thread_local unsigned lastModeGeneration=0;
  const unsigned now=viewcompat::generation.load(std::memory_order_acquire);
  if(now!=lastModeGeneration){tracking=dynamic::Track{};lastModeGeneration=now;}
  const auto& live=measuredFrame;
  const auto& pose=measuredPose;
  if(measuredCamera){
   ++sameFrameCamera;
   const auto& rig=measuredRig;
   auto id=visual::canonicalModel(studio->name);
   auto prof=visual::firstperson::profile(*settings,id,studio->checksum);
   const auto resolved=dynamic::compute(*studio,rig,temporary,pose,*settings,prof,
                                       ctx.camera->epoch,ctx.handle,live.tick,tracking);
   if(resolved.ok&&rig.anchor>=0&&unsigned(rig.anchor)<studio->count){
    auto pivot=dynamic::location(temporary[unsigned(rig.anchor)]);
    if(dynamic::transform(temporary,studio->count,pivot,
                          resolved.translation,resolved.yawRadians)){
     shift=resolved.translation;dynamicMoved=moved=true;
     ++dynamicApplied;dynamicStatus=u8"动态身体已对齐本帧相机与胸部锚点";
     dynamicSource=resolved.usedBootstrap?"live-pose-bootstrap":resolved.source;
    }
   }
  }else{
   ++liveCameraRejected;
   dynamicStatus=u8"同帧相机矩阵不可用：动态定位跳过";
  }
  if(!dynamicMoved){
   ++dynamicRejected;
   if(ready)dynamicStatus=u8"当前MOD锚点/姿态无法安全校准，保留原版位置";
  }
 }else dynamicStatus=u8"动态定位关闭";
 if(!settings->dynamicAnchor2&&!dynamicMoved&&wantOffset){
  dango::Vec3 manual{};
  if(ready&&math::offset(*ctx.camera,settings->forward,settings->right,settings->up,manual)){
   bool finite=true;
   for(unsigned i=0;i<studio->count&&finite;++i)
    for(unsigned row=0;row<3;++row)
     finite=std::isfinite(temporary[i].m[row][3]+
         (row==0?manual.x:row==1?manual.y:manual.z));
   if(finite){
    for(unsigned i=0;i<studio->count;++i){
     temporary[i].m[0][3]+=manual.x;
     temporary[i].m[1][3]+=manual.y;
     temporary[i].m[2][3]+=manual.z;
    }
    shift=manual;moved=true;
   }
  }
  if(moved){++shifted;offsetStatus=u8"手动位置偏移已应用于骨骼副本";}
  else{++shiftRejected;offsetStatus=u8"骨骼或相机校验失败，位置偏移跳过";}
 }else if(dynamicMoved)offsetStatus=u8"手动位置微调已并入动态锚点";
 else offsetStatus=u8"位置偏移为零";
 if(wantParts){
  if(ready){
   // Use actual model bone hierarchy; never guess head from vertex height.
   // Only this draw's temporary bone matrices are modified.
   const auto plan=parts::discover(*studio);
   const auto result=parts::apply(*studio,plan,temporary,settings->hideHead,settings->hideArms);
   partChanged=result.modified!=0;
   if(settings->hideHead){
    if(result.head){++headHidden;headStatus=u8"已收缩头部及后代骨骼至颈部";}
    else{++headRejected;headStatus=u8"此 MOD 缺少可验证的头部/颈部骨骼分支";}
   }else headStatus=u8"头部分支隐藏关闭";
   if(settings->hideArms){
    if(result.left||result.right){++armsHidden;armStatus=(result.left&&result.right)?u8"已收缩双臂分支":u8"仅找到单侧手臂分支";}
    else{++armsRejected;armStatus=u8"此 MOD 未找到可验证的左右上臂骨骼";}
   }else armStatus=u8"双臂分支隐藏关闭";
  }else{
   if(settings->hideHead){++headRejected;headStatus=u8"Studio 骨骼校验失败，未隐藏头部";}
   if(settings->hideArms){++armsRejected;armStatus=u8"Studio 骨骼校验失败，未隐藏手臂";}
  }
 }else{headStatus=u8"头部分支隐藏关闭";armStatus=u8"双臂分支隐藏关闭";}
 dango::Vec3 shiftedOrigin=origin+shift;
 original((moved||partChanged)?temporary:bones,shiftedOrigin);
}
}
