#pragma once
#include <algorithm>
#include <cmath>

namespace visual::freecamera::core {
struct Vec {float x=0,y=0,z=0;};
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,float s){return {a.x*s,a.y*s,a.z*s};}
inline bool valid(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float magnitude(Vec a){return std::sqrt(dot(a,a));}

// Only WASD, Space, Shift and mouse control the fly camera.
struct Input {float forward=0,side=0,up=0,dx=0,dy=0;};

struct Camera {
 Vec origin{},angles{},velocity{};
 // 'speed' is now the terminal flying speed (Source world units / second).
 float speed=1000.f,sensitivity=1.f;
 bool ready=false;

 void halt(){velocity={};}
 float currentSpeed()const{return magnitude(velocity);}
 void start(Vec p,Vec a){
  ready=valid(p)&&valid(a);
  halt();
  if(ready){origin=p;angles=a;}
 }

 void move(Input i,float dt){
  if(!ready||!std::isfinite(dt)||dt<=0.f)return;
  dt=std::min(dt,.05f);
  // Mouse-look remains immediate. Positive mouse X is negative Source yaw.
  angles.x=std::clamp(angles.x+i.dy*.022f*sensitivity,-89.f,89.f);
  angles.y=std::remainder(angles.y-i.dx*.022f*sensitivity,360.f);
  angles.z=0;

  constexpr float rad=.0174532925199433f;
  const float y=angles.y*rad,p=angles.x*rad;
  // Source right basis is (+sin(yaw), -cos(yaw)) for zero roll.
  Vec wish{std::cos(p)*std::cos(y)*i.forward+std::sin(y)*i.side,
           std::cos(p)*std::sin(y)*i.forward-std::cos(y)*i.side,
           -std::sin(p)*i.forward+i.up};
  const float wishLen=magnitude(wish);
  if(wishLen>1.f)wish=wish*(1.f/wishLen);
  const bool moving=wishLen>0.0001f;
  const float cap=std::clamp(std::isfinite(speed)?speed:1000.f,30.f,2500.f);
  const Vec target=wish*cap;

  // Native-like flight: an immediate gentle launch avoids sluggish taps,
  // then physical velocity ramps toward cap. No key-hold timers or frames.
  Vec previous=velocity;
  if(moving&&currentSpeed()<.001f){
   velocity=wish*std::min(cap,220.f);
   previous=velocity;
  }
  const float current=currentSpeed();
  // Stronger braking on release or sharp direction changes. Normal movement
  // takes roughly .35-.45 seconds to reach cap; releasing stops in ~.13 s.
  const bool sharpTurn=moving&&current>.001f&&dot(velocity,wish)/current<.70f;
  const float acceleration=cap/.45f;
  const float braking=cap/.13f;
  const float maxDelta=(moving&&!sharpTurn?acceleration:braking)*dt;
  Vec difference=target-velocity;
  const float deltaLen=magnitude(difference);
  if(deltaLen<=maxDelta)velocity=target;
  else if(deltaLen>0.f)velocity=velocity+difference*(maxDelta/deltaLen);
  if(!moving&&magnitude(velocity)<.001f)velocity={};

  // Integrate the mean velocity for better consistency across tick rates.
  const Vec next=origin+(previous+velocity)*(.5f*dt);
  if(valid(next)&&valid(velocity))origin=next;
  else halt();
 }
};
} // namespace visual::freecamera::core
