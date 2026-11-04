#include "animation.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("Animation settings regression");}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
int main(){using namespace visual;
 AnimationSettings a;check(a.periodMs==500&&a.minTenths==5&&a.maxTenths==11&&a.preset==0);
 check(near(animationScale(0,a),.5f)&&near(animationScale(250,a),1.1f)&&near(animationScale(500,a),.5f));
 for(unsigned preset=0;preset<3;++preset)for(unsigned period:{100u,500u,2300u,10000u}){
  a={period,5,11,preset};
  for(unsigned t=0;t<=period;++t){float s=animationScale(t,a);check(s>=.5f&&s<=1.10001f);check(near(s,animationScale(t+period,a)));}
  check(near(animationScale(0,a),animationScale(period,a)));
  check(near(animationScale(preset==0?period/2:preset==1?period*3/10:period/5,a),1.1f));
 }
 a={1000,5,11,1};check(animationScale(650,a)>animationScale(475,a)); // secondary rebound
 a=AnimationSettings{};
 a=adjustAnimation(a,111);check(a.periodMs==600);a=adjustAnimation(a,110);check(a.periodMs==500);
 a=adjustAnimation(a,112);check(a.minTenths==4);a=adjustAnimation(a,113);check(a.minTenths==5);
 a=adjustAnimation(a,115);check(a.maxTenths==12);a=adjustAnimation(a,114);check(a.maxTenths==11);
 for(int i=0;i<1000;++i){a=adjustAnimation(a,113);}check(a.minTenths==a.maxTenths-1);
 for(int i=0;i<1000;++i){a=adjustAnimation(a,114);}check(a.maxTenths==a.minTenths+1);
 for(int i=0;i<1000;++i){a=adjustAnimation(a,110);}check(a.periodMs==100);
 for(int i=0;i<1000;++i){a=adjustAnimation(a,111);}check(a.periodMs==10000);
 a=normalized({std::numeric_limits<unsigned>::max(),-100,100,100});check(a.periodMs==10000&&a.minTenths==1&&a.maxTenths==30&&a.preset==0);
 a=adjustAnimation(a,117);check(a.periodMs==500&&a.minTenths==5&&a.maxTenths==11);
 auto original=a;for(int i=0;i<3;++i)a=adjustAnimation(a,116);check(a.preset==original.preset&&a.periodMs==original.periodMs&&a.minTenths==original.minTenths&&a.maxTenths==original.maxTenths);
 std::cout<<"PASS: defaults, all presets bounded/periodic, Q rebound, exact 0.1 increments, bound crossing, reset, malformed settings\n";
}
