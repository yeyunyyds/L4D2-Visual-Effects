#pragma once
// Render-only first-person body effects. Reuses the existing Studio hook.
#include "first_person_body.hpp"
#include "first_person_body_math.hpp"
#include "studio_cache.hpp"
#include "first_person_body_parts.hpp"
#include "first_person_dynamic_alignment.hpp"
#include "first_person_distance_probe.hpp"
#include "first_person_studio_pass_policy.hpp"
#include "log_settings.hpp"
#include <atomic>
#include <memory>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace visual::firstperson::effects {
inline std::atomic<unsigned long> matched{0},shifted{0},shiftRejected{0};
inline std::atomic<const char*> offsetStatus{u8"位置偏移为零"};
inline std::atomic<const char*> headStatus{u8"头部分支隐藏关闭"},armStatus{u8"双臂分支隐藏关闭"};
inline std::atomic<unsigned long> headHidden{0},headRejected{0},armsHidden{0},armsRejected{0};
inline std::atomic<unsigned long> dynamicApplied{0},dynamicRejected{0},sameFrameCamera{0},liveCameraRejected{0};
inline std::atomic<unsigned long> cameraGraceApplied{0},cameraGraceDenied{0},dynamicGraceApplied{0};
inline std::atomic<unsigned long> secondaryPassBypassed{0},shadowDepthOriginal{0};
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
 const dango::Vec3& origin,int studioFlags,Original&& original) {
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
 // Source Studio shadow-depth passes must not receive first-person-only bone
 // shrink/translation. Continue drawing the original model so actual shadow
 // depth is not removed; no assumptions about render-to-color passes are made.
 if(studiopass::preserveOriginal(studioFlags)){
  ++shadowDepthOriginal;
  if(visual::logging::periodicDebugEnabled()){
   static thread_local std::uint64_t lastShadowLog=0;
   const auto now=GetTickCount64();
   if(!lastShadowLog||now<lastShadowLog||now-lastShadowLog>=1000){
    char line[200]{};
    std::snprintf(line,sizeof(line),
     "FPB_SHADOW_NATIVE frame=%llu studioFlags=0x%x",
     static_cast<unsigned long long>(source_camera::renderBegins.load()),
     static_cast<unsigned>(studioFlags));
    source_camera::log(line);lastShadowLog=now;
   }
  }
  original(bones,origin);return;
 }
 dango::Vec3 shift{};bool moved=false,partChanged=false,dynamicMoved=false;
 bool headApplied=false,leftArmApplied=false,rightArmApplied=false;
 visual::Matrix3x4 temporary[256];
 auto settings=ctx.options;
 const bool wantOffset=settings->forward!=0||settings->right!=0||settings->up!=0;
 const bool wantParts=settings->hideHead||settings->hideArms;
 const bool wantDynamic=settings->dynamicAnchor;
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
 // Read original-to-FPB matrices and live entity origin before cosmetics.
 static thread_local viewpolicy::Gate distanceGate;
 static thread_local distanceprobe::Recorder recorder;
 static thread_local dynamic::Track tracking;
 static thread_local viewpolicy::CameraGapHold cameraGap;
 static thread_local viewpolicy::MainViewPassFilter mainView;
 static thread_local viewpolicy::AlignmentStability alignment;
 // Last trusted render-only transform, never reused outside the camera gap.
 static thread_local dango::Vec3 retainedTranslation{};
 static thread_local float retainedYaw=0.f;
 static thread_local std::uint64_t retainedAt=0;
 static thread_local int cameraLogState=0;
 static thread_local bool wasDebug=false;
 static thread_local std::uint64_t lastSecondaryTrace=0;
 static thread_local unsigned gateGeneration=0;
 static thread_local std::uint32_t gateEpoch=0,gateHandle=0xffffffffu,gateChecksum=0;
 const auto requestedGeneration=cosmeticGeneration.load();
 const bool fpbDebug=visual::logging::periodicDebugEnabled();
 // Info fast path: do not clear recorder strings on every Studio draw.
 if(fpbDebug!=wasDebug){recorder.reset();cameraLogState=0;wasDebug=fpbDebug;}
 if(gateGeneration!=requestedGeneration){
  distanceGate.reset();tracking.reset();cameraGap.reset();mainView.reset();alignment.reset();retainedAt=0;
  gateGeneration=requestedGeneration;
 }
 dango::GameCameraFrame measuredFrame{};dango::CameraPose measuredPose{};
 dynamic::Rig measuredRig{};distanceprobe::Sample measured{};
 const bool measuredCamera=ready&&source_camera::sampleDuringDraw(measuredFrame)&&
     measuredFrame.epoch==ctx.camera->epoch&&dango::cameraPose(measuredFrame,measuredPose);
 const ModelProfile* prof=nullptr;
 parts::Selection plan{};
 if(ready)plan=parts::discover(*studio);
 if(measuredCamera){
  measuredRig=dynamic::discover(*studio,settings->fallbackEyeHeight);
  measured=distanceprobe::measure(*studio,measuredRig,plan,temporary,measuredPose,localBase.load());
  prof=visual::firstperson::profile(*settings,visual::canonicalModel(studio->name),studio->checksum);
  if(gateEpoch!=ctx.camera->epoch||gateHandle!=ctx.handle||gateChecksum!=studio->checksum){
   gateEpoch=ctx.camera->epoch;gateHandle=ctx.handle;gateChecksum=studio->checksum;
   distanceGate.reset();tracking.reset();cameraGap.reset();mainView.reset();alignment.reset();retainedAt=0;
  }
 }
 const auto wallNow=GetTickCount64();
 // The engine's draw-time matrix may belong to another render pass. Do not
 // allow an off-axis pass to update the main-view hysteresis or pose tracking.
 // Only act after a centered first-person pass of the SAME player and model
 // has positively established a main-view eye in the last 100ms.
 const bool committedFirst=viewcompat::mode.load(std::memory_order_acquire)==viewcompat::Mode::First;
 const bool observedFirst=measuredCamera&&measured.view.evidence==viewcompat::Mode::First;
 const bool secondaryPass=measuredCamera&&mainView.isSecondary(
      committedFirst&&observedFirst,measured.shoulder,measured.originKnown,
      measured.view.horizontal,measured.view.eyeHeight,measuredPose.eye,
      ctx.camera->epoch,ctx.handle,studio->checksum,wallNow,measured.flags);
 if(secondaryPass){
  ++secondaryPassBypassed;
  if(fpbDebug&&(!lastSecondaryTrace||wallNow<lastSecondaryTrace||
                wallNow-lastSecondaryTrace>=1000)){
   char line[240]{};
   std::snprintf(line,sizeof(line),
      "FPB_SECONDARY_BYPASS frame=%llu eyeXY=%.2f eyeZ=%.2f checksum=%u",
      static_cast<unsigned long long>(source_camera::renderBegins.load()),
      double(measured.view.horizontal),double(measured.view.eyeHeight),
      unsigned(studio->checksum));
   source_camera::log(line);lastSecondaryTrace=wallNow;
  }
  // Keep the original studio pass intact; never suppress shadow/color draws
  // until their flags have been independently verified on this build.
  original(bones,origin);return;
 }
 if(measuredCamera){
  mainView.observe(committedFirst&&observedFirst,measured.shoulder,
    measured.originKnown,measured.view.horizontal,measured.view.eyeHeight,
    measuredPose.eye,ctx.camera->epoch,ctx.handle,studio->checksum,wallNow);
 }
 const auto tick=measuredCamera?measuredFrame.tick:wallNow;
 const bool compatibility=settings->thirdPersonCompatibility;
 viewpolicy::Decision decision{};
 bool retainedCamera=false;
 if(compatibility){
  if(measuredCamera){
   decision=distanceGate.update({
    measured.originKnown,measured.eyeKnown,measured.headKnown,
    measured.view.evidence==viewcompat::Mode::First&&viewcompat::mode.load()==viewcompat::Mode::First,
    measured.entityOrigin,measuredPose.eye,dango::length(measured.eyeDelta),measured.headDistance,measured.shoulder,tick});
   // Only a measured, positively identified first-person sample can arm the
   // 48ms grace period. A rejected/third-person observation clears it.
   cameraGap.observe(decision.full&&measured.view.evidence==viewcompat::Mode::First&&
       measured.shoulder==0&&viewcompat::mode.load()==viewcompat::Mode::First,
       ctx.camera->epoch,ctx.handle,studio->checksum,wallNow);
  }else if(ready&&cameraGap.accept(
       ready,viewcompat::mode.load()==viewcompat::Mode::First,probeShoulder.load(),
       ctx.camera->epoch,ctx.handle,studio->checksum,wallNow,ctx.camera->tick)){
   decision.full=true;decision.reason="camera-grace";retainedCamera=true;
   ++cameraGraceApplied;
  }else{
   distanceGate.reset();cameraGap.reset();retainedAt=0;
   if(ready)++cameraGraceDenied;
  }
 }else{
  distanceGate.reset();cameraGap.reset();
  decision.full=true;decision.reason="compat-off";
 }
 if(fpbDebug){
  // Log only transitions: an unretained invalid camera draw is diagnostically
  // different from a successful short camera gap.
  const int state=measuredCamera?0:retainedCamera?1:!ready?3:compatibility?2:4;
  if(state!=cameraLogState){
   const char* message=state==0?"FPB_CAMERA_GAP: same-draw camera recovered":
       state==1?"FPB_CAMERA_GAP: retaining verified first person (48ms max)":
       state==2?"FPB_CAMERA_GAP: camera unavailable, native cosmetics fallback":
       state==3?"FPB_CAMERA_GAP: Studio pose invalid, original bones kept":
                 "FPB_CAMERA_GAP: camera unavailable, compatibility disabled";
   source_camera::log(message);
   cameraLogState=state;
  }
 }
 const char* path="native";
 dango::Vec3 applied{};float appliedYaw=0;bool haveApplied=false;
 if(!decision.full){
  tracking.reset();alignment.reset();retainedAt=0;
  dynamicStatus=u8"视角组合判断：保留原生位置";
  if(ready&&settings->hideHead&&decision.earlyHead){
   auto result=parts::apply(*studio,plan,temporary,true,false);
   if(result.head){++headHidden;headApplied=true;headStatus=u8"相机接近原始头部：提前隐藏";path="head-only";partChanged=true;}
   else ++headRejected;
  }else headStatus=u8"原生头部保留";
  armStatus=u8"原生双臂保留";offsetStatus=u8"原生位置保留";
  if(fpbDebug&&measuredCamera)recorder.record(measured,*studio,decision,tick,ctx.camera->epoch,ctx.handle,compatibility,path,false,headApplied,false,false,applied,appliedYaw);
  original(partChanged?temporary:bones,origin);return;
 }
 // Alignment is chosen for the verified main-view sequence, never for an
 // isolated Studio callback. Secondary camera passes returned above.
 bool deferManual=false;
 if(wantDynamic){
  dynamic::Result resolved{};
  int candidateAnchor=-1;
  if(measuredCamera){
   ++sameFrameCamera;
   const auto& rig=measuredRig;
   resolved=dynamic::compute(*studio,rig,temporary,measuredPose,*settings,prof,
                             ctx.camera->epoch,ctx.handle,measuredFrame.tick,tracking);
   if(resolved.ok&&rig.anchor>=0&&unsigned(rig.anchor)<studio->count)
    candidateAnchor=rig.anchor;
  }else ++liveCameraRejected;
  const bool candidate=candidateAnchor>=0;
  const auto choice=alignment.decide(candidate,wallNow);
  if(choice==viewpolicy::AlignmentStability::Choice::Dynamic&&candidate){
   const auto pivot=dynamic::location(temporary[unsigned(candidateAnchor)]);
   if(dynamic::transform(temporary,studio->count,pivot,resolved.translation,resolved.yawRadians)){
    shift=resolved.translation;dynamicMoved=moved=true;path="dynamic1";
    ++dynamicApplied;dynamicStatus=u8"动态身体已对齐主视角与胸部锚点";
    dynamicSource=resolved.usedBootstrap?"live-pose-bootstrap":resolved.source;
    applied=shift;appliedYaw=resolved.yawRadians;haveApplied=true;
    retainedTranslation=shift;retainedYaw=resolved.yawRadians;retainedAt=wallNow;
   }else {alignment.reset();retainedAt=0;}
  }else if(choice==viewpolicy::AlignmentStability::Choice::Hold&&
           ready&&retainedAt&&wallNow>=retainedAt&&
           wallNow-retainedAt<=viewpolicy::AlignmentStability::HoldMs&&
           (measuredCamera||retainedCamera)){
   const auto rig=measuredCamera?measuredRig:
                  dynamic::discover(*studio,settings->fallbackEyeHeight);
   if(rig.anchor>=0&&unsigned(rig.anchor)<studio->count){
    const auto pivot=dynamic::location(temporary[unsigned(rig.anchor)]);
    if(dynamic::transform(temporary,studio->count,pivot,retainedTranslation,retainedYaw)){
     shift=retainedTranslation;dynamicMoved=moved=true;path="dynamic-grace";
     applied=shift;appliedYaw=retainedYaw;haveApplied=true;
     ++dynamicGraceApplied;dynamicStatus=u8"短暂失败：保持可信动态位置，不切换手动校准";
    }
   }
  }
  // A failed draw cannot instantly activate the differently placed manual
  // pose. After a sustained failure the manual path remains stable until
  // dynamic solutions have recovered continuously for 80ms.
  deferManual=choice!=viewpolicy::AlignmentStability::Choice::Manual;
  if(!dynamicMoved){
   ++dynamicRejected;
   if(ready)dynamicStatus=deferManual?
      u8"动态定位暂时失效：等待稳定或保持原生绘制":
      u8"动态定位持续不可用：稳定使用手动回退";
  }
 }else {tracking.reset();alignment.reset();retainedAt=0;dynamicStatus=u8"动态定位关闭";}
 if(!dynamicMoved&&wantOffset&&!deferManual){
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
   const auto result=parts::apply(*studio,plan,temporary,settings->hideHead,settings->hideArms);
   partChanged=result.modified!=0;
   headApplied=result.head;leftArmApplied=result.left;rightArmApplied=result.right;
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
 if(!dynamicMoved&&moved){path="manual";applied=shift;haveApplied=true;}
 else if(!moved&&partChanged)path="parts-only";
 if(fpbDebug&&measuredCamera)recorder.record(measured,*studio,decision,tick,ctx.camera->epoch,ctx.handle,compatibility,path,haveApplied,headApplied,leftArmApplied,rightArmApplied,applied,appliedYaw);
 dango::Vec3 shiftedOrigin=origin+shift;
 original((moved||partChanged)?temporary:bones,shiftedOrigin);
}
}
