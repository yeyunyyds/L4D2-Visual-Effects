#pragma once
#include "dango/entity_inventory.hpp"
#include "dango/source_trace_contract.hpp"
#include <array>
#include <optional>
#include <algorithm>
#include <cmath>
namespace visual::occlusion {
// Three points inside the current entity's vertical hull. World-only traces
// are a conservative approximation, not a pixel/depth-buffer visibility test.
template<class Record> std::array<dango::Vec3,3> points(const Record& r,bool tank=false){
 auto center=*r.networkOrigin;
 float low=tank?14.f:12.f,high=tank?105.f:70.f;
 if(r.collisionMins&&r.collisionMaxs&&
    std::isfinite(r.collisionMins->z)&&std::isfinite(r.collisionMaxs->z)&&
    r.collisionMins->z>=-160&&r.collisionMaxs->z<=256&&
    r.collisionMaxs->z-r.collisionMins->z>=8){
  low=r.collisionMins->z;high=r.collisionMaxs->z;
 }
 float span=high-low;
 return {center+dango::Vec3{0,0,low+span*.83f},
         center+dango::Vec3{0,0,low+span*.50f},
         center+dango::Vec3{0,0,low+span*.18f}};
}
template<class Record,class Cast>
std::optional<bool> bodyVisible(dango::Vec3 eye,const Record& r,Cast cast,bool tank=false){
 if(!r.networkOrigin||!std::isfinite(eye.x)||!std::isfinite(eye.y)||!std::isfinite(eye.z))return {};
 bool failed=false;
 for(auto p:points(r,tank)){
  dango::trace::Result result{};
  if(!cast(eye,p,result)){failed=true;continue;}
  if(!result.startSolid&&!result.allSolid&&
     (result.fraction>=.999f||dango::length(result.end-p)<2.f))return true;
 }
 if(failed)return {};
 return false;
}
} // namespace visual::occlusion
