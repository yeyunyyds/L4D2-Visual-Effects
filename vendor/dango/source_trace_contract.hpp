#pragma once
#include "wall_panel.hpp"
#include <cstddef>
#include <algorithm>
namespace dango::trace {
struct alignas(16) AlignedVector {float x=0,y=0,z=0,w=0;};
struct alignas(16) Ray {AlignedVector start,delta,offset,extents;uint32_t transform=0;uint8_t isRay=1,isSwept=0,padding[10]{};};
struct Plane {Vec3 normal;float distance=0;uint8_t type=0,signbits=0,padding[2]{};};
struct Surface {uint32_t name=0;int16_t properties=0;uint16_t flags=0;};
struct Result {Vec3 start,end;Plane plane;float fraction=1;int32_t contents=0;uint16_t displacementFlags=0;uint8_t allSolid=0,startSolid=0;float fractionLeftSolid=0;Surface surface;int32_t hitgroup=0;int16_t physicsBone=0,padding=0;uint32_t entity=0;int32_t hitbox=0;};
static_assert(sizeof(Ray)==80&&alignof(Ray)==16&&offsetof(Ray,isRay)==68&&offsetof(Ray,isSwept)==69,"Verified x86 Ray layout");
static_assert(sizeof(Result)==84&&offsetof(Result,plane)==24&&offsetof(Result,fraction)==44&&offsetof(Result,startSolid)==55&&offsetof(Result,surface)==60&&offsetof(Result,entity)==76,"Verified x86 trace result layout");
struct GuardedResult {std::array<uint8_t,16> before;Result result;std::array<uint8_t,64> after;GuardedResult(){before.fill(0xa5);after.fill(0xa5);}bool intact()const{return std::all_of(before.begin(),before.end(),[](uint8_t v){return v==0xa5;})&&std::all_of(after.begin(),after.end(),[](uint8_t v){return v==0xa5;});}};
inline Ray ray(Vec3 start,Vec3 end){Ray r;auto d=end-start;r.start={start.x,start.y,start.z,0};r.delta={d.x,d.y,d.z,0};r.isSwept=length(d)>0;return r;}
inline bool valid(const Result& r,Vec3 start,Vec3 end){
 if(!wall::finite(r.start)||!wall::finite(r.end)||!std::isfinite(r.fraction)||r.fraction<0||r.fraction>1||r.startSolid>1||r.allSolid>1)return false;
 if(r.startSolid||r.allSolid)return true;
 if(length(r.start-start)>2||length(r.end-(start+(end-start)*r.fraction))>2)return false;
 return r.fraction>=1||(wall::finite(r.plane.normal)&&std::isfinite(r.plane.distance)&&length(r.plane.normal)>.95f&&length(r.plane.normal)<1.05f&&std::abs(wall::dot(r.plane.normal,r.end)-r.plane.distance)<2);
}
// Axis-aligned experimental box centered on the tracked origin. This is not an entity hull read.
inline bool validExtents(Vec3 half){return wall::finite(half)&&half.x>=0&&half.y>=0&&half.z>=0&&std::max({half.x,half.y,half.z})<=64;}
inline Ray boxRay(Vec3 start,Vec3 end,Vec3 half){auto r=ray(start,end);r.extents={half.x,half.y,half.z,0};r.isRay=length(half)<1e-6f;return r;}
inline Vec3 boxSupport(Vec3 normal,Vec3 half){return {normal.x>0?half.x:normal.x<0?-half.x:0,normal.y>0?half.y:normal.y<0?-half.y:0,normal.z>0?half.z:normal.z<0?-half.z:0};}
inline bool validSweep(const Result& r,Vec3 start,Vec3 end,Vec3 half){
 if(!validExtents(half))return false;
 // Hull trace end is the box origin, not the point on the original surface.
 auto adjusted=r;auto support=boxSupport(r.plane.normal,half);adjusted.plane.distance+=wall::dot(r.plane.normal,support);return valid(adjusted,start,end);
}
inline bool wallHit(const Result& r){return r.fraction<1&&!r.startSolid&&!r.allSolid&&!(r.surface.flags&(0x2|0x4|0x80))&&!(r.contents&0x2);}
// World-only support checks are a finite sampling heuristic, not a proof that
// the whole rectangle lies inside one BSP face.
template<class Trace> bool supported(const wall::Panel& panel,Trace cast){
 for(int y=0;y<3;++y)for(int x=0;x<3;++x){auto p=wall::point(panel,float(x)/2,float(y)/2)-panel.normal*.75f;Result r;
 if(!cast(p+panel.normal*8,p-panel.normal*8,r)||!wallHit(r)||wall::dot(r.plane.normal,panel.normal)<.98f||length(r.end-p)>2)return false;}return true;
}
template<class Trace> bool visible(const wall::Panel& panel,Vec3 eye,Trace cast){
 if(wall::dot(panel.normal,eye-panel.center)<=.5f)return false;
 const float samples[][2]={{.5f,.5f},{.05f,.05f},{.95f,.05f},{.05f,.95f},{.95f,.95f}};
 for(auto& uv:samples){auto p=wall::point(panel,uv[0],uv[1]);Result r;if(!cast(eye,p,r)||r.startSolid||r.allSolid||(r.fraction<1&&length(r.end-p)>2))return false;}return true;
}
}
