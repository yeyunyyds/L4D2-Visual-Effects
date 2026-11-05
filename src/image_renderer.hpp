#pragma once
#include <d3d9.h>
#include <mutex>
#include "image_runtime.hpp"
namespace visual::picturerender {
inline std::atomic<const char*> status{u8"挂画渲染关闭"};
struct Texture {IDirect3DTexture9* texture=nullptr;unsigned width=0,height=0;std::uint64_t revision=0;std::size_t bytes=0;};
inline std::mutex mutex;
inline std::map<std::string,Texture> cache;
inline std::map<std::string,std::uint64_t> retry;
inline IDirect3DDevice9* owner=nullptr;
inline void releaseLocked(){for(auto& item:cache)if(item.second.texture)item.second.texture->Release();cache.clear();retry.clear();owner=nullptr;}
inline void release(){std::lock_guard<std::mutex> lock(mutex);releaseLocked();}
inline void maintain(IDirect3DDevice9* device){
 std::lock_guard<std::mutex> lock(mutex);auto settings=std::atomic_load(&pictures::options);auto scene=std::atomic_load(&pictures::sceneFrame);
 if(owner&&owner!=device)releaseLocked();
 if(!settings->enabled||scene->items.empty()){releaseLocked();return;}
 for(auto i=cache.begin();i!=cache.end();){bool used=false;for(const auto& item:scene->items)used|=item.key==i->first;
  if(!used){if(i->second.texture)i->second.texture->Release();i=cache.erase(i);}else ++i;
 }
}
inline bool needed(){
 auto settings=std::atomic_load(&pictures::options);if(!settings->enabled)return false;
 auto scene=std::atomic_load(&pictures::sceneFrame);auto camera=std::atomic_load(&source_camera::snapshot);auto tick=GetTickCount64();
 for(const auto& item:scene->items)if(pictures::visibleLease(item,*camera,tick))return true;
 return false;
}
inline Texture* texture(IDirect3DDevice9* device,const std::string& key,const pictures::Pixels& pixels,std::uint64_t now){
 auto found=cache.find(key);if(found!=cache.end()&&found->second.revision==pixels.revision)return &found->second;
 if(found!=cache.end()){found->second.texture->Release();cache.erase(found);}
 auto delayed=retry.find(key);if(delayed!=retry.end()&&now<delayed->second)return nullptr;retry[key]=now+2000;
 unsigned width=pictures::textureEdge(pixels.width),height=pictures::textureEdge(pixels.height);
 D3DCAPS9 caps{};if(FAILED(device->GetDeviceCaps(&caps))||width>caps.MaxTextureWidth||height>caps.MaxTextureHeight){status=u8"图片超过显卡纹理上限";return nullptr;}
 std::size_t used=0;for(const auto& item:cache)used+=item.second.bytes;std::size_t bytes=std::size_t(width)*height*4;
 if(bytes>pictures::CacheBudget-used){status=u8"显存缓存已满，请删除旧挂画";return nullptr;}
 IDirect3DTexture9* created=nullptr;
 if(FAILED(device->CreateTexture(width,height,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&created,nullptr))){status=u8"创建图片纹理失败";return nullptr;}
 D3DLOCKED_RECT lock{};
 if(FAILED(created->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD))){created->Release();status=u8"图片纹理写入失败";return nullptr;}
 bool valid=lock.pBits&&lock.Pitch>=int(width*4)&&pixels.bgra.size()==std::size_t(pixels.width)*pixels.height*4;
 if(valid)for(unsigned y=0;y<height;++y){auto row=static_cast<unsigned char*>(lock.pBits)+y*lock.Pitch;std::memset(row,0,width*4);if(y<pixels.height)std::memcpy(row,pixels.bgra.data()+std::size_t(y)*pixels.width*4,pixels.width*4);}
 HRESULT unlocked=created->UnlockRect(0);if(!valid||FAILED(unlocked)){created->Release();return nullptr;}
 retry.erase(key);try{auto entry=cache.emplace(key,Texture{created,width,height,pixels.revision,bytes});return &entry.first->second;}catch(...){created->Release();throw;}
}
struct Vertex {float x,y,z,rhw;DWORD color;float u,v;};
inline void draw(IDirect3DDevice9* device,float width,float height){
 auto settings=std::atomic_load(&pictures::options);if(!settings->enabled)return;
 auto scene=std::atomic_load(&pictures::sceneFrame);auto camera=std::atomic_load(&source_camera::snapshot);auto decoded=std::atomic_load(&pictures::assets);auto now=GetTickCount64();
 if(scene->epoch!=camera->epoch||!dango::usableCamera(*camera,now)||now<scene->tick||now-scene->tick>350)return;
 std::lock_guard<std::mutex> lock(mutex);if(owner!=device){releaseLocked();owner=device;}
 IDirect3DStateBlock9* state=nullptr;if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))||!state){status=u8"无法保存挂画渲染状态";return;}
 if(FAILED(state->Capture())){state->Release();status=u8"无法保存挂画渲染状态";return;}
 struct Restore {IDirect3DStateBlock9* state;~Restore(){state->Apply();state->Release();}} restore{state};
 device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
 device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
 device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_FOGENABLE,FALSE);
 device->SetRenderState(D3DRS_STENCILENABLE,FALSE);device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
 device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
 device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);device->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
 device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);device->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
 device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);device->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
 device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);device->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
 device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);device->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
 device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
 device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
 D3DVIEWPORT9 viewport{0,0,DWORD(width),DWORD(height),0,1};device->SetViewport(&viewport);
 for(const auto& item:scene->items){
  if(!pictures::visibleLease(item,*camera,now))continue;
  auto pixels=decoded->decoded.find(item.key);if(pixels==decoded->decoded.end())continue;
  std::array<dango::wall::Vertex,4> projected;if(!dango::wall::projectPanel(item.panel,*camera,width,height,projected))continue;
  auto tex=texture(device,item.key,*pixels->second,now);if(!tex)continue;
  DWORD alpha=DWORD(item.opacity*255/10),color=alpha*0x01010101u;Vertex vertices[4];
  for(unsigned i=0;i<4;++i){const auto& p=projected[i];vertices[i]={p.x-.5f,p.y-.5f,p.z,p.rhw,color,p.u*float(pixels->second->width)/tex->width,p.v*float(pixels->second->height)/tex->height};}
  device->SetTexture(0,tex->texture);
  if(SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex))))status=u8"挂画已绘制（仅本机可见）";
 }
}
} // namespace visual::picturerender
