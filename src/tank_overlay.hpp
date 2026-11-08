#pragma once
#include "tank_runtime.hpp"
#include "spray_display.hpp"
#include "tank_warning_geometry.hpp"
namespace visual::tank {
inline bool needed(){auto s=std::atomic_load(&options);auto f=std::atomic_load(&snapshot);auto c=std::atomic_load(&source_camera::snapshot);auto now=GetTickCount64();auto visual=std::atomic_load(&visualSettings);const char* reason=nullptr;
 if(faulted)reason="predictor faulted";else if(!s->enabled)reason="Tank prediction disabled";else if(!s->draw)reason="Tank drawing disabled";else if(!visual->showBar&&!visual->showRange&&!visual->showDirection&&!visual->showTarget)reason="all Tank display components disabled";else if(!display::foreground)reason="game not foreground";else if(display::paused)reason="Tank display suspended after Escape; rotate view or restore drawing";else if(!c->active)reason="camera inactive";else if(c->epoch!=f->epoch)reason="camera/result epoch mismatch";else if(now<f->tick||now-f->tick>250)reason="Tank result stale";else if(f->tanks.empty())reason="no Tank results";
 display::gateReason=reason?reason:"ready";return !reason;
}
inline void pollDisplay(const dango::GameCameraFrame& camera,bool escape,bool pluginMenu,bool foreground){static OverlayGate gate;auto cfg=std::atomic_load(&options);gate.poll(camera,escape,pluginMenu,foreground,cfg->enabled&&cfg->draw,display::resumeRequested.exchange(false));display::foreground=foreground;display::paused=gate.paused;}
inline bool project(Vec3 p,const dango::GameCameraFrame& c,float w,float h,float& x,float& y){Vec3 v;if(!dango::transform(c.worldToClip,p,v,true)||v.z<0||v.z>1||std::abs(v.x)>1.1f||std::abs(v.y)>1.1f)return false;x=(v.x+1)*w*.5f;y=(1-v.y)*h*.5f;return true;}
inline void draw(IDirect3DDevice9* device,float width,float height){
 if(!needed())return;++display::calls;display::tick=GetTickCount64();display::visibleBars=0;display::viewportWidth=width;display::viewportHeight=height;
 auto estimates=std::atomic_load(&observedSnapshot);auto f=std::atomic_load(&snapshot);auto c=std::atomic_load(&source_camera::snapshot);auto visual=std::atomic_load(&visualSettings);auto cfg=std::atomic_load(&options);auto colors=palette(visual->preset);float arrowWidth=colors.width*visual->arrowWidth,rangeWidth=colors.width*visual->rangeWidth,rangeDetailWidth=visual->rangeWidth;
 IDirect3DStateBlock9* state=nullptr;if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))||!state){display::stage="state block creation failed";return;}if(FAILED(state->Capture())){state->Release();display::stage="state capture failed";return;}
 struct Restore{IDirect3DStateBlock9* s;~Restore(){s->Apply();s->Release();}}restore{state};
 std::map<std::uint32_t,RenderAnchor> poses;{std::lock_guard<std::mutex> lock(animationMutex);poses=renderAnchors;}
 struct Vertex{float x,y,z,w;DWORD color;};std::vector<Vertex> fillVertices;fillVertices.reserve(960);
 std::vector<Segment> lines;lines.reserve(1800);
 auto line=[&](Vec3 a,Vec3 b,DWORD color,float thick=1){float ax,ay,bx,by;if(project(a,*c,width,height,ax,ay)&&project(b,*c,width,height,bx,by))stroke(lines,{ax,ay},{bx,by},color,thick);};
 for(const auto& o:f->tanks){bool danger=o.warning.known&&o.warning.inside;DWORD rangeColor=danger&&cfg->warnRange?0xffff3030:colors.range;auto center=o.origin+Vec3{0,0,5};auto aimVector=forward(o.heading);if(o.eye){float p=o.pitch*3.14159265358979323846f/180;aimVector=aimVector*std::cos(p);aimVector.z=-std::sin(p);}auto tip=center+aimVector*(60*visual->arrowSize);if(visual->showDirection){line(center,tip,colors.direction,arrowWidth);auto side=forward(o.heading+90);line(tip,center+aimVector*(49*visual->arrowSize)+side*(7*visual->arrowSize),colors.direction,arrowWidth);line(tip,center+aimVector*(49*visual->arrowSize)-side*(7*visual->arrowSize),colors.direction,arrowWidth);}
  if(visual->showTarget&&o.target)line(center+Vec3{0,0,60},o.target->origin+Vec3{0,0,60},o.target->authoritativeField?colors.direction:colors.future);
  auto bodyOrigin=o.origin;auto pose=poses.find(o.handle);if(pose!=poses.end()&&validRenderAnchor(pose->second,f->epoch,o.handle,GetTickCount64(),o.origin)){bodyOrigin=pose->second.position;++display::renderAnchorHits;}else ++display::networkAnchorFallbacks;
  auto at=visual->showBar?placeBar(*c,bodyOrigin+Vec3{0,0,45},width,height,*visual,o.heading):std::nullopt;
  if(at){++display::barProjected;++display::visibleBars;std::optional<float> fraction;if(o.cooldown){auto e=estimates->find(o.handle);float span=e!=estimates->end()&&e->second.span&&*e->second.span>0?*e->second.span:std::max(1.f,*o.cooldown);fraction=std::clamp(*o.cooldown/span,0.f,1.f);}barGeometry(lines,{at->x,at->y},*visual,at->scale,fraction,danger&&cfg->warnBar);}else if(visual->showBar)++display::barRejected;
  if(!o.geometry||!visual->showRange)continue;
  for(unsigned i=0;i<40;++i){float a=o.heading-o.halfArc+o.halfArc*2*i/40,b=o.heading-o.halfArc+o.halfArc*2*(i+1)/40;if(!colors.dashed||i%2==0)line(center+forward(a)*o.radius,center+forward(b)*o.radius,rangeColor,rangeWidth);
   if(danger&&cfg->warnFill){auto polygon=warningTriangle(*c,width,height,{center,center+forward(a)*o.radius,center+forward(b)*o.radius});for(std::size_t j=1;j+1<polygon.size();++j)for(auto p:{polygon[0],polygon[j],polygon[j+1]})fillVertices.push_back({p.x,p.y,0,1,0x30ff3030});}
   if(colors.ticks&&i%10==0)line(center+forward(a)*o.radius*.95f,center+forward(a)*o.radius*1.04f,rangeColor,rangeDetailWidth);
   if(i%2==0){auto p=o.predicted+Vec3{0,0,5};line(p+forward(a)*o.radius,p+forward(b)*o.radius,colors.future,rangeDetailWidth);}
  }line(center,center+forward(o.heading-o.halfArc)*o.radius,rangeColor,rangeDetailWidth);line(center,center+forward(o.heading+o.halfArc)*o.radius,rangeColor,rangeDetailWidth);
 }
 std::vector<Vertex> vertices;vertices.reserve(lines.size()*4);
 for(const auto& l:lines){auto dx=l.b.x-l.a.x,dy=l.b.y-l.a.y;float length=std::sqrt(dx*dx+dy*dy);float nx=length>.001f?-dy/length:0,ny=length>.001f?dx/length:0;unsigned count=unsigned(std::clamp(std::ceil(l.width),1.f,12.f));for(unsigned i=0;i<count;++i){float d=count>1?(float(i)/float(count-1)-.5f)*std::max(0.f,l.width-1):0;vertices.push_back({l.a.x+nx*d,l.a.y+ny*d,0,1,l.color});vertices.push_back({l.b.x+nx*d,l.b.y+ny*d,0,1,l.color});}}
 if(vertices.empty()&&fillVertices.empty()){display::stage="no projected overlay vertices";return;}
 device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetTexture(0,nullptr);device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_FOGENABLE,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_STENCILENABLE,FALSE);device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);device->SetRenderState(D3DRS_CLIPPLANEENABLE,0);device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 D3DVIEWPORT9 viewport{0,0,DWORD(width),DWORD(height),0,1};device->SetViewport(&viewport);HRESULT hr=S_OK;
 if(!fillVertices.empty()){device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);hr=device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,UINT(fillVertices.size()/3),fillVertices.data(),sizeof(Vertex));device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);}
 if(SUCCEEDED(hr)&&!vertices.empty())hr=device->DrawPrimitiveUP(D3DPT_LINELIST,UINT(vertices.size()/2),vertices.data(),sizeof(Vertex));display::result=hr;display::stage=SUCCEEDED(hr)?"overlay submitted":"DrawPrimitiveUP failed";if(SUCCEEDED(hr))++display::submitted;
}
}
