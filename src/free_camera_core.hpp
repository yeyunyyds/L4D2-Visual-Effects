#pragma once
#include <algorithm>
#include <cmath>
namespace visual::freecamera::core {
struct Vec {float x=0,y=0,z=0;};
inline bool valid(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
// Only WASD, Space, Shift and mouse control the fly camera by default.
struct Input {float forward=0,side=0,up=0,dx=0,dy=0;};
struct Camera {
 Vec origin{},angles{};float speed=260.f,sensitivity=1.f;bool ready=false;
 void start(Vec p,Vec a){origin=p;angles=a;ready=valid(p)&&valid(a);}
 void move(Input i,float dt){
  if(!ready)return;
  dt=std::clamp(dt,0.f,.05f);
  angles.x=std::clamp(angles.x+i.dy*.022f*sensitivity,-89.f,89.f);
  // Source yaw grows counter-clockwise; positive mouse X should turn RIGHT.
  angles.y=std::remainder(angles.y-i.dx*.022f*sensitivity,360.f);
  angles.z=0;
  constexpr float r=.0174532925199433f;
  const float y=angles.y*r,p=angles.x*r;
  // Source's AngleVectors right vector at pitch=roll=0 is (+sin(yaw), -cos(yaw)).
  // At yaw=0, D moves toward -Y, not +Y.
  Vec v{std::cos(p)*std::cos(y)*i.forward+std::sin(y)*i.side,
        std::cos(p)*std::sin(y)*i.forward-std::cos(y)*i.side,
        -std::sin(p)*i.forward+i.up};
  const float n=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
  if(n>1.f){v.x/=n;v.y/=n;v.z/=n;}
  const float scale=std::clamp(speed,30.f,2500.f)*dt;
  Vec next{origin.x+v.x*scale,origin.y+v.y*scale,origin.z+v.z*scale};
  if(valid(next))origin=next;
 }
};
}
