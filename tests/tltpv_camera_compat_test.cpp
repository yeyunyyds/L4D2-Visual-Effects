#include "tltpv_camera_compat.hpp"
#include <cassert>
#include <limits>
int main(){
 using namespace visual::tltpv;
 using dango::Vec3;dango::CameraPose p{};
 p.eye={5,3,64};
 assert(classify(p,Vec3{0,0,0},false)==Mode::First);
 assert(classify(p,Vec3{0,0,0},true)==Mode::First); // stale shoulder CVar
 p.eye={-50,0,64};
 assert(classify(p,Vec3{0,0,0},true)==Mode::Third);
 assert(classify(p,Vec3{0,0,0},false)==Mode::Third); // stale CVar
 p.eye={25,0,64};
 assert(classify(p,Vec3{0,0,0},std::nullopt)==Mode::Unknown);
 assert(classify(p,Vec3{0,0,0},false)==Mode::First);
 assert(classify(p,Vec3{0,0,0},true)==Mode::Unknown);
 p.eye={34,0,64};
 assert(classify(p,Vec3{0,0,0},true)==Mode::Third);
 assert(classify(p,Vec3{0,0,0},false)==Mode::Unknown);
 assert(classify(p,std::nullopt,true)==Mode::Unknown);
 p.eye={0,0,std::numeric_limits<float>::quiet_NaN()};
 assert(classify(p,Vec3{0,0,0},true)==Mode::Unknown);
 p.eye={0,0,250};
 assert(classify(p,Vec3{0,0,0},false)==Mode::Unknown);
}
