#include "free_camera_external_policy.hpp"
#include <cassert>
using visual::freecamera::external::Owner;
using visual::freecamera::external::Policy;
int main(){
 Policy p;
 assert(!p.request(true,0));
 p.allow(true,1000);
 assert(p.request(true,1000)&&!p.on);
 p.setPose(1001);
 assert(p.select(11000)==Owner::Native); // status is not activation
 p.advance(10999);assert(!p.on);
 p.noteInput(11000);p.setPose(11000);
 p.advance(20999);assert(!p.on);
 p.advance(21000);assert(p.on&&p.select(21000)==Owner::External);
 p.noteInput(21001);assert(p.select(21001)==Owner::User);
 p.setPose(22000);p.advance(24001);assert(p.select(24001)==Owner::External);
 p.toggle(24002);assert(!p.on&&!p.desired);
 p.setPose(24003);assert(!p.hasPose);
 p.request(true,25000);p.setPose(25001);p.toggle(25002);assert(!p.on&&!p.desired);
 p.toggle(26000);assert(p.on);p.request(false,26001);assert(p.on);
 p.request(true,26002);p.setPose(26003);p.advance(29002);assert(p.select(29002)==Owner::External);
 p.allow(false,29003);assert(!p.on&&!p.desired);
 p.allow(true,30000);p.request(true,30000);p.setPose(30001);p.advance(45002);assert(!p.desired&&!p.on);
 p.initialIdleSeconds=1;p.request(true,46000);p.setPose(46001);p.advance(47000);assert(p.on);
 p.stop();assert(p.select(47001)==Owner::Native);
}
