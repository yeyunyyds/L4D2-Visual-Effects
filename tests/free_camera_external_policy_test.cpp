#include "free_camera_external_policy.hpp"
#include <cassert>
using visual::freecamera::external::Owner;
using visual::freecamera::external::Policy;

int main(){
 Policy p;
 assert(!p.allowed&&!p.desired&&p.idleSeconds==3);
 assert(!p.request(true));
 assert(p.select(10000)==Owner::Native);
 p.setIdle(0);assert(p.idleSeconds==1);
 p.setIdle(31);assert(p.idleSeconds==30);
 p.setIdle(3);
 p.allow(true,1000);
 assert(p.request(true));p.setPose(1001);
 assert(p.select(2000)==Owner::Native); // never stolen during live play
 assert(p.remainingMs(2000)==2000);
 assert(p.select(4001)==Owner::External);
 p.onExternalFrame();
 p.noteInput(4050);                   // a single mouse count wins immediately
 assert(p.select(4050)==Owner::User);
 assert(p.select(7049)==Owner::User);
 assert(p.select(7050)==Owner::External);
 p.onExternalFrame();
 p.noteInput(7060);                   // repeated keyboard use restarts delay
 p.noteInput(7600);
 assert(p.select(10599)==Owner::User);
 assert(p.select(10600)==Owner::External);
 p.setManual(true);                   // manual FreeCamera takes precedence
 assert(p.select(12000)==Owner::User);
 p.setManual(false);
 assert(p.select(12000)==Owner::External);
 assert(p.select(16002)==Owner::Native); // stale provider expires safely
 p.setPose(16003);
 assert(p.select(16004)==Owner::External);
 p.request(false);
 assert(p.select(16005)==Owner::Native);
 assert(!p.hasPose);
 // A fresh request requires a new pose, never replays a stale coordinate.
 assert(p.request(true));
 assert(p.select(20000)==Owner::Native);
 p.setPose(20000);
 assert(p.select(20000)==Owner::External);
 p.onExternalFrame();
 p.allow(false,20001);               // explicit user menu disable wins
 assert(p.select(50000)==Owner::Native&&!p.desired);
 assert(!p.request(true));
 p.allow(true,50000);
 assert(p.request(true));
 p.setPose(50000);
 p.noteInput(50001);
 assert(p.select(50002)==Owner::Native); // fresh activation waits as native
 p.setManual(true);
 assert(p.select(50003)==Owner::User);
 p.stop();
 assert(p.select(50004)==Owner::Native);
}
