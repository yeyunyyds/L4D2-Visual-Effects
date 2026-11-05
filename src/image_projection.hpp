#pragma once
#include "image_scene.hpp"
namespace visual::pictures {
// Homogeneous clipping preserves the visible polygon when one corner crosses the camera/near plane.
struct ClipVertex {double x,y,z,w,u,v;};
inline std::vector<dango::wall::Vertex> clippedPanel(const dango::wall::Panel& panel,const dango::GameCameraFrame& camera,float width,float height){
 using namespace dango;CameraPose eye;std::vector<wall::Vertex> output;
 if(!camera.active||camera.epoch!=panel.epoch||width<=0||height<=0||!cameraPose(camera,eye)||wall::dot(panel.normal,eye.eye-panel.center)<=.001f)return output;
 std::vector<ClipVertex> polygon;const float uv[][2]={{0,0},{1,0},{1,1},{0,1}};const auto& m=camera.worldToClip;
 for(const auto& t:uv){auto p=wall::point(panel,t[0],t[1]);ClipVertex v{m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],m[8]*p.x+m[9]*p.y+m[10]*p.z+m[11],m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15],t[0],t[1]};
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.w))return output;
  polygon.push_back(v);
 }
 auto distance=[](const ClipVertex& v,unsigned plane){switch(plane){case 0:return v.w-.001;case 1:return v.z;case 2:return v.w-v.z;case 3:return v.w+v.x;case 4:return v.w-v.x;case 5:return v.w+v.y;default:return v.w-v.y;}};
 for(unsigned plane=0;plane<7&&!polygon.empty();++plane){std::vector<ClipVertex> next;auto previous=polygon.back();double dp=distance(previous,plane);
  for(const auto& current:polygon){double dc=distance(current,plane);bool a=dp>=0,b=dc>=0;
   if(a!=b){double t=dp/(dp-dc);next.push_back({previous.x+(current.x-previous.x)*t,previous.y+(current.y-previous.y)*t,previous.z+(current.z-previous.z)*t,previous.w+(current.w-previous.w)*t,previous.u+(current.u-previous.u)*t,previous.v+(current.v-previous.v)*t});}
   if(b)next.push_back(current);
   previous=current;dp=dc;
  }polygon=std::move(next);
 }
 if(polygon.size()<3||polygon.size()>16)return output;
 for(const auto& v:polygon)output.push_back({float((v.x/v.w+1)*width*.5),float((1-v.y/v.w)*height*.5),float(std::clamp(v.z/v.w,0.,1.)),float(1/v.w),float(v.u),float(v.v)});
 return output;
}
template<class Trace> bool pictureVisible(const dango::wall::Panel& panel,dango::Vec3 eye,Trace cast){
 using namespace dango;if(wall::dot(panel.normal,eye-panel.center)<=.001f)return false;
 const float samples[][2]={{.5f,.5f},{.05f,.05f},{.95f,.05f},{.05f,.95f},{.95f,.95f}};
 for(const auto& uv:samples){auto p=wall::point(panel,uv[0],uv[1]);trace::Result hit;
  if(!cast(eye,p,hit)||hit.startSolid||hit.allSolid||(hit.fraction<1&&length(hit.end-p)>2))return false;
 }return true;
}
} // namespace visual::pictures
