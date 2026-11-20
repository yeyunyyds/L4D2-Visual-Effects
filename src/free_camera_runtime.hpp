#pragma once
// EXPERIMENTAL: exact-build, fail-closed L4D2 ClientMode vtable experiment.
// Indexes and field offsets below are SDK-derived hypotheses; actual 2026
// target DLL must be validated in-game before use outside offline tests.
#include "free_camera_core.hpp"
#include "module_verification.hpp"
#include "dango/source_camera.hpp"
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
namespace visual::freecamera {
using core::Vec;
inline std::atomic<bool> requested{false},bound{false},active{false},paused{false};
inline std::atomic<const char*> message{"FreeCamera off"};
inline std::atomic<unsigned long> viewCount{0},cmdCount{0},viewRejected{0},cmdRejected{0};
inline std::atomic<unsigned long> menuToggles{0},keyToggles{0},keyPolls{0};
inline std::atomic<ULONGLONG> lastInput{0};
inline std::atomic<float> movementSpeed{260.f};
inline core::Camera camera;
inline Vec lockedAim{};inline bool haveAim=false;
inline std::uint32_t epoch=0;
inline void** table=nullptr;inline void* originalView=nullptr;inline void* originalMove=nullptr;
inline ULONGLONG nextProbe=0;
constexpr unsigned viewSlot=19,moveSlot=27;
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
  c->command_number>0&&core::valid(c->viewangles)&&
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
inline void disable(const char* reason="FreeCamera off"){
 requested=false;active=false;message=reason;
}
inline void toggle(){
 const bool next=!requested.load();
 requested=next;active=false;
 camera.ready=false;haveAim=false;lastInput=0;
 message=next?(bound?"FreeCamera waiting for valid view/input":"FreeCamera armed; awaiting ClientMode hook"):"FreeCamera off";
 source_camera::log(next?"FreeCamera: request ON":"FreeCamera: request OFF");
}
inline void __fastcall overrideView(void* self,void*,void* setup){
 auto fn=reinterpret_cast<ViewCallback>(originalView);
 if(fn)fn(self,setup);
 ++viewCount;
 if(!requested){active=false;camera.ready=false;return;}
 Vec origin{},angles{};ViewLayout layout{};
 if(!viewFields(setup,origin,angles,layout)){
  ++viewRejected;disable("CViewSetup layout mismatch; disabled");return;
 }
 const auto now=GetTickCount64(),last=lastInput.load();
 if(!last||now<last||now-last>1000){
  active=false;message="Waiting for CreateMove";return;
 }
 if(!camera.ready){camera.start(origin,angles);epoch=source_camera::epoch;}
 if(!camera.ready)return;
 if(!write(setup,layout.origin,&camera.origin,12)||
    !write(setup,layout.angles,&camera.angles,12)){
  ++viewRejected;disable("CViewSetup write failed");return;
 }
 active=true;message="FreeCamera ACTIVE";
}
inline bool __fastcall createMove(void* self,void*,float dt,Cmd* cmd){
 auto fn=reinterpret_cast<MoveCallback>(originalMove);
 bool result=fn?fn(self,dt,cmd):false;
 ++cmdCount;
 if(!requested){haveAim=false;return result;}
 if(!cmd||!cmd->command_number)return result; // Source may issue empty sample commands.
 if(!validCmd(cmd)){++cmdRejected;disable("CUserCmd layout mismatch; disabled");return result;}
 if(!haveAim){lockedAim=cmd->viewangles;haveAim=true;}
 lastInput=GetTickCount64();
 if(camera.ready&&!paused&&foreground()){
  core::Input i{};
  i.forward=float(int(key('W'))-int(key('S')));
  i.side=float(int(key('D'))-int(key('A')));
  i.up=float(int(key(VK_SPACE))-int(key(VK_CONTROL)));
  i.fast=key(VK_SHIFT);i.dx=float(cmd->mousedx);i.dy=float(cmd->mousedy);
  camera.speed=movementSpeed.load();
  camera.move(i,dt);
 }
 cmd->viewangles=lockedAim;
 cmd->forwardmove=cmd->sidemove=cmd->upmove=0;
 cmd->buttons=0;cmd->impulse=0;cmd->weaponselect=0;
 cmd->mousedx=cmd->mousedy=0;
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
inline void* locate(HMODULE dll){
 auto base=reinterpret_cast<std::uintptr_t>(dll);
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(dll);
 if(!source_camera::readable(dos,sizeof(*dos))||dos->e_magic!=IMAGE_DOS_SIGNATURE)return nullptr;
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
 if(!source_camera::readable(nt,sizeof(*nt))||nt->Signature!=IMAGE_NT_SIGNATURE)return nullptr;
 auto sections=IMAGE_FIRST_SECTION(nt);
 unsigned matches=0;std::uintptr_t address=0;
 for(unsigned s=0;s<nt->FileHeader.NumberOfSections;s++){
  if(!(sections[s].Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
  auto start=base+sections[s].VirtualAddress;
  auto size=std::min<std::size_t>(sections[s].Misc.VirtualSize,sections[s].SizeOfRawData);
  if(size<8||!source_camera::readable(reinterpret_cast<void*>(start),size))continue;
  auto b=reinterpret_cast<const std::uint8_t*>(start);
  for(std::size_t n=0;n+8<=size;n++){
   if(b[n]!=0x89||b[n+1]!=0x04||b[n+2]!=0xb5||b[n+7]!=0xe8)continue;
   std::uint32_t ptr=0;std::memcpy(&ptr,b+n+3,4);
   if(ptr>=base&&ptr+4<base+nt->OptionalHeader.SizeOfImage){if(!matches){matches=1;address=ptr;}else if(address!=ptr)matches=2;}
  }
 }
 if(matches!=1)return nullptr; // old L4D2 ClientMode array signature
 // g_pClientMode references a slot array, then the first mode instance.
 // Old prototype dereferenced only once and interpreted the array as an object.
 void* modeSlots=nullptr;
 if(!read(reinterpret_cast<void*>(address),0,&modeSlots,4)||
    !source_camera::readable(modeSlots,4))return nullptr;
 void* instance=nullptr;
 if(!read(modeSlots,0,&instance,4)||!source_camera::readable(instance,4))return nullptr;
 auto vt=*reinterpret_cast<void***>(instance);
 if(!vt||!source_camera::owns(vt,dll)||!source_camera::readable(vt,(moveSlot+1)*4)||
    !source_camera::executable(vt[viewSlot])||!source_camera::executable(vt[moveSlot]))return nullptr;
 return instance;
}
inline void tryBind(){
 if(bound)return;
 auto now=GetTickCount64();if(now<nextProbe)return;nextProbe=now+2000;
 auto dll=source_camera::clientModule;
 if(!dll||!source_camera::hooked){message="Waiting for Source camera initialization";return;}
 if(!source_camera::matchesImage(dll,source_camera::ClientTimestamp,
    source_camera::ClientImageSize,"FreeCamera client")||
    !verifiedModuleFile(dll,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")){
  message="FreeCamera game build mismatch";return;
 }
 auto instance=locate(dll);
 if(!instance){message="ClientMode signature not found/ambiguous";return;}
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
 bound=true;message="FreeCamera hooks ready; press F6";
 source_camera::log("FreeCamera: experimental ClientMode view[19] move[27] installed");
}
inline void frame(const dango::GameCameraFrame& f){
 if(!f.active){disable("FreeCamera reset on map exit");camera.ready=false;haveAim=false;lastInput=0;return;}
 tryBind();
 if(enabled()&&f.epoch!=epoch){disable("FreeCamera reset on map epoch");camera.ready=false;}
}
inline void poll(bool escape,bool menu,bool gameForeground){
 ++keyPolls;
 static bool held=false;bool now=key(VK_F6);bool edge=now&&!held;held=now;
 paused=escape||menu||!gameForeground;
 if(edge&&gameForeground&&!escape&&!menu){++keyToggles;toggle();}
}
inline void stop(){
 disable("FreeCamera stopped");camera.ready=false;
 if(bound&&table){
  if(!changeSlot(table,moveSlot,reinterpret_cast<void*>(&createMove),originalMove))
   source_camera::log("FreeCamera: move hook downstream owns slot; pinned thunk retained");
  if(!changeSlot(table,viewSlot,reinterpret_cast<void*>(&overrideView),originalView))
   source_camera::log("FreeCamera: view hook downstream owns slot; pinned thunk retained");
 }
 bound=false;
}
}
