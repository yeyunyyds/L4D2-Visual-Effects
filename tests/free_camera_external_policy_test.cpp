#include "free_camera_external_policy.hpp"
#include <cassert>
using visual::freecamera::external::Owner;
using visual::freecamera::external::Policy;
int main(){
 Policy p;
 assert(!p.allowed&&!p.on&&!p.desired&&p.idleSeconds==3);
 assert(!p.request(true,0));
 p.setIdle(0);assert(p.idleSeconds==1);
 p.setIdle(31);assert(p.idleSeconds==30);
 p.setIdle(3);
 p.allow(true,1000);
 assert(p.request(true,1000)&&p.on);
 assert(p.select(1000)==Owner::User);
 p.setPose(1001);
 assert(p.select(3999)==Owner::User);
 assert(p.select(4000)==Owner::External);
 p.noteInput(4050);
 assert(p.on&&p.select(4050)==Owner::User);
 assert(p.select(7049)==Owner::User);
 assert(p.select(7050)==Owner::External);
 p.toggle(7060); // F6/menu uses the same switch, even during external ownership
 assert(!p.on&&!p.desired&&p.select(7060)==Owner::Native);
 p.setPose(7100);assert(!p.hasPose&&p.select(11000)==Owner::Native);
 p.toggle(11000);assert(p.on&&p.select(15000)==Owner::User);
 assert(p.request(true,15000));p.setPose(15000);
 assert(p.select(18000)==Owner::External);
 p.allow(false,18001);assert(!p.on&&!p.desired);
 p.allow(true,18002);assert(p.request(true,18003));p.setPose(18003);
 assert(p.select(21003)==Owner::External);
 p.noteInput(21004);p.setPose(22000);
 assert(p.select(24003)==Owner::User&&p.select(24004)==Owner::External);
 p.request(false,24005);assert(!p.on&&!p.desired);
 p.toggle(25000);p.request(false,25001);assert(p.on); // stale client stop cannot close manual camera
 assert(p.request(true,25002));p.setPose(25002);
 p.expire(40003);assert(!p.on&&!p.desired);
 p.allow(false,41000);p.toggle(41001);assert(p.on);
 p.allow(false,41002);assert(p.on); // permission alone does not disable ordinary manual camera
 p.stop();assert(p.select(41003)==Owner::Native);
}
