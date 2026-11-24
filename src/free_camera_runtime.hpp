#include "plugin_lifetime.hpp"
#pragma once
// EXPERIMENTAL: exact-build, fail-closed L4D2 ClientMode vtable experiment.
// Indexes and field offsets below are SDK-derived hypotheses; actual 2026
// target DLL must be validated in-game before use outside offline tests.
#include "free_camera_core.hpp"
#include "free_camera_viewmodel_policy.hpp"
#include "free_camera_activation_policy.hpp"
#include "free_camera_command_policy.hpp"
#include "free_camera_hotkey.hpp"
#include "free_camera_external_policy.hpp"
#include "../include/l4d2visual_camera_api.h"
#include "module_verification.hpp"
#include "dango/source_camera.hpp"
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <mutex>
namespace visual::freecamera {
using core::Vec;
inline std::atomic<bool> requested{false},bound{false},active{false},paused{false};
inline std::atomic<bool> externalPermission{false},externalSession{false},apiLive{true};
inline std::atomic<external::Owner> currentOwner{external::Owner::Native};
inline std::atomic<bool> f6Binding{false};
// Key is configured by hud.json freeCamera.hotkey; existing enable flag retained.
inline std::atomic<int> hotkeyVk{hotkey::defaultVk};
inline std::atomic<bool> hotkeyConfigValid{true};
inline std::atomic<bool> viewModelHookBound{false};
inline std::atomic<const char*> viewModelHookStatus{"ViewModel predicate hook not installed"};
inline std::atomic<unsigned long> viewModelCalls{0},viewModelHidden{0},viewModelNativeTrue{0};
inline std::atomic<ULONGLONG> nextViewModelProbe{0};
inline std::atomic<const char*> message{"FreeCamera off"};
inline std::atomic<unsigned long> viewCount{0},cmdCount{0},viewRejected{0},cmdRejected{0};
inline std::atomic<unsigned long> menuToggles{0},keyToggles{0},keyPolls{0};
inline std::atomic<ULONGLONG> lastInput{0};
// Highest velocity; not an instantaneous constant step.
inline std::atomic<float> movementSpeed{1000.f};
// FreeCamera may hide the first-person weapon and arms, not the world body.
inline std::atomic<bool> hideViewmodel{true};
inline std::atomic<float> currentMovementSpeed{0.f};
inline core::Camera camera;
inline external::Policy ownerPolicy;
inline core::Vec externalOrigin{},externalAngles{};
inline bool externalTargetReady=false; // guarded by motionMutex
inline std::mutex motionMutex;
inline core::Input latestInput{};
inline std::atomic<bool> liveMap{false},bodyClear{false};
inline Vec nativeEntryEye{}; // guarded by motionMutex
inline bool nativeEntryKnown=false;
inline std::atomic<ULONGLONG> lastBadFrameLog{0},lastBadViewLog{0},lastBadCmdLog{0},lastMismatchLog{0};
struct MovementKeys {int forward='W',back='S',left='A',right='D',up=VK_SPACE,down=VK_LSHIFT,walk=VK_CAPITAL;float walkScale=.30f;};
inline MovementKeys movementKeys;
inline void setMovementKeys(MovementKeys value){std::lock_guard<std::mutex> lock(motionMutex);movementKeys=value;}
inline MovementKeys getMovementKeys(){std::lock_guard<std::mutex> lock(motionMutex);return movementKeys;}
inline core::MouseQueue pendingMouse;
inline std::atomic<std::int64_t> mouseReceivedX{0},mouseReceivedY{0};
inline std::atomic<std::int64_t> mouseAppliedX{0},mouseAppliedY{0};
inline std::atomic<std::int64_t> mouseDroppedX{0},mouseDroppedY{0};
inline std::atomic<std::uint64_t> mouseSamples{0},mouseZeroSamples{0};
inline std::atomic<std::uint64_t> serverCmdSamples{0},extraMouseSamples{0},extraMouseNonzero{0};
inline core::FrameClock renderClock;
inline std::atomic<unsigned long> renderSteps{0},renderRepeats{0},renderGaps{0};
inline std::atomic<float> renderDeltaMs{0.f};
inline Vec lockedAim{};inline bool haveAim=false;
inline std::uint32_t epoch=0,observedEpoch=0;
inline void** table=nullptr;
inline void* originalView=nullptr;inline void* originalMove=nullptr;
using ViewModelCallback=bool(__thiscall*)(void*);
inline ViewModelCallback originalViewModel=nullptr;
inline ULONGLONG nextProbe=0;
// Verified against SHA256 c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f.
// IClientMode::ShouldDrawViewModel follows LevelShutdown in the vtable.
constexpr unsigned viewSlot=19,moveSlot=27,viewModelSlot=30;
constexpr std::uintptr_t originalViewModelRva=0x2286b0;
struct Cmd {
 void* vptr;int command_number,tick_count;Vec viewangles;
 float forwardmove,sidemove,upmove;int buttons;unsigned char impulse,pad[3];
 int weaponselect,weaponsubtype,random_seed;short mousedx,mousedy;bool predicted;
};
static_assert(offsetof(Cmd,viewangles)==12,"CUserCmd angle position");
static_assert(offsetof(Cmd,buttons)==36,"CUserCmd buttons position");
static_assert(offsetof(Cmd,mousedx)==56,"CUserCmd mouse position");
using ViewCallback=void(__thiscall*)(void*,void*);
using MoveCallback=bool(__thiscall*)(void*,float,Cmd*);
inline bool read(const void* object,unsigned offset,void* out,unsigned size){
 auto ptr=reinterpret_cast<const char*>(object)+offset;
 if(!source_camera::readable(ptr,size))return false;
 std::memcpy(out,ptr,size);return true;
}
inline bool write(void* object,unsigned offset,const void* in,unsigned size){
 auto ptr=reinterpret_cast<char*>(object)+offset;
 if(!source_camera::readable(ptr,size))return false;
 std::memcpy(ptr,in,size);return true;
}
// L4D2 has CViewSetup fields four bytes earlier than SDK2013. Accept only
// one plausible layout, failing closed rather than writing unknown offsets.
struct ViewLayout {unsigned fov,fovVm,origin,angles,zNear,zFar;const char* name;};
constexpr ViewLayout l4d2View{52,56,60,72,84,88,"L4D2"};
constexpr ViewLayout sdk2013View{56,60,64,76,88,92,"SDK2013"};
inline std::atomic<const char*> detectedLayout{"not validated"};
inline bool checkView(void* setup,const ViewLayout& l,Vec& p,Vec& a) {
 int w=0,h=0;float f=0,vf=0,nearPlane=0,farPlane=0;
 return read(setup,16,&w,4)&&read(setup,24,&h,4)&&
  read(setup,l.fov,&f,4)&&read(setup,l.fovVm,&vf,4)&&
  read(setup,l.origin,&p,12)&&read(setup,l.angles,&a,12)&&
  read(setup,l.zNear,&nearPlane,4)&&read(setup,l.zFar,&farPlane,4)&&
  w>=64&&w<=32768&&h>=64&&h<=32768&&
  std::isfinite(f)&&f>=35&&f<=160&&std::isfinite(vf)&&vf>20&&vf<180&&
  core::valid(p)&&core::valid(a)&&std::abs(a.x)<=90&&std::abs(a.y)<=720&&
  std::isfinite(nearPlane)&&nearPlane>0&&nearPlane<100&&
  std::isfinite(farPlane)&&farPlane>nearPlane+100&&farPlane<2000000;
}
inline bool viewFields(void* setup,Vec& p,Vec& a,ViewLayout& layout){
 Vec l4P{},l4A{},sdkP{},sdkA{};
 const bool l4=checkView(setup,l4d2View,l4P,l4A);
 const bool sdk=checkView(setup,sdk2013View,sdkP,sdkA);
 if(l4==sdk){detectedLayout=l4?"AMBIGUOUS layout":"NO matching layout";return false;}
 layout=l4?l4d2View:sdk2013View;
 p=l4?l4P:sdkP;a=l4?l4A:sdkA;
 detectedLayout=layout.name;
 return true;
}
inline bool validCmd(const Cmd* c){
 return source_camera::readable(c,offsetof(Cmd,predicted)+1)&&
  commandpolicy::shouldSample(c->command_number)&&core::valid(c->viewangles)&&
  std::abs(c->viewangles.x)<1000&&std::abs(c->viewangles.y)<1000&&
  std::isfinite(c->forwardmove)&&std::isfinite(c->sidemove);
}
inline bool foreground(){
 HWND window=GetForegroundWindow();DWORD pid=0;
 if(window)GetWindowThreadProcessId(window,&pid);
 return pid==GetCurrentProcessId();
}
inline bool key(int code){return code>0&&(GetAsyncKeyState(code)&0x8000)!=0;}
// Called under motionMutex. Caps is a held modifier, not a toggle/LED state.
inline void sampleMovementLocked(){const auto& k=movementKeys;latestInput.forward=float(int(key(k.forward))-int(key(k.back)));latestInput.side=float(int(key(k.right))-int(key(k.left)));latestInput.up=float(int(key(k.up))-int(key(k.down)));latestInput.speedScale=key(k.walk)?k.walkScale:1.f;}
inline bool enabled(){return requested.load()&&active.load();}
inline activation::Phase currentPhase(){
 return activation::phase(requested.load(),bound.load(),active.load(),paused.load());
}
// Must hold motionMutex; original game hooks are never called under this lock.
inline void dropMouseLocked(){
 const auto d=pendingMouse.drop();
 mouseDroppedX.fetch_add(d.x,std::memory_order_relaxed);
 mouseDroppedY.fetch_add(d.y,std::memory_order_relaxed);
}
inline core::MouseCounts queuedMouse(){
 std::lock_guard<std::mutex> lock(motionMutex);
 return pendingMouse.peek();
}
inline void resetMotionLocked(){
 camera.ready=false;camera.halt();nativeEntryKnown=false;bodyClear=false;
 latestInput={};dropMouseLocked();
 renderClock.reset();haveAim=false;lastInput=0;
 currentMovementSpeed=0.f;renderDeltaMs=0.f;
}
// The camera owner is selected under motionMutex. Never call an original
// engine hook while holding this lock.
inline void expireLeaseLocked(ULONGLONG at){
 const auto last=ownerPolicy.lastPoseMs;
 if(ownerPolicy.expire(at))source_camera::log("FreeCamera lease expired: now="+std::to_string(at)+" lastPose="+std::to_string(last));
}
inline external::Owner selectOwnerLocked(ULONGLONG at){
 expireLeaseLocked(at);
 const auto next=ownerPolicy.select(at);
 if(next==external::Owner::External)ownerPolicy.externalWasDriving=true;
 const auto previous=currentOwner.load(std::memory_order_relaxed);
 if(next!=previous){
  externalTargetReady=false;
  if(next==external::Owner::Native){
   active=false;resetMotionLocked();
  }else if(previous==external::Owner::Native){
   active=false;resetMotionLocked();
  }else if(next==external::Owner::External){
   camera.halt();latestInput={};dropMouseLocked();
  }else if(previous==external::Owner::External){
   // External -> user: never reset the detached camera pose.
   camera.halt();
  }
  currentOwner.store(next,std::memory_order_release);
  source_camera::log("FreeCamera owner "+std::to_string(static_cast<unsigned>(previous))+" -> "+std::to_string(static_cast<unsigned>(next)));
 }
 requested.store(ownerPolicy.on,std::memory_order_release);
 externalSession.store(ownerPolicy.desired,std::memory_order_release);
 return next;
}
inline void allowExternal(bool enabled){
 std::lock_guard<std::mutex> lock(motionMutex);
 ownerPolicy.allow(enabled,GetTickCount64());
 externalPermission.store(enabled,std::memory_order_release);
 selectOwnerLocked(GetTickCount64());
 source_camera::log(enabled?"FreeCamera ABI permission ON":"FreeCamera ABI permission OFF");
}
inline void setExternalIdleSeconds(unsigned seconds){
 std::lock_guard<std::mutex> lock(motionMutex);
 ownerPolicy.setIdle(seconds);
 selectOwnerLocked(GetTickCount64());
}
inline unsigned externalIdleSeconds(){
 std::lock_guard<std::mutex> lock(motionMutex);
 return ownerPolicy.idleSeconds;
}
inline std::atomic<bool> externalMenuBlocked{true};
inline bool setExternalInitialIdle(unsigned seconds){
 visual::lifetime::Guard guard;if(!guard)return false;
 if(!apiLive.load(std::memory_order_acquire))return false;
 std::lock_guard<std::mutex> lock(motionMutex);
 if(!ownerPolicy.allowed)return false;
 ownerPolicy.initialIdleSeconds=std::clamp(seconds,1u,120u);
 return true;
}
inline bool setExternalNativeOnInput(bool enabled){
 visual::lifetime::Guard guard;if(!guard)return false;
 if(!apiLive.load(std::memory_order_acquire))return false;
 std::lock_guard<std::mutex> lock(motionMutex);
 if(!ownerPolicy.allowed)return false;
 ownerPolicy.nativeOnInput=enabled;return true;
}
inline bool requestExternal(bool enabled){
 visual::lifetime::Guard guard;if(!guard)return false;
 if(!apiLive.load(std::memory_order_acquire))return false;
 std::lock_guard<std::mutex> lock(motionMutex);
 const auto at=GetTickCount64();
 if(!ownerPolicy.request(enabled,at))return false;
 selectOwnerLocked(at);
 source_camera::log(enabled?"FreeCamera ABI session requested":"FreeCamera ABI session released");
 return true;
}
inline bool publishExternalPose(const L4D2VisualCameraPoseV1* p){
 visual::lifetime::Guard guard;if(!guard)return false;
 if(!apiLive.load(std::memory_order_acquire)||!p||p->size!=sizeof(*p))return false;
 core::Vec origin{p->origin[0],p->origin[1],p->origin[2]};
 core::Vec angles{p->angles[0],p->angles[1],p->angles[2]};
 if(!core::valid(origin)||!core::valid(angles)||
    std::abs(origin.x)>2000000.f||std::abs(origin.y)>2000000.f||std::abs(origin.z)>2000000.f||
    std::abs(angles.x)>89.f||std::abs(angles.y)>720.f||std::abs(angles.z)>180.f)return false;
 std::lock_guard<std::mutex> lock(motionMutex);
 if(!ownerPolicy.allowed||!ownerPolicy.desired)return false;
 const auto at=GetTickCount64();
 expireLeaseLocked(at);
 if(!ownerPolicy.desired){selectOwnerLocked(at);return false;}
 const auto previous=currentOwner.load(std::memory_order_relaxed);
 ownerPolicy.setPose(at);
 const auto next=selectOwnerLocked(at);
 // While the user is driving, accept a heartbeat but discard its coordinates.
 // At handoff hold the last actual camera pose until the next fresh sample.
 if(previous==external::Owner::External&&next==external::Owner::External){
  externalOrigin=origin;externalAngles=angles;externalTargetReady=true;
 }
 return true;
}
inline bool getExternalPose(L4D2VisualCameraPoseV1* output){
 visual::lifetime::Guard guard;if(!guard)return false;
 if(!apiLive.load(std::memory_order_acquire)||!output||output->size!=sizeof(*output))return false;
 std::lock_guard<std::mutex> lock(motionMutex);
 if(!ownerPolicy.allowed||!ownerPolicy.on||!active.load()||!camera.ready)return false;
 *output=L4D2VisualCameraPoseV1{sizeof(*output),{camera.origin.x,camera.origin.y,camera.origin.z},{camera.angles.x,camera.angles.y,camera.angles.z}};
 return true;
}
inline bool statusExternal(L4D2VisualCameraStatusV1* output){
 visual::lifetime::Guard guard;if(!guard||!apiLive.load())return false;
 if(!output||output->size!=sizeof(*output))return false;
 std::lock_guard<std::mutex> lock(motionMutex);
 const ULONGLONG at=GetTickCount64();
 selectOwnerLocked(at);
 *output=L4D2VisualCameraStatusV1{
  sizeof(*output),L4D2VISUAL_CAMERA_ABI_VERSION,
  ownerPolicy.allowed?1u:0u,ownerPolicy.desired?1u:0u,
  static_cast<std::uint32_t>(currentOwner.load(std::memory_order_acquire)),
  ownerPolicy.idleSeconds,ownerPolicy.remainingMs(at),
  bound.load()?1u:0u,active.load()?1u:0u
 };
 return true;
}
inline bool hasHumanInput(const Cmd* cmd=nullptr){
 // Only physical input counts as activity. AI/server model movement
 // and toggled command bits must not reset the AFK timer.
 if(cmd&&(cmd->mousedx||cmd->mousedy))return true;
 // Detect held keyboard/mouse buttons while the L4D2 window has focus.
 for(int vk=VK_LBUTTON;vk<=VK_OEM_CLEAR;++vk){
  if(vk==VK_PACKET||vk==VK_PROCESSKEY||(externalSession.load()&&hotkeyConfigValid.load()&&vk==hotkeyVk.load()))continue;
  if(GetAsyncKeyState(vk)&0x8000)return true;
 }
 return false;
}
inline double renderTimeSeconds(){
 static const double inv=[]{
  LARGE_INTEGER f{};
  return QueryPerformanceFrequency(&f)&&f.QuadPart>0?
   1.0/static_cast<double>(f.QuadPart):0.0;
 }();
 LARGE_INTEGER t{};
 return inv>0.0&&QueryPerformanceCounter(&t)?
  static_cast<double>(t.QuadPart)*inv:0.0;
}
inline void disable(const char* reason="FreeCamera off"){
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  ownerPolicy.stop();
  selectOwnerLocked(GetTickCount64());
  active=false;resetMotionLocked();
 }
 message=reason;source_camera::log(std::string("FreeCamera reset: ")+reason);
}
inline void toggle(){
 bool next=false;
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  ownerPolicy.toggle(GetTickCount64());
  next=ownerPolicy.on;
  selectOwnerLocked(GetTickCount64());
 }
 message=next?(bound?"FreeCamera waiting for valid view/input":"FreeCamera armed; awaiting ClientMode hook"):"FreeCamera off";
 source_camera::log(next?"FreeCamera: ON":"FreeCamera: OFF");
}
inline void __fastcall overrideView(void* self,void*,void* setup){
 auto fn=reinterpret_cast<ViewCallback>(originalView);
 if(fn)fn(self,setup);
 visual::lifetime::Guard guard;if(!guard)return;
 ++viewCount;
 external::Owner owner;
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  owner=selectOwnerLocked(GetTickCount64());
 }
 if(owner==external::Owner::Native){active=false;return;}
 Vec origin{},angles{};ViewLayout layout{};
 if(!viewFields(setup,origin,angles,layout)){
  ++viewRejected;
  if(externalSession.load()){const auto at=GetTickCount64();if(at-lastBadViewLog.load()>=1000){lastBadViewLog=at;source_camera::log("FreeCamera ignored invalid view layout; preserving external lease");}return;}
  disable("CViewSetup layout mismatch; disabled");return;
 }
 const auto now=GetTickCount64(),last=lastInput.load();
 if(!externalSession.load(std::memory_order_acquire)&&(!last||now<last||now-last>1000)){
  active=false;message="Waiting for CreateMove";
  std::lock_guard<std::mutex> lock(motionMutex);
  camera.halt();renderClock.reset();dropMouseLocked();
  currentMovementSpeed=0.f;
  return;
 }
 Vec outputOrigin{},outputAngles{};
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  owner=selectOwnerLocked(GetTickCount64());
  if(owner==external::Owner::Native)return;
  if(!camera.ready){
   camera.start(origin,angles);nativeEntryEye=origin;nativeEntryKnown=camera.ready;bodyClear=false;
   epoch=source_camera::epoch;
   renderClock.reset();
  }
  if(!camera.ready)return;
  if(owner==external::Owner::External&&externalTargetReady){
   // Provider-supplied absolute pose. Never consume or integrate mouse input
   // while DangoMusic owns the detached camera.
   camera.origin=externalOrigin;camera.angles=externalAngles;
   camera.halt();latestInput={};dropMouseLocked();
  }
  const auto renderId=source_camera::renderBegins.load(std::memory_order_relaxed);
  const core::FrameTick tick=renderClock.next(renderId,renderTimeSeconds());
  if(tick.kind==core::FrameKind::Repeat){
   ++renderRepeats;
  }else{
   ++renderSteps;
   renderDeltaMs=tick.dt*1000.f;
   if(owner==external::Owner::External){
    // Pose is submitted by the provider; no manual movement integration.
   }else if(paused.load()||externalMenuBlocked.load()||!foreground()){
    dropMouseLocked();
    camera.halt();
   }else{
    const auto delta=pendingMouse.take();
    camera.look(float(delta.x),float(delta.y));
    mouseAppliedX.fetch_add(delta.x,std::memory_order_relaxed);
    mouseAppliedY.fetch_add(delta.y,std::memory_order_relaxed);
    if(tick.kind==core::FrameKind::Gap){
     ++renderGaps;camera.halt();
    }else if(tick.kind==core::FrameKind::Step){
     // Manual keyboard input is available even without spectator CreateMove.
     sampleMovementLocked();camera.speed=movementSpeed.load(std::memory_order_relaxed);camera.advance(latestInput,tick.dt);
    }
   }
   currentMovementSpeed=camera.currentSpeed();
  }
  outputOrigin=camera.origin;
  outputAngles=camera.angles;
  if(nativeEntryKnown&&core::magnitude(outputOrigin-nativeEntryEye)>=48.f)bodyClear=true;
  if(!write(setup,layout.origin,&outputOrigin,12)||!write(setup,layout.angles,&outputAngles,12)){
   ++viewRejected;ownerPolicy.stop();selectOwnerLocked(GetTickCount64());message="CViewSetup write failed";source_camera::log("FreeCamera reset: CViewSetup write failed");return;
  }
  active=true;
  message=owner==external::Owner::External?"FreeCamera EXTERNAL":"FreeCamera USER";
 }
}
// Preserve the original client-mode decision and any upstream hook in the
// chain. In FreeCamera mode, override ONLY the viewmodel visibility result.
inline bool __fastcall shouldDrawViewModel(void* self,void*) {
 const auto original=originalViewModel;
 if(!original)return true; // Fail open if unexpectedly not initialized.
 const bool native=original(self);
 visual::lifetime::Guard guard;if(!guard)return native;
 ++viewModelCalls;
 if(native)++viewModelNativeTrue;
 const bool visible=viewmodel::policy::visible(enabled(),
                                               hideViewmodel.load(std::memory_order_acquire),
                                               native);
 if(native&&!visible)++viewModelHidden;
 return visible;
}
inline bool __fastcall createMove(void* self,void*,float dt,Cmd* cmd){
 auto fn=reinterpret_cast<MoveCallback>(originalMove);
 bool result=fn?fn(self,dt,cmd):false;
 visual::lifetime::Guard guard;if(!guard)return result;
 ++cmdCount;
 if(!requested&&!externalPermission)return result;
 if(!cmd||!source_camera::readable(cmd,offsetof(Cmd,predicted)+1))return result;
 // Source ExtraMouseSample uses command_number==0 and must be observed.
 const auto kind=commandpolicy::classify(cmd->command_number);
 if(kind==commandpolicy::Kind::Invalid)return result;
 if(kind==commandpolicy::Kind::ExtraMouseSample){
  ++extraMouseSamples;
  if(cmd->mousedx||cmd->mousedy)++extraMouseNonzero;
 }else ++serverCmdSamples;
 const bool focused=foreground();
 const bool input=focused&&externalPermission.load(std::memory_order_acquire)&&hasHumanInput(cmd);
 external::Owner owner;
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  const auto at=GetTickCount64();expireLeaseLocked(at);
  if(input||!focused||paused.load()||externalMenuBlocked.load())ownerPolicy.noteInput(at,input&&focused&&!paused.load()&&!externalMenuBlocked.load());
  else if(liveMap.load()&&bound.load())ownerPolicy.advance(at);
  owner=selectOwnerLocked(at);
 }
 if(owner==external::Owner::Native)return result;
 // Never suppress server commands before a valid detached view exists.
 if(!active.load(std::memory_order_acquire)){
  lastInput=GetTickCount64();
  return result;
 }
 if(!validCmd(cmd)){
  ++cmdRejected;
  if(externalSession.load()){const auto at=GetTickCount64();if(at-lastBadCmdLog.load()>=1000){lastBadCmdLog=at;source_camera::log("FreeCamera ignored invalid command; preserving external lease");}return result;}
  disable("CUserCmd layout mismatch; disabled");return result;
 }
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  owner=selectOwnerLocked(GetTickCount64());
  if(owner==external::Owner::Native)return result;
  if(!haveAim){lockedAim=cmd->viewangles;haveAim=true;}
  lastInput=GetTickCount64();
  mouseReceivedX.fetch_add(cmd->mousedx,std::memory_order_relaxed);
  mouseReceivedY.fetch_add(cmd->mousedy,std::memory_order_relaxed);
  ++mouseSamples;
  if(!cmd->mousedx&&!cmd->mousedy)++mouseZeroSamples;
  if(owner==external::Owner::User&&!paused.load()&&focused){
   sampleMovementLocked();
   pendingMouse.push(cmd->mousedx,cmd->mousedy);
  }else{
   latestInput={};
   dropMouseLocked();
   mouseDroppedX.fetch_add(cmd->mousedx,std::memory_order_relaxed);
   mouseDroppedY.fetch_add(cmd->mousedy,std::memory_order_relaxed);
  }
  // Camera control is local only; keep the networked survivor stationary.
  cmd->viewangles=lockedAim;
  cmd->forwardmove=cmd->sidemove=cmd->upmove=0;
  cmd->buttons=0;cmd->impulse=0;cmd->weaponselect=0;
  cmd->mousedx=cmd->mousedy=0;
 }
 return result;
}

