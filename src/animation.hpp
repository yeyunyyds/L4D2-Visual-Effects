#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace visual {
struct AnimationSettings {
 unsigned periodMs=500;
 int minTenths=5,maxTenths=11;
 unsigned preset=0;
};
inline AnimationSettings normalized(AnimationSettings a) {
 a.periodMs=std::clamp(a.periodMs,100u,10000u);
 a.periodMs=((a.periodMs+50)/100)*100;
 a.minTenths=std::clamp(a.minTenths,1,29);
 a.maxTenths=std::clamp(a.maxTenths,a.minTenths+1,30);
 if(a.preset>2)a.preset=0;
 return a;
}
inline const char* presetName(unsigned p){return p==1?u8"Q弹回弹":p==2?u8"双跳脉冲":u8"平滑往复";}
inline double smoothStep(double x){return x*x*(3-2*x);}
inline double animationWeight(double phase,unsigned preset) {
 constexpr double pi=3.14159265358979323846;
 if(preset==1){
  if(phase<.3)return smoothStep(phase/.3);
  double u=(phase-.3)/.7;
  return (1-smoothStep(u))*(.3+.7*(1+std::cos(4*pi*u))*.5);
 }
 if(preset==2){
  if(phase<.4){double v=std::sin(pi*phase/.4);return v*v;}
  if(phase>=.5&&phase<.8){double v=std::sin(pi*(phase-.5)/.3);return .55*v*v;}
  return 0;
 }
 return (1-std::cos(2*pi*phase))*.5;
}
inline float animationScale(std::uint64_t elapsed,const AnimationSettings& settings){
 auto a=normalized(settings);
 double p=double(elapsed%a.periodMs)/a.periodMs;
 return float((a.minTenths+(a.maxTenths-a.minTenths)*std::clamp(animationWeight(p,a.preset),0.,1.))/10.);
}
inline AnimationSettings adjustAnimation(AnimationSettings a,unsigned action){
 a=normalized(a);
 switch(action){
 case 110:a.periodMs=std::max(100u,a.periodMs-100);break;
 case 111:a.periodMs=std::min(10000u,a.periodMs+100);break;
 case 112:a.minTenths=std::max(1,a.minTenths-1);break;
 case 113:a.minTenths=std::min(a.maxTenths-1,a.minTenths+1);break;
 case 114:a.maxTenths=std::max(a.minTenths+1,a.maxTenths-1);break;
 case 115:a.maxTenths=std::min(30,a.maxTenths+1);break;
 case 116:a.preset=(a.preset+1)%3;break;
 case 117:a=AnimationSettings{};break;
 }
 return a;
}
}
