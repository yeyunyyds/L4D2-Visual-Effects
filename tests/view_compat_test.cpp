#include "view_compat.hpp"
#include <cassert>
#include <limits>
int main(){
 using namespace visual::viewcompat;
 using dango::Vec3;dango::CameraPose p{};
 p.eye={5,3,64};
 assert(classify(p,Vec3{0,0,0},false).evidence==Mode::First);
 assert(classify(p,Vec3{0,0,0},true).evidence==Mode::Third);
 p.eye={-50,0,64};
 assert(classify(p,Vec3{0,0,0},true).evidence==Mode::Third);
 assert(classify(p,Vec3{0,0,0},false).evidence==Mode::First);
 p.eye={0,0,150}; // top-down shoulder camera: XY zero is NOT first-person
 assert(classify(p,Vec3{0,0,0},true).evidence==Mode::Third);
 assert(classify(p,Vec3{0,0,0},false).evidence==Mode::Third);
 assert(classify(p,Vec3{0,0,0},std::nullopt).evidence==Mode::Third);
 p.eye={0,0,110};
 assert(classify(p,Vec3{0,0,0},false).evidence==Mode::Unknown);
 p.eye={25,0,64};
 assert(classify(p,Vec3{0,0,0},std::nullopt).evidence==Mode::Unknown);
 assert(classify(p,Vec3{0,0,0},false).evidence==Mode::First);
 assert(classify(p,std::nullopt,true).evidence==Mode::Unknown);
 p.eye={0,0,std::numeric_limits<float>::quiet_NaN()};
 assert(classify(p,Vec3{0,0,0},true).evidence==Mode::Unknown);
 // Moving camera: stale coordinate samples must not toggle the native gate.
 CameraLatch latch;
 assert(latch.update(Mode::Third,1000,4)==Mode::Third);
 assert(latch.update(Mode::Unknown,1020,4)==Mode::Third);
 assert(latch.update(Mode::Unknown,1200,4)==Mode::Third);
 assert(latch.update(Mode::Third,1230,4)==Mode::Third);
 assert(latch.update(Mode::First,1240,4)==Mode::Third);
 assert(latch.update(Mode::Unknown,1255,4)==Mode::Third);
 assert(latch.update(Mode::First,1270,4)==Mode::Third);
 assert(latch.update(Mode::First,1370,4)==Mode::First);
 assert(latch.update(Mode::First,1380,4)==Mode::First);
 assert(latch.update(Mode::Third,1390,4)==Mode::Third);
 assert(latch.update(Mode::Unknown,1400,4)==Mode::Third);
 assert(latch.update(Mode::Unknown,1950,4)==Mode::Unknown);
 assert(latch.update(Mode::First,1960,4)==Mode::Unknown);
 assert(latch.update(Mode::First,2060,4)==Mode::First);
 assert(latch.update(Mode::First,2080,5)==Mode::Unknown); // map changed
}