inline bool changeSlot(void** vt,unsigned index,void* expected,void* replacement){
 if(!vt||!source_camera::readable(vt+index,4))return false;
 DWORD before=0;
 if(!VirtualProtect(vt+index,4,PAGE_READWRITE,&before))return false;
 auto actual=InterlockedCompareExchangePointer(vt+index,replacement,expected);
 DWORD ignored=0;VirtualProtect(vt+index,4,before,&ignored);
 return actual==expected;
}
// Verified exact 2026 client.dll SHA256 c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f
// Disassembly: client+0x22854e stores EAX DIRECTLY into client+0x7b1b8c[esi].
// ClientMode object = client+0x7c3240+index*0x148, vt = client+0x5bd4fc.
// OverrideView[19] at client+0x26ff40; CreateMove[27] at client+0xc2d70.
// The previous two-level read was incorrect.
inline std::atomic<const char*> locateDetail{"not attempted"};
inline std::atomic<int> locatedMode{-1};
constexpr std::uintptr_t globalRva=0x7b1b8c,registerInsnRva=0x22854e;
constexpr std::uintptr_t objectBaseRva=0x7c3240,objectStride=0x148;
constexpr std::uintptr_t vtableRva=0x5bd4fc;
constexpr std::uintptr_t originalViewRva=0x26ff40,originalMoveRva=0x0c2d70;
inline void* locate(HMODULE dll){
 locateDetail="checking exact-build client mode profile";
 locatedMode=-1;
 if(sizeof(void*)!=4){locateDetail="x86 client required";return nullptr;}
 const auto base=reinterpret_cast<std::uintptr_t>(dll);
 if(!base){locateDetail="client.dll not loaded";return nullptr;}
 unsigned char opcode[8]{};
 if(!read(reinterpret_cast<void*>(base+registerInsnRva),0,opcode,sizeof(opcode))){
  locateDetail="ClientMode registration opcode unreadable";return nullptr;
 }
 std::uint32_t global=0;std::memcpy(&global,opcode+3,4);
 if(opcode[0]!=0x89||opcode[1]!=0x04||opcode[2]!=0xb5||opcode[7]!=0xe8||
    global!=std::uint32_t(base+globalRva)){
  locateDetail="ClientMode registration opcode mismatch";return nullptr;
 }
 void* expectedVtable=reinterpret_cast<void*>(base+vtableRva);
 void* expectedView=nullptr;void* expectedMove=nullptr;
 if(!source_camera::readable(expectedVtable,(moveSlot+1)*4)||
    !read(expectedVtable,viewSlot*4,&expectedView,4)||
    !read(expectedVtable,moveSlot*4,&expectedMove,4)||
    expectedView!=reinterpret_cast<void*>(base+originalViewRva)||
    expectedMove!=reinterpret_cast<void*>(base+originalMoveRva)){
  locateDetail="ClientMode vtable slot fingerprint mismatch";return nullptr;
 }
 bool anyRegistered=false;
 for(int i=0;i<2;++i){
  void* instance=nullptr;
  if(!read(reinterpret_cast<void*>(base+globalRva+i*4),0,&instance,4)){
   locateDetail="ClientMode global array unreadable";return nullptr;
  }
  if(!instance)continue;
  anyRegistered=true;
  if(instance!=reinterpret_cast<void*>(base+objectBaseRva+i*objectStride))continue;
  void* vtable=nullptr;
  if(!read(instance,0,&vtable,4))continue;
  if(vtable!=expectedVtable||!source_camera::owns(vtable,dll))continue;
  if(!source_camera::executable(expectedView)||!source_camera::executable(expectedMove))continue;
  locatedMode=i;
  locateDetail=i==0?"Verified ClientMode slot 0":"Verified ClientMode slot 1";
  return instance;
 }
 locateDetail=anyRegistered?"ClientMode registered object identity mismatch":
                             "ClientMode slots not yet initialized";
 return nullptr;
}

