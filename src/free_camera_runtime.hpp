#pragma once
// EXPERIMENTAL: exact-build, fail-closed L4D2 ClientMode vtable experiment.
// Indexes and field offsets below are SDK-derived hypotheses; actual 2026
// target DLL must be validated in-game before use outside offline tests.
#include "free_camera_core.hpp"
#include "free_camera_viewmodel_policy.hpp"
#include "free_camera_activation_policy.hpp"
#include "free_camera_command_policy.hpp"
#include "free_camera_hotkey.hpp"
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
inline std::mutex motionMutex;
inline core::Input latestInput{};
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
inline std::uint32_t epoch=0;
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
inline bool key(int code){return (GetAsyncKeyState(code)&0x8000)!=0;}
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
 camera.ready=false;camera.halt();
 latestInput={};dropMouseLocked();
 renderClock.reset();haveAim=false;lastInput=0;
 currentMovementSpeed=0.f;renderDeltaMs=0.f;
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
 requested=false;active=false;
 {std::lock_guard<std::mutex> lock(motionMutex);resetMotionLocked();}
 message=reason;
}
inline void toggle(){
 bool next=false;
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  next=!requested.load();
  requested=next;active=false;resetMotionLocked();
 }
 message=next?(bound?"FreeCamera waiting for valid view/input":"FreeCamera armed; awaiting ClientMode hook"):"FreeCamera off";
 source_camera::log(next?"FreeCamera: request ON":"FreeCamera: request OFF");
}
inline void __fastcall overrideView(void* self,void*,void* setup){
 auto fn=reinterpret_cast<ViewCallback>(originalView);
 if(fn)fn(self,setup);
 ++viewCount;
 if(!requested){active=false;return;}
 Vec origin{},angles{};ViewLayout layout{};
 if(!viewFields(setup,origin,angles,layout)){
  ++viewRejected;disable("CViewSetup layout mismatch; disabled");return;
 }
 const auto now=GetTickCount64(),last=lastInput.load();
 if(!last||now<last||now-last>1000){
  active=false;message="Waiting for CreateMove";
  std::lock_guard<std::mutex> lock(motionMutex);
  camera.halt();renderClock.reset();dropMouseLocked();
  currentMovementSpeed=0.f;
  return;
 }
 Vec outputOrigin{},outputAngles{};
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  if(!requested)return;
  if(!camera.ready){
   camera.start(origin,angles);
   epoch=source_camera::epoch;
   renderClock.reset();
  }
  if(!camera.ready)return;
  const auto renderId=source_camera::renderBegins.load(std::memory_order_relaxed);
  const core::FrameTick tick=renderClock.next(renderId,renderTimeSeconds());
  if(tick.kind==core::FrameKind::Repeat){
   ++renderRepeats;
  }else{
   ++renderSteps;
   renderDeltaMs=tick.dt*1000.f;
   const core::Input i=latestInput;
   if(paused.load()||!foreground()){
    dropMouseLocked();
    camera.halt();
   }else{
    // Every real render (including Gap) consumes available mouse movement.
    // Gap only halts translation; it must never discard rotation.
    const auto delta=pendingMouse.take();
    camera.look(float(delta.x),float(delta.y));
    mouseAppliedX.fetch_add(delta.x,std::memory_order_relaxed);
    mouseAppliedY.fetch_add(delta.y,std::memory_order_relaxed);
    if(tick.kind==core::FrameKind::Gap){
     ++renderGaps;camera.halt();
    }else if(tick.kind==core::FrameKind::Step){
     camera.advance(i,tick.dt);
    }
   }
   currentMovementSpeed=camera.currentSpeed();
  }
  outputOrigin=camera.origin;
  outputAngles=camera.angles;
 }
 if(!write(setup,layout.origin,&outputOrigin,12)||
    !write(setup,layout.angles,&outputAngles,12)){
  ++viewRejected;disable("CViewSetup write failed");return;
 }
 active=true;message="FreeCamera ACTIVE";
}
// Preserve the original client-mode decision and any upstream hook in the
// chain. In FreeCamera mode, override ONLY the viewmodel visibility result.
inline bool __fastcall shouldDrawViewModel(void* self,void*) {
 const auto original=originalViewModel;
 if(!original)return true; // Fail open if unexpectedly not initialized.
 const bool native=original(self);
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
 ++cmdCount;
 if(!requested)return result;
 if(!cmd)return result;
 // Source's extra render-frame mouse sampler uses command_number == 0.
 // These are valid camera inputs, even though no server command is sent.
 const auto kind=commandpolicy::classify(cmd->command_number);
 if(kind==commandpolicy::Kind::Invalid)return result;
 if(kind==commandpolicy::Kind::ExtraMouseSample){
  extraMouseSamples.fetch_add(1,std::memory_order_relaxed);
  if(cmd->mousedx||cmd->mousedy)
   extraMouseNonzero.fetch_add(1,std::memory_order_relaxed);
 }else{
  serverCmdSamples.fetch_add(1,std::memory_order_relaxed);
 }
 // Preserve native commands until the camera is actually Active.
 // A fresh input timestamp allows OverrideView to complete startup.
 if(!active.load(std::memory_order_acquire)){
  lastInput=GetTickCount64();
  return result;
 }
 if(!validCmd(cmd)){
  ++cmdRejected;disable("CUserCmd layout mismatch; disabled");return result;
 }
 {
  std::lock_guard<std::mutex> lock(motionMutex);
  if(!requested)return result;
  if(!haveAim){lockedAim=cmd->viewangles;haveAim=true;}
  lastInput=GetTickCount64();
  mouseReceivedX.fetch_add(cmd->mousedx,std::memory_order_relaxed);
  mouseReceivedY.fetch_add(cmd->mousedy,std::memory_order_relaxed);
  mouseSamples.fetch_add(1,std::memory_order_relaxed);
  if(!cmd->mousedx&&!cmd->mousedy)
   mouseZeroSamples.fetch_add(1,std::memory_order_relaxed);
  if(!paused.load()&&foreground()){
   latestInput.forward=float(int(key('W'))-int(key('S')));
   latestInput.side=float(int(key('D'))-int(key('A')));
   latestInput.up=float(int(key(VK_SPACE))-int(key(VK_SHIFT)));
   pendingMouse.push(cmd->mousedx,cmd->mousedy);
  }else{
   latestInput={};
   dropMouseLocked();
   mouseDroppedX.fetch_add(cmd->mousedx,std::memory_order_relaxed);
   mouseDroppedY.fetch_add(cmd->mousedy,std::memory_order_relaxed);
  }
  // The engine also calls SetViewAngles after ExtraMouseSample.
  // Keep the world's player body locked while the free view rotates.
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
 if(!f.active){if(requested)disable("FreeCamera reset on map exit");return;}
 // Install ClientMode hooks only after a real enable request.
 if(requested.load(std::memory_order_acquire))tryBind();
 if(enabled()&&f.epoch!=epoch){disable("FreeCamera reset on map epoch");}
}
inline void poll(bool escape,bool menu,bool gameForeground){
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
 // Keep camera movement active with the menu open as before.
 const bool shouldPause=escape||!gameForeground;
 const bool previouslyPaused=paused.exchange(shouldPause);
 if(shouldPause&&!previouslyPaused){
  std::lock_guard<std::mutex> lock(motionMutex);
  camera.halt();latestInput={};dropMouseLocked();
  renderClock.reset();currentMovementSpeed=0.f;
 }
 if(edge&&f6Binding.load(std::memory_order_acquire)&&hotkeyConfigValid.load(std::memory_order_acquire)&&gameForeground&&!escape&&!menu){++keyToggles;toggle();}
}
inline void stop(){
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
