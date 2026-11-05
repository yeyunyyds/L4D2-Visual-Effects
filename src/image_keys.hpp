#pragma once
#include "image_runtime.hpp"
#include "spray_input.hpp"
#include "spray_display.hpp"
#include <cwchar>
namespace visual::picturekeys {
inline pictures::SprayInput input;
inline std::atomic<const char*> lastResult{u8"尚未提交T键喷漆"};
inline std::atomic<const char*> status{u8"T 键绑定关闭"};
inline bool localWindow(HWND window){DWORD process=0;return window&&IsWindow(window)&&GetWindowThreadProcessId(window,&process)&&process==GetCurrentProcessId();}
inline bool foregroundGame(IDirect3DDevice9* device){
 auto foreground=GetForegroundWindow();if(!localWindow(foreground))return false;
 D3DDEVICE_CREATION_PARAMETERS creation{};
 if(SUCCEEDED(device->GetCreationParameters(&creation))&&localWindow(creation.hFocusWindow)&&GetAncestor(foreground,GA_ROOT)==GetAncestor(creation.hFocusWindow,GA_ROOT))return true;
 IDirect3DSwapChain9* chain=nullptr;D3DPRESENT_PARAMETERS parameters{};bool matches=false;
 if(SUCCEEDED(device->GetSwapChain(0,&chain))&&chain){matches=SUCCEEDED(chain->GetPresentParameters(&parameters))&&localWindow(parameters.hDeviceWindow)&&GetAncestor(foreground,GA_ROOT)==GetAncestor(parameters.hDeviceWindow,GA_ROOT);chain->Release();}
 // Some wrappers expose no game HWND at all; the foreground window still must
 // belong to this game process. Reject native dialogs and text edit controls.
 wchar_t name[64]{};GetClassNameW(foreground,name,64);
 return matches||(!localWindow(creation.hFocusWindow)&&!localWindow(parameters.hDeviceWindow)&&std::wcscmp(name,L"#32770")!=0);
}
inline void poll(IDirect3DDevice9* device,bool menuOpen){
 auto settings=std::atomic_load(&pictures::options);bool enabled=settings->enabled&&settings->bindT;
 bool foreground=enabled&&foregroundGame(device);const int codes[]={'T','Y','U',VK_OEM_3,VK_RETURN,VK_ESCAPE};bool keys[pictures::SprayInput::Count]={};
 if(foreground)for(unsigned i=0;i<pictures::SprayInput::Count;++i)keys[i]=(GetAsyncKeyState(codes[i])&0x8000)!=0;
 auto now=GetTickCount64();bool fire=input.tick(keys,foreground,menuOpen,enabled,now);
 status=!enabled?u8"T 键绑定关闭":!foreground?u8"T 键：游戏未获得焦点":menuOpen?u8"T 键：请先关闭插件菜单":input.chat||input.console?u8"T 键：输入中，回车或Esc退出":u8"T 键：就绪";
 if(!fire)return;
 auto images=std::atomic_load(&pictures::assets);auto camera=std::atomic_load(&source_camera::snapshot);
 if(!dango::usableCamera(*camera,now)){lastResult=u8"上次T键：请先进入地图";return;}
 if(!images->decoded.count(settings->selected)){lastResult=u8"上次T键：请先选择图片并等待加载";return;}
 pictures::requestPlace();input.placed(now);lastResult=u8"上次T键：放置请求已提交";
}
inline void stop(){input.reset();}
}
