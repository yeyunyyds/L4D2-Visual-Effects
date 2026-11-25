#include "first_person_view_policy.hpp"
#include <cassert>
using namespace visual::firstperson::viewpolicy;
static Input at(unsigned tick,float ox,float oz,float x,float z,float eye,float head=20,int shoulder=0){
 Input in;in.tick=tick;in.originKnown=in.eyeKnown=in.headKnown=true;
 in.origin={ox,0,oz};in.camera={ox+x,0,oz+z};
 in.eyeDistance=eye;in.headDistance=head;in.shoulder=shoulder;in.fallbackFirst=true;return in;
}
int main(){
 Gate g;
 assert(!g.update(at(100,0,0,0,62,4,5)).full);
 assert(g.update(at(180,0,0,0,62,4,5)).full);
 // Moving and jumping together must not trigger the stationary branch.
 assert(g.update(at(260,15,18,5,63,18)).full);
 assert(g.update(at(340,30,36,0,62,17)).full);
 // A stationary character can crouch and stand up.
 assert(g.update(at(420,30,36,0,42,17)).full);
 assert(g.update(at(500,30,36,0,62,8)).full);
 // Detached camera is caught before the skeletal distance threshold.
 auto d=g.update(at(580,30,36,4.2f,62,9));
 assert(!d.full&&d.stationary&&d.detached);
 assert(!g.update(at(660,30,36,5,63,12)).full);
 assert(!g.update(at(740,30,36,2,62,7,8)).full);
 d=g.update(at(820,30,36,0,62,4,5));assert(d.full);
 // Pure vertical camera transition.
 d=g.update(at(900,30,36,0,67,12,18));assert(!d.full);
 assert(!g.update(at(980,30,36,0,65,8,11)).full);
 d=g.update(at(1060,30,36,0,63,5,8));assert(!d.full&&d.earlyHead);
 assert(g.update(at(1140,30,36,0,62,4,5)).full);
 // Explicit third person wins; returning camera can hide just the head.
 assert(!g.update(at(1220,30,36,45,80,50,50,1)).full);
 assert(!g.update(at(1300,30,36,0,63,5,8,1)).full);
 assert(g.update(at(1380,30,36,0,62,4,5,1)).earlyHead);
 // Old distance fallback remains.
 assert(!g.update(at(1460,30,36,0,62,28,20)).full);
 // Unknown evidence fails closed.
 Input unknown{};unknown.tick=1540;assert(!g.update(unknown).full);
}
