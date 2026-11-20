#include "../src/free_camera_core.hpp"
#include <cassert>
#include <cmath>
using namespace visual::freecamera::core;
static void near(float a,float b,float eps=.02f){assert(std::abs(a-b)<=eps);}
static void step(Camera& c,Input i,float seconds,float dt=.01f){
 for(float elapsed=0;elapsed<seconds-.000001f;elapsed+=dt)
  c.advance(i,std::min(dt,seconds-elapsed));
}
int main(){
 Input forward{};forward.forward=1;
 Camera c;c.start({0,0,64},{0,0,0});
 assert(c.ready&&c.currentSpeed()==0.f);
 c.advance(forward,.01f);
 assert(c.currentSpeed()>0.f&&c.currentSpeed()<100.f);
 assert(c.origin.x>0.f&&c.origin.x<1.f);
 step(c,forward,.20f);assert(c.currentSpeed()>750.f);
 step(c,forward,.30f);assert(c.currentSpeed()>975.f&&c.currentSpeed()<1000.01f);
 step(c,forward,1.f);near(c.currentSpeed(),1000.f,.01f);

 // Release never drops velocity instantaneously.
 Input released{};const float before=c.currentSpeed(),x=c.origin.x;
 c.advance(released,.01f);
 assert(c.currentSpeed()>0.f&&c.currentSpeed()<before);
 assert(c.origin.x>x&&c.origin.x-x<10.f);
 step(c,released,.25f);assert(c.currentSpeed()<25.f);

 c.start({0,0,64},{0,0,0});
 step(c,forward,.8f);
 Input reverse{};reverse.forward=-1;
 const float position=c.origin.x;
 c.advance(reverse,.001f);
 assert(std::abs(c.origin.x-position)<1.1f);
 step(c,reverse,.5f);assert(c.velocity.x<0.f&&c.currentSpeed()<1000.01f);

 c.start({0,0,64},{0,0,0});
 Input right{};right.side=1;
 step(c,right,.5f);assert(c.origin.y<0.f&&std::abs(c.origin.x)<.001f);
 c.start({0,0,64},{0,0,0});
 Input diag{};diag.forward=1;diag.side=1;
 step(c,diag,.8f);near(c.currentSpeed(),1000.f,2.f);

 c.start({0,0,64},{0,0,0});
 Input up{};up.up=1;
 step(c,up,.2f);assert(c.origin.z>64.f);
 Input down{};down.up=-1;
 step(c,down,.5f);assert(c.velocity.z<0.f);
 c.start({0,0,64},{0,0,0});
 c.look(100.f,0.f);assert(c.angles.y<0.f);
 c.look(-100.f,0.f);near(c.angles.y,0.f,.0001f);
 c.look(0.f,100000.f);near(c.angles.x,89.f,.001f);

 FrameClock clock;
 auto f=clock.next(100,1.0);
 assert(f.kind==FrameKind::First&&f.dt==0.f);
 f=clock.next(100,1.002);
 assert(f.kind==FrameKind::Repeat&&f.dt==0.f);
 f=clock.next(101,1.01);
 assert(f.kind==FrameKind::Step);near(f.dt,.01f,.00001f);
 f=clock.next(102,1.5);assert(f.kind==FrameKind::Gap);
 f=clock.next(103,1.51);assert(f.kind==FrameKind::Step);
 f=clock.next(99,1.52);assert(f.kind==FrameKind::Gap);
 clock.reset();assert(clock.next(1,5.0).kind==FrameKind::First);
 assert(clock.next(2,5.001).kind==FrameKind::Step);

 c.start({0,0,64},{0,0,0});
 c.advance(forward,999.f);assert(c.origin.x<30.f);
 const float old=c.origin.x;
 c.advance(forward,-1.f);near(c.origin.x,old,.001f);
 c.halt();near(c.currentSpeed(),0.f,.001f);
 c.start({5,6,7},{0,0,0});near(c.currentSpeed(),0.f,.001f);

 // Analytic integration agrees at 50Hz and 100Hz for a held input.
 Camera fast,slow;
 fast.start({0,0,0},{0,0,0});
 slow.start({0,0,0},{0,0,0});
 step(fast,forward,1.f,.01f);
 step(slow,forward,1.f,.02f);
 near(fast.origin.x,slow.origin.x,.1f);
 near(fast.currentSpeed(),slow.currentSpeed(),.01f);
}
