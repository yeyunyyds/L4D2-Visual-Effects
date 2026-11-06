#pragma once
#include <d3d9.h>
#include <vector>
#include <mutex>
#include "dango/native_hud.hpp"
#include "diagnostics.hpp"
#include "image_renderer.hpp"
#include "image_keys.hpp"
#include "tank_overlay.hpp"
namespace visual::hudruntime {
inline dango::hud::NativeHud hud;
struct Hook{void** table;unsigned slot;void* before;void* after;};
inline std::vector<Hook> hooks;
inline std::mutex hookMutex;
inline std::recursive_mutex paintMutex;
inline IDirect3DSurface9* presentationBackup=nullptr;inline IDirect3DDevice9* backupOwner=nullptr;inline D3DSURFACE_DESC backupDescription{};
inline void releaseBackup(){if(presentationBackup)presentationBackup->Release();presentationBackup=nullptr;backupOwner=nullptr;}
inline bool acquireBackup(IDirect3DDevice9* device,const D3DSURFACE_DESC& d,IDirect3DSurface9** out){
 if(presentationBackup&&(backupOwner!=device||backupDescription.Width!=d.Width||backupDescription.Height!=d.Height||backupDescription.Format!=d.Format||backupDescription.MultiSampleType!=d.MultiSampleType||backupDescription.MultiSampleQuality!=d.MultiSampleQuality))releaseBackup();
 if(!presentationBackup){if(FAILED(device->CreateRenderTarget(d.Width,d.Height,d.Format,d.MultiSampleType,d.MultiSampleQuality,FALSE,&presentationBackup,nullptr)))return false;backupOwner=device;backupDescription=d;}
 presentationBackup->AddRef();*out=presentationBackup;return true;
}
inline thread_local unsigned depth=0;
inline pictures::DisplayGate displayGate;
inline std::atomic<unsigned long> presents{0},frames{0};
inline std::atomic<const char*> paintStatus{"No Present callback"};
inline void* original(void* object,unsigned slot){std::lock_guard<std::mutex> lock(hookMutex);auto table=*reinterpret_cast<void***>(object);for(auto& h:hooks)if(h.table==table&&h.slot==slot)return h.before;return nullptr;}
inline void attach(void* object,unsigned slot,void* after){if(!object)return;auto table=*reinterpret_cast<void***>(object);std::lock_guard<std::mutex> lock(hookMutex);for(auto& h:hooks)if(h.table==table&&h.slot==slot)return;DWORD old{};if(!VirtualProtect(table+slot,4,PAGE_READWRITE,&old))return;auto before=table[slot];hooks.push_back({table,slot,before,after});InterlockedExchangePointer(table+slot,after);DWORD ignored{};VirtualProtect(table+slot,4,old,&ignored);}
// Restore the pristine backbuffer after Present to prevent HUD ghosting on reuse.
// If backup fails, skip drawing rather than modifying an unprotected surface.
template<class Present> HRESULT paint(IDirect3DDevice9* device,IDirect3DSwapChain9* chain,Present present){
 std::lock_guard<std::recursive_mutex> lock(paintMutex);
 if(depth++) {auto hr=present();--depth;return hr;}
 if(FAILED(device->TestCooperativeLevel())){pictures::displayPaused=true;auto hr=present();--depth;return hr;}
 ++presents;picturerender::maintain(device);hud.pollHotkey(device);auto camera=std::atomic_load(&source_camera::snapshot);displayGate.poll((GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0,hud.isOpen(),picturekeys::foregroundGame(device),bool(camera->active));picturekeys::poll(device,hud.isOpen());tank::pollMarkKey(picturekeys::foregroundGame(device),hud.isOpen());if(!hud.isOpen()&&!picturerender::needed()&&!tank::needed())releaseBackup();paintStatus=hud.isOpen()?"HUD requested":"HUD closed";
 IDirect3DSurface9* back=nullptr;IDirect3DSurface9* backup=nullptr;D3DSURFACE_DESC desc{};
 bool ready=(hud.isOpen()||picturerender::needed()||tank::needed()) && SUCCEEDED(chain?chain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&back):device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back));
 if(ready)ready=SUCCEEDED(back->GetDesc(&desc))&&acquireBackup(device,desc,&backup)&&SUCCEEDED(device->StretchRect(back,nullptr,backup,nullptr,D3DTEXF_NONE));
 if(hud.isOpen()&&!ready)paintStatus="Backbuffer backup failed; HUD skipped";
 D3DVIEWPORT9 viewport{};bool haveViewport=SUCCEEDED(device->GetViewport(&viewport));
 IDirect3DSurface9* target=nullptr;
 if(ready){
  bool acquired=haveViewport&&SUCCEEDED(device->GetRenderTarget(0,&target))&&target;
  bool selected=acquired&&SUCCEEDED(device->SetRenderTarget(0,back));
  if(selected&&SUCCEEDED(device->BeginScene())){
   try{picturerender::draw(device,float(desc.Width),float(desc.Height));}catch(...){picturerender::status=u8"挂画渲染异常，请关闭后重开";}
   try{tank::draw(device,float(desc.Width),float(desc.Height));}catch(...){}
   hud.frame(device,float(desc.Width),float(desc.Height));++frames;paintStatus="Overlay frame submitted";device->EndScene();
  }else {paintStatus="Overlay render target/BeginScene unavailable";picturerender::status=u8"挂画目标表面不可用";}
  if(acquired)device->SetRenderTarget(0,target);if(target)target->Release();if(haveViewport)device->SetViewport(&viewport);
 }
 auto hr=present();
 if(ready&&SUCCEEDED(hr))device->StretchRect(backup,nullptr,back,nullptr,D3DTEXF_NONE);
 if(backup)backup->Release();if(back)back->Release();--depth;return hr;
}
inline HRESULT WINAPI devicePresent(IDirect3DDevice9* d,const RECT* s,const RECT* t,HWND w,const RGNDATA* r){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DDevice9*,const RECT*,const RECT*,HWND,const RGNDATA*)>(original(d,17));return paint(d,nullptr,[&]{return f(d,s,t,w,r);});}
inline HRESULT WINAPI chainPresent(IDirect3DSwapChain9* c,const RECT* s,const RECT* t,HWND w,const RGNDATA* r,DWORD flags){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DSwapChain9*,const RECT*,const RECT*,HWND,const RGNDATA*,DWORD)>(original(c,3));IDirect3DDevice9* d=nullptr;if(FAILED(c->GetDevice(&d)))return f(c,s,t,w,r,flags);auto hr=paint(d,c,[&]{return f(c,s,t,w,r,flags);});d->Release();return hr;}
inline void attachChain(IDirect3DSwapChain9* c){attach(c,3,reinterpret_cast<void*>(&chainPresent));}
inline HRESULT WINAPI getChain(IDirect3DDevice9* d,UINT i,IDirect3DSwapChain9** c){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DDevice9*,UINT,IDirect3DSwapChain9**)>(original(d,14));auto hr=f(d,i,c);if(SUCCEEDED(hr)&&c&&*c)attachChain(*c);return hr;}
inline HRESULT WINAPI addChain(IDirect3DDevice9* d,D3DPRESENT_PARAMETERS* p,IDirect3DSwapChain9** c){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*,IDirect3DSwapChain9**)>(original(d,13));auto hr=f(d,p,c);if(SUCCEEDED(hr)&&c&&*c)attachChain(*c);return hr;}
inline HRESULT WINAPI reset(IDirect3DDevice9* d,D3DPRESENT_PARAMETERS* p){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*)>(original(d,16));std::lock_guard<std::recursive_mutex> lock(paintMutex);releaseBackup();picturerender::release();hud.beforeReset();auto hr=f(d,p);hud.afterReset(hr);return hr;}
inline HRESULT WINAPI presentEx(IDirect3DDevice9Ex* d,const RECT* s,const RECT* t,HWND w,const RGNDATA* r,DWORD flags){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DDevice9Ex*,const RECT*,const RECT*,HWND,const RGNDATA*,DWORD)>(original(d,121));return paint(d,nullptr,[&]{return f(d,s,t,w,r,flags);});}
inline HRESULT WINAPI resetEx(IDirect3DDevice9Ex* d,D3DPRESENT_PARAMETERS* p,D3DDISPLAYMODEEX* mode){auto f=reinterpret_cast<HRESULT(WINAPI*)(IDirect3DDevice9Ex*,D3DPRESENT_PARAMETERS*,D3DDISPLAYMODEEX*)>(original(d,132));std::lock_guard<std::recursive_mutex> lock(paintMutex);releaseBackup();picturerender::release();hud.beforeReset();auto hr=f(d,p,mode);hud.afterReset(hr);return hr;}
inline void created(IDirect3DDevice9* d){if(!d)return;std::lock_guard<std::recursive_mutex> lock(paintMutex);releaseBackup();picturerender::release();hud.release();attach(d,17,reinterpret_cast<void*>(&devicePresent));attach(d,16,reinterpret_cast<void*>(&reset));attach(d,14,reinterpret_cast<void*>(&getChain));attach(d,13,reinterpret_cast<void*>(&addChain));IDirect3DSwapChain9* c=nullptr;if(SUCCEEDED(d->GetSwapChain(0,&c))){attachChain(c);c->Release();}IDirect3DDevice9Ex* ex=nullptr;if(SUCCEEDED(d->QueryInterface(__uuidof(IDirect3DDevice9Ex),reinterpret_cast<void**>(&ex)))){attach(ex,121,reinterpret_cast<void*>(&presentEx));attach(ex,132,reinterpret_cast<void*>(&resetEx));ex->Release();}diagnostics::write("D3D9 HUD Present/Reset callbacks installed");}
inline void stop(){picturekeys::stop();std::lock_guard<std::recursive_mutex> paintLock(paintMutex);releaseBackup();picturerender::release();hud.shutdown();std::lock_guard<std::mutex> lock(hookMutex);for(auto it=hooks.rbegin();it!=hooks.rend();++it){DWORD old{};if(VirtualProtect(it->table+it->slot,4,PAGE_READWRITE,&old)){InterlockedCompareExchangePointer(it->table+it->slot,it->before,it->after);DWORD ignored{};VirtualProtect(it->table+it->slot,4,old,&ignored);}}hooks.clear();}
}
