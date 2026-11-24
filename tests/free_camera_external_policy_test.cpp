#include "free_camera_external_policy.hpp"
#include "plugin_lifetime.hpp"
#include <cassert>
using visual::freecamera::external::Owner;
using visual::freecamera::external::Policy;
int main(){
 {visual::lifetime::Guard guard;assert(guard&&visual::lifetime::callbacks.load()==1);visual::lifetime::requestStop();visual::lifetime::Guard denied;assert(!denied&&visual::lifetime::callbacks.load()==1);}assert(!visual::lifetime::callbacks.load());
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
 // An older input timestamp may arrive after a fresh concurrent heartbeat.
 p.setPose(48000);p.advance(47999);assert(p.desired&&p.on&&p.select(47999)==Owner::External);
 p.setPose(47998);assert(p.lastPoseMs==48000);p.noteInput(48002);p.noteInput(48001);assert(p.lastInputMs==48002);
 assert(!p.expire(63000)&&p.desired);assert(p.expire(63001)&&!p.desired&&!p.on);
 p.stop();assert(p.select(47001)==Owner::Native);
 Policy native;native.allow(true,1000);native.nativeOnInput=true;assert(native.request(true,1000));native.setPose(1001);native.advance(11000);assert(native.select(11000)==Owner::External);native.externalWasDriving=true;
 native.noteInput(11001,false);assert(native.on&&native.select(11001)==Owner::User); // menu pause is not takeover
 native.noteInput(11002,true);assert(!native.on&&native.desired&&native.select(11002)==Owner::Native);
 native.setPose(12000);native.advance(21001);assert(!native.on);native.advance(21002);assert(native.select(21002)==Owner::External);
 native.externalWasDriving=true;native.nativeOnInput=false;native.noteInput(21003,true);assert(native.on&&native.select(21003)==Owner::User);
}
