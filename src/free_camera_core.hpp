#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace visual::freecamera::core {
struct Vec {float x=0,y=0,z=0;};
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,float s){return {a.x*s,a.y*s,a.z*s};}
inline bool valid(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline float magnitude(Vec a){return std::sqrt(dot(a,a));}

// Repeated OverrideView callbacks within one View_Render share one pose.
enum class FrameKind { First, Step, Repeat, Gap };
struct FrameTick {FrameKind kind=FrameKind::First;float dt=0.f;};
struct FrameClock {
 bool initialized=false;
 std::uint64_t id=0;
 double seconds=0.0;
 void reset(){initialized=false;id=0;seconds=0.0;}
 FrameTick next(std::uint64_t frame,double atSeconds){
  if(initialized&&frame==id)return {FrameKind::Repeat,0.f};
  const bool first=!initialized;
  const double elapsed=atSeconds-seconds;
  const bool restarted=!first&&frame<id;
  initialized=true;id=frame;seconds=atSeconds;
  if(first)return {FrameKind::First,0.f};
  if(restarted||!std::isfinite(elapsed)||elapsed<=0.0||elapsed>.20)
   return {FrameKind::Gap,0.f};
  return {FrameKind::Step,static_cast<float>(std::min(elapsed,.05))};
 }
};

struct Input {float forward=0,side=0,up=0,dx=0,dy=0;};
// Discrete mouse counts must survive command/render frequency mismatches.
// Clamp only the amount taken per render, keeping any queued remainder.
struct MouseCounts{std::int64_t x=0,y=0;};
struct MouseQueue{
 MouseCounts pending{};
 void push(std::int64_t dx,std::int64_t dy) noexcept {
  pending.x+=dx;pending.y+=dy;
 }
 MouseCounts take(std::int64_t limit=4096) noexcept {
  MouseCounts n{std::clamp(pending.x,-limit,limit),
                std::clamp(pending.y,-limit,limit)};
  pending.x-=n.x;pending.y-=n.y;
  return n;
 }
 MouseCounts drop() noexcept {const auto n=pending;pending={};return n;}
 MouseCounts peek()const noexcept{return pending;}
};
struct Camera {
 Vec origin{},angles{},velocity{};
 float speed=1000.f,sensitivity=1.f;
 bool ready=false;
 void halt(){velocity={};}
 float currentSpeed()const{return magnitude(velocity);}
 void start(Vec p,Vec a){
  ready=valid(p)&&valid(a);
  halt();
  if(ready){origin=p;angles=a;}
 }
 // Mouse-look is immediate. Input tick mouse deltas are applied once per
 // real render, not once per synthetic FG display frame.
 void look(float dx,float dy){
  if(!ready||!std::isfinite(dx)||!std::isfinite(dy))return;
  angles.x=std::clamp(angles.x+dy*.022f*sensitivity,-89.f,89.f);
  angles.y=std::remainder(angles.y-dx*.022f*sensitivity,360.f);
  angles.z=0;
 }
 // Exact integration of dv/dt=(target-v)/tau. Continuous velocity, and
 // no extra interpolation delay on the finished camera position.
 void advance(Input i,float dt){
  if(!ready||!std::isfinite(dt)||dt<=0.f)return;
  dt=std::min(dt,.05f);
  constexpr float rad=.0174532925199433f;
  const float y=angles.y*rad,p=angles.x*rad;
  // Source right basis at zero roll is (+sin(yaw), -cos(yaw)).
  Vec wish{std::cos(p)*std::cos(y)*i.forward+std::sin(y)*i.side,
           std::cos(p)*std::sin(y)*i.forward-std::cos(y)*i.side,
           -std::sin(p)*i.forward+i.up};
  const float len=magnitude(wish);
  if(!std::isfinite(len))return;
  if(len>1.f)wish=wish*(1.f/len);
  const bool moving=len>.0001f;
  const float cap=std::clamp(std::isfinite(speed)?speed:1000.f,30.f,2500.f);
  const Vec target=wish*cap;
  const float oldSpeed=currentSpeed();
  const bool sharp=moving&&oldSpeed>.001f&&dot(velocity,wish)<oldSpeed*.7f;
  const float tau=moving?(sharp?.085f:.12f):.065f;
  const float blend=-std::expm1(-dt/tau);
  const Vec previous=velocity;
  const Vec nextVelocity=previous+(target-previous)*blend;
  const Vec displacement=target*dt+(previous-target)*(tau*blend);
  const Vec next=origin+displacement;
  if(!valid(nextVelocity)||!valid(next)){halt();return;}
  velocity=nextVelocity;
  if(!moving&&magnitude(velocity)<.01f)velocity={};
  origin=next;
 }
 // Backwards-compatible math helper, not used by the live render path.
 void move(Input i,float dt){look(i.dx,i.dy);advance(i,dt);}
};
} // namespace visual::freecamera::core
