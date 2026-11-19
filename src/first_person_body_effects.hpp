#pragma once
// Render-only first-person body effects. Reuses the existing Studio hook.
#include "first_person_body.hpp"
#include "first_person_body_math.hpp"
#include "studio_cache.hpp"
#include "first_person_body_parts.hpp"
#include "first_person_dynamic_alignment.hpp"
#include <d3d9.h>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <cmath>
#include <cstring>
namespace visual::firstperson::effects {
inline std::mutex deviceMutex;
inline IDirect3DDevice9* renderDevice=nullptr;
inline std::atomic<unsigned long> matched{0},shifted{0},shiftRejected{0},clipSet{0},clipRejected{0};
inline std::atomic<const char*> clipStatus{u8"高度裁剪未启用"};
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
inline void setDevice(IDirect3DDevice9* next) noexcept {
 std::lock_guard<std::mutex> lock(deviceMutex);
 if(next==renderDevice)return;
 if(next)next->AddRef();
 auto old=renderDevice;renderDevice=next;
 if(old)old->Release();
}
inline IDirect3DDevice9* acquire() noexcept {
 std::lock_guard<std::mutex> lock(deviceMutex);
 auto p=renderDevice;
 if(p)p->AddRef();
 return p;
}
inline void stop() noexcept {setDevice(nullptr);}
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
 if(ctx.options->tltpvCompatibility&&
    tltpv::mode.load(std::memory_order_acquire)!=tltpv::Mode::First)return false;
 if(!ctx.options->clipEnabled&&!ctx.options->hideHead&&!ctx.options->hideArms&&
    ctx.options->forward==0&&ctx.options->right==0&&ctx.options->up==0&&!ctx.options->dynamicAnchor)return false;
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
class ClipScope {
 IDirect3DDevice9* device=nullptr;
 DWORD previousState=0,index=0;
 std::array<float,4> previousPlane{};
 bool applied=false;
public:
 ClipScope(const dango::GameCameraFrame& camera,float cutZ) noexcept {
  device=acquire();
  if(!device){clipStatus=u8"没有可用的 D3D9 设备";++clipRejected;return;}
  D3DCAPS9 caps{};
  if(FAILED(device->GetDeviceCaps(&caps))||caps.MaxUserClipPlanes==0||
     FAILED(device->GetRenderState(D3DRS_CLIPPLANEENABLE,&previousState))) {
    clipStatus=u8"设备不支持裁剪平面或无法读取渲染状态";++clipRejected;return;
  }
  auto maxPlane=std::min<UINT>(caps.MaxUserClipPlanes,6);
  bool freePlane=false;
  for(UINT i=0;i<maxPlane;++i)if(!(previousState&(1u<<i))){index=i;freePlane=true;break;}
  if(!freePlane||FAILED(device->GetClipPlane(index,previousPlane.data()))) {
    clipStatus=u8"没有可安全复用的裁剪平面槽位";++clipRejected;return;
  }
  IDirect3DVertexShader9* shader=nullptr;
  if(FAILED(device->GetVertexShader(&shader))) {
    clipStatus=u8"无法检测顶点着色器状态";++clipRejected;return;
  }
  bool programmable=shader!=nullptr;
  if(shader)shader->Release();
  std::array<float,4> plane{};
  if(!math::clipPlane(camera.worldToClip,cutZ,programmable,plane)) {
    clipStatus=u8"世界高度到 GPU 裁剪空间的转换失败";++clipRejected;return;
  }
  if(FAILED(device->SetClipPlane(index,plane.data()))) {
    clipStatus=u8"GPU 拒绝 SetClipPlane";++clipRejected;return;
  }
  if(FAILED(device->SetRenderState(D3DRS_CLIPPLANEENABLE,previousState|(1u<<index)))) {
    device->SetClipPlane(index,previousPlane.data());
    clipStatus=u8"GPU 拒绝启用 ClipPlane";++clipRejected;return;
  }
  applied=true;++clipSet;
  clipStatus=programmable?u8"裁剪平面已设置（VS/clip-space，画面待实测）":u8"裁剪平面已设置（fixed/world-space，画面待实测）";
 }
 ~ClipScope() noexcept {
  if(applied){device->SetRenderState(D3DRS_CLIPPLANEENABLE,previousState);device->SetClipPlane(index,previousPlane.data());}
  if(device)device->Release();
 }
 ClipScope(const ClipScope&)=delete;
 ClipScope& operator=(const ClipScope&)=delete;
};
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
 if(ctx.options->tltpvCompatibility){
  // TLTPV can switch cameras after the previous game-frame snapshot.
  // Validate the current Studio-draw camera before changing any matrix.
  const auto frame=std::atomic_load(&source_entities::snapshot);
  std::optional<dango::Vec3> playerOrigin;
  if(frame&&frame->epoch==ctx.camera->epoch)
   for(const auto& actor:frame->records)if(actor.handle==ctx.handle){
    playerOrigin=actor.networkOrigin;break;
   }
  dango::GameCameraFrame live{};
  auto viewed=tltpv::Mode::Unknown;
  if(source_camera::sampleDuringDraw(live)&&live.epoch==ctx.camera->epoch)
   viewed=tltpv::detect(live,playerOrigin);
  tltpv::observe(viewed);
  if(viewed!=tltpv::Mode::First){
   tltpv::status=viewed==tltpv::Mode::Third?
     "TLTPV: draw-time third-person camera, body bypassed":
     "TLTPV: draw-time camera uncertain, body bypassed";
   original(bones,origin);return;
  }
 }
 ++matched;
 dango::Vec3 shift{};bool moved=false,partChanged=false,dynamicMoved=false;
 visual::Matrix3x4 temporary[256];
 auto settings=ctx.options;
 const bool wantOffset=settings->forward!=0||settings->right!=0||settings->up!=0;
 const bool wantParts=settings->hideHead||settings->hideArms;
 const bool wantDynamic=settings->dynamicAnchor;
 const visual::StudioPose* studio=nullptr;
 bool ready=false;
 if(bones&&(wantOffset||wantParts||wantDynamic)){
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
 if(wantDynamic){
  static thread_local dynamic::Track tracking;
  static thread_local unsigned lastModeGeneration=0;
  const unsigned now=tltpv::generation.load(std::memory_order_acquire);
  if(now!=lastModeGeneration){tracking=dynamic::Track{};lastModeGeneration=now;}
  dango::GameCameraFrame live{};
  dango::CameraPose pose{};
  if(ready&&source_camera::sampleDuringDraw(live)&&
     live.epoch==ctx.camera->epoch&&dango::cameraPose(live,pose)){
   ++sameFrameCamera;
   auto rig=dynamic::discover(*studio,settings->fallbackEyeHeight);
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
 if(!dynamicMoved&&wantOffset){
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
 if(settings->clipEnabled){
   ClipScope clip(*ctx.camera,shiftedOrigin.z+settings->clipHeight);
   original((moved||partChanged)?temporary:bones,shiftedOrigin);
 }else{
   clipStatus=u8"高度裁剪未启用";
   original((moved||partChanged)?temporary:bones,shiftedOrigin);
 }
}
}
