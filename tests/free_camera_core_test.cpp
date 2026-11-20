#include "../src/free_camera_core.hpp"
#include <cassert>
#include <cmath>
using namespace visual::freecamera::core;

static void close(float a,float b,float epsilon=.02f){
 assert(std::abs(a-b)<=epsilon);
}
static void step(Camera& camera,Input input,float seconds,float dt=.01f){
 for(float elapsed=0;elapsed<seconds-.000001f;elapsed+=dt)
  camera.move(input,std::min(dt,seconds-elapsed));
}
int main(){
 Camera c;c.start({0,0,64},{0,0,0});
 assert(c.ready&&c.currentSpeed()==0);
 Input forward{};forward.forward=1;
 c.move(forward,.01f);
 assert(c.currentSpeed()>220.f&&c.currentSpeed()<260.f); // responsive launch
 float early=c.currentSpeed();
 step(c,forward,.15f);
 assert(c.currentSpeed()>early+200.f);
 step(c,forward,.45f);
 close(c.currentSpeed(),1000.f);
 assert(c.origin.x>0&&std::abs(c.origin.y)<.001f);

 // Terminal velocity never increases forever, even when held.
 step(c,forward,1.f);
 close(c.currentSpeed(),1000.f);

 // Release: fast controlled braking, not an endless coast.
 Input released{};
 float releasedAt=c.origin.x;
 step(c,released,.20f);
 close(c.currentSpeed(),0.f,.01f);
 assert(c.origin.x>releasedAt&&c.origin.x-releasedAt<130.f);

 // Reversing direction brakes before accelerating in the opposite direction.
 c.start({0,0,64},{0,0,0});
 step(c,forward,.8f);
 Input reverse{};reverse.forward=-1;
 step(c,reverse,.40f);
 assert(c.velocity.x<0&&c.currentSpeed()<=1000.01f);

 // Source yaw 0: +X forward and -Y is RIGHT.
 c.start({0,0,64},{0,0,0});
 Input right{};right.side=1;
 step(c,right,.5f);
 assert(c.origin.y<0&&std::abs(c.origin.x)<.001f);

 // Full-speed diagonal flight is normalized, not faster than straight.
 c.start({0,0,64},{0,0,0});
 Input diagonal{};diagonal.forward=1;diagonal.side=1;
 step(c,diagonal,.8f);
 close(c.currentSpeed(),1000.f);

 // Space / Shift drive Z without depending on pitch.
 c.start({0,0,64},{0,0,0});
 Input up{};up.up=1;
 step(c,up,.2f);
 assert(c.origin.z>64);
 Input down{};down.up=-1;
 step(c,down,.5f);
 assert(c.origin.z<100);

 // Mouse stays direct and never inherits movement acceleration.
 c.start({0,0,64},{0,0,0});
 Input mouse{};mouse.dx=100;
 c.move(mouse,.01f);
 assert(c.angles.y<0);
 mouse.dx=-100;c.move(mouse,.01f);
 close(c.angles.y,0.f,.0001f);
 mouse={};mouse.dy=100000;c.move(mouse,.01f);
 close(c.angles.x,89.f,.001f);

 // Start/halt clean up inertia; invalid dt and huge frame time stay bounded.
 c.start({0,0,64},{0,0,0});
 c.move(forward,999.f);assert(c.origin.x<30.f);
 float old=c.origin.x;
 c.move(forward,-1.f);close(c.origin.x,old,.001f);
 c.halt();close(c.currentSpeed(),0.f,.001f);
 c.start({5,6,7},{0,0,0});close(c.currentSpeed(),0.f,.001f);

 // Flight distance should be nearly invariant between 100Hz and 50Hz.
 Camera highRate,lowRate;
 highRate.start({0,0,0},{0,0,0});
 lowRate.start({0,0,0},{0,0,0});
 step(highRate,forward,1.f,.01f);
 step(lowRate,forward,1.f,.02f);
 assert(std::abs(highRate.origin.x-lowRate.origin.x)<25.f);
}