// Bind the actual ClientMode::ShouldDrawViewModel[30], not an SDK guess.
// Disassembly of the exact supported client.dll establishes the RVA and
// no-argument BOOL return. Other ClientMode hooks still own their slots.
inline void tryBindViewModel(){
 if(viewModelHookBound.load(std::memory_order_acquire)||!bound||!table)return;
 const auto now=GetTickCount64();
 if(now<nextViewModelProbe.load(std::memory_order_relaxed))return;
 nextViewModelProbe=now+2000;
 const auto dll=source_camera::clientModule;
 if(!dll||GetModuleHandleW(L"client.dll")!=dll){
  viewModelHookStatus="client.dll changed; ViewModel hook declined";return;
 }
 const auto base=reinterpret_cast<std::uintptr_t>(dll);
 // Checked against the source DLL with SHA256
 // c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f.
 constexpr unsigned char signature[]={
  0x55,0x8b,0xec,0x83,0xec,0x08,0x53,0x56,
  0x8b,0xd9,0xe8,0x71,0x9a,0xff,0xff
 };
 const auto expected=reinterpret_cast<const void*>(base+originalViewModelRva);
 if(!source_camera::matchesImage(dll,source_camera::ClientTimestamp,
       source_camera::ClientImageSize,"FreeCamera ViewModel client")||
    !verifiedModuleFile(dll,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")||
    !source_camera::readable(expected,sizeof(signature))||
    std::memcmp(expected,signature,sizeof(signature))!=0||
    !source_camera::executable(expected)||
    !source_camera::readable(table,(viewModelSlot+1)*sizeof(void*))){
  viewModelHookStatus="ViewModel slot30 build/signature rejected; native drawing retained";
  return;
 }
 auto upstream=table[viewModelSlot];
 if(upstream==reinterpret_cast<void*>(&shouldDrawViewModel)||
    !source_camera::executable(upstream)){
  viewModelHookStatus="ViewModel slot30 hook conflict; native drawing retained";
  return;
 }
 // Chaining a pre-existing executable hook matches the other ClientMode
 // hooks; all original decisions are evaluated exactly once.
 originalViewModel=reinterpret_cast<ViewModelCallback>(upstream);
 if(!changeSlot(table,viewModelSlot,upstream,
                reinterpret_cast<void*>(&shouldDrawViewModel))){
  viewModelHookStatus="ViewModel slot30 compare-exchange conflict";
  return;
 }
 viewModelHookBound=true;
 viewModelHookStatus="ClientMode ShouldDrawViewModel[30] installed";
 source_camera::log("FreeCamera: verified ClientMode ShouldDrawViewModel[30] hook installed");
}

inline void tryBind(){
 if(bound){tryBindViewModel();return;}
 auto now=GetTickCount64();if(now<nextProbe)return;nextProbe=now+2000;
 auto dll=source_camera::clientModule;
 if(!dll||!source_camera::hooked){message="Waiting for Source camera initialization";return;}
 if(!source_camera::matchesImage(dll,source_camera::ClientTimestamp,
    source_camera::ClientImageSize,"FreeCamera client")||
    !verifiedModuleFile(dll,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")){
  message="FreeCamera game build mismatch";return;
 }
 auto instance=locate(dll);
 if(!instance){message=locateDetail.load();return;}
 auto vt=*reinterpret_cast<void***>(instance);
 auto v=vt[viewSlot],m=vt[moveSlot];
 if(v==reinterpret_cast<void*>(&overrideView)||m==reinterpret_cast<void*>(&createMove)){
  message="Recursive hook refused";return;
 }
 HMODULE pinned=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
    reinterpret_cast<LPCWSTR>(&overrideView),&pinned)){message="DLL pin failed";return;}
 originalView=v;originalMove=m;table=vt;
 if(!changeSlot(vt,viewSlot,v,reinterpret_cast<void*>(&overrideView))){
  message="OverrideView hook conflict";return;
 }
 if(!changeSlot(vt,moveSlot,m,reinterpret_cast<void*>(&createMove))){
  changeSlot(vt,viewSlot,reinterpret_cast<void*>(&overrideView),v);
  message="CreateMove hook conflict";return;
 }
 bound=true;message="FreeCamera hooks ready";
 source_camera::log(std::string("FreeCamera: verified 2026 ClientMode bound index=")+std::to_string(locatedMode.load())+" view[19] move[27]");
 tryBindViewModel();
}
inline void frame(const dango::GameCameraFrame& f){
 liveMap.store(f.active!=0,std::memory_order_release);
 bool externalPending=false;
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  externalPending=ownerPolicy.desired;
 }
 if(!f.active){
  const auto state=source_camera::captureState.load();
  const bool sameEpoch=observedEpoch!=0&&observedEpoch==f.epoch;
  // 3/4 = unreadable or temporarily mismatched matrices, not a level exit.
  if(sameEpoch&&(state==3||state==4)){
   if(externalPending&&f.tick-lastBadFrameLog.load()>=1000){lastBadFrameLog=f.tick;source_camera::log("FreeCamera transient invalid frame; preserving external lease state="+std::to_string(state));}
   return;
  }
  observedEpoch=0;
  if(requested||externalPending)disable("FreeCamera reset on map exit");
  return;
 }
 // Even a pending external request must NOT carry a pose into the next map.
 const bool mapChanged=observedEpoch!=0&&observedEpoch!=f.epoch;
 observedEpoch=f.epoch;
 if(mapChanged&&(requested||externalPending))disable("FreeCamera reset on map epoch");
 // External permission opts in to the SAME hooks before any external request;
 // until a valid pose and idle interval, CreateMove is native passthrough.
 if(requested.load(std::memory_order_acquire)||externalPermission.load(std::memory_order_acquire))tryBind();
 if(enabled()&&f.epoch!=epoch){disable("FreeCamera reset on map epoch");}
 if(externalPending&&active.load()&&currentOwner.load()==external::Owner::External){
  dango::CameraPose rendered;
  if(dango::cameraPose(f,rendered)){
   std::lock_guard<std::mutex> lock(motionMutex);
   auto delta=core::magnitude(Vec{rendered.eye.x,rendered.eye.y,rendered.eye.z}-camera.origin);
   if(camera.ready&&delta>32.f&&f.tick-lastMismatchLog.load()>=1000){lastMismatchLog=f.tick;source_camera::log("FreeCamera final matrix/override mismatch units="+std::to_string(delta));}
  }
 }
}
inline void poll(bool escape,bool menu,bool gameForeground){
 externalMenuBlocked.store(escape||menu||!gameForeground,std::memory_order_release);
 ++keyPolls;
 static bool held=false;
 static int previousVk=0;
 const int vk=hotkeyVk.load(std::memory_order_acquire);
 const bool now=vk>0&&key(vk);
 // A new config key may already be down. Arm only after its first release.
 const bool edge=(vk==previousVk)&&now&&!held;
 held=now;
 previousVk=vk;
 // Do not toggle via hotkey while the native menu is open: digit/navigation
 // key choices may otherwise trigger both a menu action and FreeCamera.
 // Do not move the detached camera while using menu controls.
 const bool shouldPause=escape||menu||!gameForeground;
 // Menus, lost focus and Escape all interrupt the external controller.
 // Never resume it while the menu is continuously open.
 if(externalSession.load(std::memory_order_acquire)){
  const bool input=escape||menu||!gameForeground||hasHumanInput();
  std::lock_guard<std::mutex> lock(motionMutex);
  const auto at=GetTickCount64();expireLeaseLocked(at);
  if(input)ownerPolicy.noteInput(at,!shouldPause&&!edge);
  else if(liveMap.load()&&bound.load())ownerPolicy.advance(at);
  selectOwnerLocked(at);
 }
 const bool previouslyPaused=paused.exchange(shouldPause);
 if(shouldPause&&!previouslyPaused){
  std::lock_guard<std::mutex> lock(motionMutex);
  camera.halt();latestInput={};dropMouseLocked();
  renderClock.reset();currentMovementSpeed=0.f;
 }
 if(edge&&(f6Binding.load(std::memory_order_acquire)||externalSession.load(std::memory_order_acquire))&&hotkeyConfigValid.load(std::memory_order_acquire)&&gameForeground&&!escape&&!menu){++keyToggles;toggle();}
}
inline void stop(){
 apiLive=false;
 disable("FreeCamera stopped");
 if(viewModelHookBound.exchange(false,std::memory_order_acq_rel)&&table){
  if(!changeSlot(table,viewModelSlot,
       reinterpret_cast<void*>(&shouldDrawViewModel),
       reinterpret_cast<void*>(originalViewModel)))
    source_camera::log("FreeCamera: downstream ViewModel hook owns slot; pinned thunk retained");
 }
 viewModelHookStatus="ViewModel predicate hook stopped";
 if(bound&&table){
  if(!changeSlot(table,moveSlot,reinterpret_cast<void*>(&createMove),originalMove))
   source_camera::log("FreeCamera: move hook downstream owns slot; pinned thunk retained");
  if(!changeSlot(table,viewSlot,reinterpret_cast<void*>(&overrideView),originalView))
   source_camera::log("FreeCamera: view hook downstream owns slot; pinned thunk retained");
 }
 bound=false;
}
}
