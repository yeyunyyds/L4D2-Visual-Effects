#pragma once
#include "tank_runtime.hpp"
#include "spray_display.hpp"
namespace visual::tank {
inline bool needed(){if(faulted)return false;auto s=std::atomic_load(&options);auto f=std::atomic_load(&snapshot);auto c=std::atomic_load(&source_camera::snapshot);auto now=GetTickCount64();return s->enabled&&s->draw&&!pictures::displayPaused&&c->active&&c->epoch==f->epoch&&now>=f->tick&&now-f->tick<=250&&!f->tanks.empty();}
inline bool project(Vec3 p,const dango::GameCameraFrame& c,float w,float h,float& x,float& y){Vec3 v;if(!dango::transform(c.worldToClip,p,v,true)||v.z<0||v.z>1||std::abs(v.x)>1.1f||std::abs(v.y)>1.1f)return false;x=(v.x+1)*w*.5f;y=(1-v.y)*h*.5f;return true;}
inline void pollMarkKey(bool foreground,bool menu){static bool held=false;auto cfg=std::atomic_load(&options);bool down=foreground&&(GetAsyncKeyState(VK_F7)&0x8000);bool edge=down&&!held;held=down;
 if(!edge||menu||!cfg->enabled||!cfg->markKey)return;auto f=std::atomic_load(&snapshot);auto camera=std::atomic_load(&source_camera::snapshot);dango::CameraPose eye;
 if(f->epoch!=camera->epoch||!dango::usableCamera(*camera,GetTickCount64())||!dango::cameraPose(*camera,eye))return;
 const Observation* best=nullptr;float bestAngle=1;
 for(const auto& o:f->tanks){auto delta=o.origin-eye.eye;float distance=dango::length(delta);if(distance<1||distance>2048)continue;float alignment=(delta.x*eye.forward.x+delta.y*eye.forward.y+delta.z*eye.forward.z)/distance;if(alignment>.5f&&1-alignment<bestAngle){bestAngle=1-alignment;best=&o;}}
 if(best)mark(f->epoch,best->handle);else markStatus=u8"准星前未找到Tank，标记未提交";
}
inline void draw(IDirect3DDevice9* device,float width,float height){
 if(!needed())return;auto estimates=std::atomic_load(&observedSnapshot);auto f=std::atomic_load(&snapshot);auto c=std::atomic_load(&source_camera::snapshot);IDirect3DStateBlock9* state=nullptr;
 if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))||!state)return;if(FAILED(state->Capture())){state->Release();return;}
 struct Restore{IDirect3DStateBlock9* s;~Restore(){s->Apply();s->Release();}}restore{state};
 struct Vertex{float x,y,z,w;DWORD color;};std::vector<Vertex> vertices;vertices.reserve(1200);
 auto line=[&](Vec3 a,Vec3 b,DWORD color){float ax,ay,bx,by;if(project(a,*c,width,height,ax,ay)&&project(b,*c,width,height,bx,by)){vertices.push_back({ax,ay,0,1,color});vertices.push_back({bx,by,0,1,color});}};
 for(const auto& o:f->tanks){auto center=o.origin+Vec3{0,0,5};auto aimVector=forward(o.heading);if(o.eye){float p=o.pitch*3.14159265358979323846f/180;aimVector=aimVector*std::cos(p);aimVector.z=-std::sin(p);}line(center,center+aimVector*60,o.eye?0xff00ccff:0xff999999);
  if(o.target)line(center+Vec3{0,0,60},o.target->origin+Vec3{0,0,60},o.target->authoritativeField?0xffff8866:0xffddcc55);
  // Grey = unknown; amber/purple are animation/manual stopwatch ESTIMATES.
  {auto start=o.origin+Vec3{0,0,91};float x,y;if(project(start,*c,width,height,x,y)){
    auto e=estimates->find(o.handle);bool manual=e!=estimates->end()&&std::string(e->second.source)=="manual-observed-punch";DWORD color=!o.cooldown?0xff888888:manual?0xffcc88ff:0xffddaa44;
    for(int dy=-5;dy<=3;++dy){DWORD background=(dy==-5||dy==3)?0xff999999:0xff333333;vertices.push_back({x-40,y+dy,0,1,background});vertices.push_back({x+40,y+dy,0,1,background});}
    if(o.cooldown){float scale=e!=estimates->end()&&e->second.span&&*e->second.span>0?*e->second.span:std::max(1.f,*o.cooldown);float fill=std::clamp(*o.cooldown/scale,0.f,1.f);for(int dy=-3;dy<=1;++dy){vertices.push_back({x-38,y+dy,0,1,color});vertices.push_back({x-38+76*fill,y+dy,0,1,color});}}
   }}
  if(!o.geometry)continue;
  for(unsigned i=0;i<32;++i){float a=o.heading-o.halfArc+o.halfArc*2*i/32,b=o.heading-o.halfArc+o.halfArc*2*(i+1)/32;line(center+forward(a)*o.radius,center+forward(b)*o.radius,0xffddaa44);
   if((i%2)==0){auto p=o.predicted+Vec3{0,0,5};line(p+forward(a)*o.radius,p+forward(b)*o.radius,0xff7799cc);}
  }line(center,center+forward(o.heading-o.halfArc)*o.radius,0xffddaa44);line(center,center+forward(o.heading+o.halfArc)*o.radius,0xffddaa44);
 }
 if(vertices.empty())return;device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetTexture(0,nullptr);device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_FOGENABLE,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_STENCILENABLE,FALSE);device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);device->SetRenderState(D3DRS_CLIPPLANEENABLE,0);device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 D3DVIEWPORT9 viewport{0,0,DWORD(width),DWORD(height),0,1};device->SetViewport(&viewport);device->DrawPrimitiveUP(D3DPT_LINELIST,UINT(vertices.size()/2),vertices.data(),sizeof(Vertex));
}
}
