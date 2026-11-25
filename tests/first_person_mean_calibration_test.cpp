#include "first_person_mean_calibration.hpp"
#include <cassert>
int main(){
 using namespace visual::firstperson::meanbody;
 using visual::firstperson::dynamic::transform;
 assert(stance(-1)==Stance::Unknown);
 assert(stance(1)==Stance::Standing&&stance(3)==Stance::Crouching);
 assert(stance(0)==Stance::Airborne&&stance(2)==Stance::Airborne);
 Mean m;for(unsigned i=0;i<30;++i)assert(m.add({{float(i),2,3},.25f}));
 assert(m.ready()&&std::abs(m.value.x-14.5f)<.001f&&m.yaw==.25f&&!m.add({{20,20,20},0}));
 Mean bad;assert(!bad.add({{97,0,0},0}));
 Calibration c;c.identify(1,1,"model",2,{5,-13,-1,-1,0,1});
 assert(!c.observe(Stance::Standing,false,{{1,2,3},0},1000));
 assert(!c.observe(Stance::Standing,true,{{1,2,3},0},1010));
 for(unsigned i=0;i<30;++i)assert(c.observe(Stance::Standing,true,{{1,2,3},0},1160+50*i));
 assert(c.frozen(Stance::Standing)&&!c.frozen(Stance::Crouching));
 assert(!c.observe(Stance::Standing,true,{{50,0,0},0},3000));
 c.identify(1,1,"model",2,{5,-12,-1,-1,0,1});assert(!c.frozen(Stance::Standing));
 // Reproduce a single teacher transform with identical original matrices.
 const dango::Vec3 f{.6f,.8f,0},r{.8f,-.6f,0},shift{-13,5,-3},pivot{4,9,12};
 auto sample=capture(shift,.3f,f,r);auto restored=replay(sample,f,r);
 assert(dango::length(restored-shift)<.0001f);
 visual::Matrix3x4 a[1]{},b[1]{};
 for(int i=0;i<3;++i){a[0].m[i][i]=1;a[0].m[i][3]=float(i*5);}
 b[0]=a[0];
 assert(transform(a,1,pivot,shift,.3f)&&transform(b,1,pivot,restored,sample.yaw));
 for(int i=0;i<3;++i)for(int j=0;j<4;++j)assert(std::abs(a[0].m[i][j]-b[0].m[i][j])<.0001f);
 DistanceGate g;
 assert(!g.update(20,true,0,false,100));
 assert(g.update(20,true,0,false,180));
 assert(g.update(26,true,0,false,200));
 assert(!g.update(28,true,0,true,201));
 assert(!g.update(25,true,0,true,220));
 assert(!g.update(20,true,0,true,240));
 assert(g.update(20,true,0,true,320));
 assert(!g.update(3,true,1,true,321));
 assert(g.update(0,false,0,true,322));
 assert(!g.update(0,false,0,false,323));
}
