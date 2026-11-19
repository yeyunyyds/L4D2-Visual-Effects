#include "first_person_body_math.hpp"
#include <cassert>
#include <cmath>
int main(){
 using namespace visual::firstperson::math;
 using dango::Matrix4;
 Matrix4 identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
 std::array<float,4> plane{};
 assert(clipPlane(identity,65,true,plane));
 assert(plane[2]<0&&plane[3]>0);
 assert(clipPlane(identity,65,false,plane));
 assert(plane[2]==-1&&plane[3]==65);
 Matrix4 scale{2,0,0,0,0,3,0,0,0,0,4,0,0,0,0,1};
 assert(clipPlane(scale,15,true,plane));
 float at40=plane[2]*40+plane[3],at80=plane[2]*80+plane[3];
 assert(at40>0&&at80<0);
 Matrix4 invalid{};assert(!clipPlane(invalid,65,true,plane));
 assert(!clipPlane(identity,NAN,true,plane));
}
