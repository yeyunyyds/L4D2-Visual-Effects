#include "free_camera_external_policy.hpp"
#include <cassert>
using visual::freecamera::external::Owner;
using visual::freecamera::external::Policy;
int main(){
 Policy p;p.allow(true,1000);assert(p.request(true,1000)&&!p.on);
 p.setPose(1001);p.advance(10999);assert(!p.on);
 p.advance(11000);assert(p.on&&p.select(11000)==Owner::External);
 p.noteInput(11001);assert(p.select(11001)==Owner::User);
 p.setPose(12000);p.advance(14001);assert(p.select(14001)==Owner::External);
 p.toggle(14002);assert(!p.on&&p.desired&&p.select(14002)==Owner::Native);
 p.setPose(20000);p.advance(24001);assert(!p.on);p.advance(24002);assert(p.on&&p.select(24002)==Owner::External);
 p.toggle(24003);assert(!p.on&&p.desired);p.request(false,24004);assert(!p.desired&&!p.on);
 p.toggle(25000);assert(p.on);p.request(false,25001);assert(p.on);
 p.request(true,25002);p.setPose(25003);p.advance(28002);assert(p.select(28002)==Owner::External);
 p.allow(false,28003);assert(!p.on&&!p.desired);
 p.allow(true,30000);p.request(true,30000);p.setPose(30001);p.advance(45002);assert(!p.desired&&!p.on);
 p.initialIdleSeconds=1;p.request(true,46000);p.setPose(46001);p.advance(47000);assert(p.on&&p.select(47000)==Owner::External);
 p.stop();assert(p.select(47001)==Owner::Native);
}
