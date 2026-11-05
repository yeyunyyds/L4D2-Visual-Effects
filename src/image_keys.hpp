#pragma once
#include "image_runtime.hpp"
namespace visual::picturekeys {
inline bool held[6]={};inline bool typing=false;inline ULONGLONG last=0;
// Poll edges in Present: unlike SetWindowSubclass this also works when the game's
// window and rendering callbacks belong to different threads. Never consume T.
inline void poll(IDirect3DDevice9* device,bool menuOpen){
 auto settings=std::atomic_load(&pictures::options);if(!settings->enabled||!settings->bindT){typing=false;for(auto& key:held)key=false;return;}
 D3DDEVICE_CREATION_PARAMETERS params{};if(FAILED(device->GetCreationParameters(&params))||!params.hFocusWindow)return;
 const int keys[]={'T','Y','U',VK_OEM_3,VK_RETURN,VK_ESCAPE};bool pressed[6]={};
 bool foreground=GetForegroundWindow()==params.hFocusWindow;
 for(unsigned i=0;i<6;++i){bool down=foreground&&(GetAsyncKeyState(keys[i])&0x8000);pressed[i]=down&&!held[i];held[i]=down;}
 if(!foreground){typing=false;return;}if(pressed[4]||pressed[5])typing=false;else if(pressed[1]||pressed[2]||pressed[3])typing=true;
 if(!pressed[0]||typing||menuOpen)return;
 auto images=std::atomic_load(&pictures::assets);auto camera=std::atomic_load(&source_camera::snapshot);
 CURSORINFO cursor{sizeof(CURSORINFO),0,nullptr,{0,0}};auto now=GetTickCount64();
 if(settings->enabled&&settings->bindT&&now-last>=500&&dango::usableCamera(*camera,now)&&images->decoded.count(settings->selected)&&GetCursorInfo(&cursor)&&!(cursor.flags&CURSOR_SHOWING)){pictures::requestPlace();last=now;}
}
inline void stop(){typing=false;for(auto& key:held)key=false;}
}
