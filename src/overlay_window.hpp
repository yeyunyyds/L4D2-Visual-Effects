#pragma once
#include <d3d9.h>
#include <cwchar>
namespace visual::overlaywindow {
inline bool localWindow(HWND window){DWORD process=0;return window&&IsWindow(window)&&GetWindowThreadProcessId(window,&process)&&process==GetCurrentProcessId();}
// Only the device-window identities are cached, never the live focus result.
// Accessed under hudruntime::paintMutex; invalidate on device lifecycle events.
struct WindowCache {IDirect3DDevice9* device=nullptr;HWND focus=nullptr,present=nullptr,lastForeground=nullptr;ULONGLONG refreshAt=0;};
inline WindowCache windows;
inline void invalidate(){windows={};}
inline void refresh(IDirect3DDevice9* device,HWND foreground,ULONGLONG at){
 windows={};windows.device=device;windows.refreshAt=at+1000;windows.lastForeground=foreground;
 D3DDEVICE_CREATION_PARAMETERS creation{};
 if(SUCCEEDED(device->GetCreationParameters(&creation))&&localWindow(creation.hFocusWindow))windows.focus=GetAncestor(creation.hFocusWindow,GA_ROOT);
 IDirect3DSwapChain9* chain=nullptr;D3DPRESENT_PARAMETERS parameters{};
 if(SUCCEEDED(device->GetSwapChain(0,&chain))&&chain){if(SUCCEEDED(chain->GetPresentParameters(&parameters))&&localWindow(parameters.hDeviceWindow))windows.present=GetAncestor(parameters.hDeviceWindow,GA_ROOT);chain->Release();}
}
inline bool foregroundGame(IDirect3DDevice9* device){
 auto foreground=GetForegroundWindow();if(!localWindow(foreground))return false;
 auto root=GetAncestor(foreground,GA_ROOT);auto at=GetTickCount64();
 if(windows.device!=device||at>=windows.refreshAt||root!=windows.lastForeground)refresh(device,root,at);
 if(root&&(root==windows.focus||root==windows.present))return true;
 // Preserve the wrapper fallback, but never accept a native dialog.
 if(windows.focus||windows.present)return false;
 wchar_t name[64]{};GetClassNameW(foreground,name,64);return std::wcscmp(name,L"#32770")!=0;
}
}
