#include "first_person_mean_calibration.hpp"
#include <cassert>
int main(){
 using namespace visual::firstperson::meanbody;
 assert(stance(-1)==Stance::Unknown);
 assert(stance(1)==Stance::Standing&&stance(3)==Stance::Crouching);
 assert(stance(0)==Stance::Airborne&&stance(2)==Stance::Airborne);
 Mean m;for(unsigned i=0;i<30;++i)assert(m.add({float(i),2,3}));
 assert(m.ready()&&std::abs(m.value.x-14.5f)<.001f&&!m.add({20,20,20}));
 Mean bad;assert(!bad.add({97,0,0}));
 Calibration c;c.identify(1,1,"model",2,5,true);
 assert(!c.observe(Stance::Standing,false,{1,2,3},1000));
 assert(!c.observe(Stance::Standing,true,{1,2,3},1010));
 for(unsigned i=0;i<30;++i)assert(c.observe(Stance::Standing,true,{1,2,3},1160+50*i));
 assert(c.frozen(Stance::Standing)&&!c.frozen(Stance::Crouching));
 assert(!c.observe(Stance::Standing,true,{50,0,0},3000));
 c.identify(1,1,"model",2,5,false);assert(!c.frozen(Stance::Standing));
 const auto v=local({4,3,2},{0,1,0},{1,0,0});assert(v.x==3&&v.y==4&&v.z==2);
}
