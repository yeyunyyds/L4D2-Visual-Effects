#pragma once
#include <d3d9.h>
#include <cwchar>
namespace visual::overlaywindow {
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
}
