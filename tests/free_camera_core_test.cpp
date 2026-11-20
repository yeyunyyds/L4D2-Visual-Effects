#include "../src/free_camera_core.hpp"
#include <cassert>
#include <cmath>
using namespace visual::freecamera::core;
int main(){
 Camera c;c.start({0,0,64},{0,0,0});assert(c.ready);
 Input i;i.forward=1;c.move(i,.05f);assert(c.origin.x>12.99f&&c.origin.x<13.01f);
 // Source yaw 0: +X forward, -Y is right. D should move -Y.
 i={};i.side=1;c.move(i,.05f);assert(c.origin.y< -12.99f);
 i={};i.side=-1;c.move(i,.05f);assert(std::abs(c.origin.y)<.01f);
 i={};i.up=1;c.move(i,.05f);assert(c.origin.z>76.99f);
 // Positive mouseX turns right (negative Source yaw); positive mouseY looks down.
 i={};i.dx=100;c.move(i,.01f);assert(c.angles.y<0.f);
 const float rightYaw=c.angles.y;
 i={};i.dx=-100;c.move(i,.01f);assert(std::abs(c.angles.y)<.001f);
 i={};i.dy=100000;i.dx=100000;c.move(i,.01f);
 assert(c.angles.x==89.f&&std::abs(c.angles.y)<=180.f);
 float x=c.origin.x;i={};i.forward=1;c.move(i,999.f);
 assert(valid(c.origin)&&std::abs(c.origin.x-x)<400.f);
}
