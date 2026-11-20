#include "first_person_body_math.hpp"
#include <cassert>
#include <cmath>
#include <limits>
int main(){
 using namespace visual::firstperson::math;
 using dango::Vec3;
 assert(finite(Vec3{1,2,3}));
 assert(!finite(Vec3{0,std::numeric_limits<float>::quiet_NaN(),0}));
 dango::GameCameraFrame bad{};
 Vec3 shifted{};
 assert(!offset(bad,1,2,3,shifted));
}
