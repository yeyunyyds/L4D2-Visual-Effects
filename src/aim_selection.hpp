#pragma once
#include "dango/entity_follow.hpp"
#include "target_identity.hpp"
namespace visual {
struct AimSelection {int classId=-1;std::uint32_t handle{};std::string name;float score=0;};
// Adapt DangoMusic entity-follow candidates to the complete modeled-entity catalog.
// This is a crosshair-nearby heuristic, not a physics trace or occlusion query.
inline AimSelection identifyAim(const dango::inventory::Frame& frame,
                               const dango::GameCameraFrame& camera,std::uint64_t tick){
 AimSelection chosen;dango::CameraPose eye;
 if(!dango::usableCamera(camera,tick)||!dango::cameraPose(camera,eye)||
    frame.epoch!=camera.epoch||!frame.tick||tick<frame.tick||tick-frame.tick>250)return chosen;
 for(const auto& r:frame.records){
  if(!eligibleRecord(r)||r.classId<0||r.classId>4096)continue;
  auto pose=dango::follow::pose(frame,r.handle);if(!pose.ready)continue;
  auto point=dango::scene::point(pose.matrix,{}),ray=point-eye.eye;
  float distance=dango::length(ray);if(distance<48||distance>1200)continue;
  dango::Vec3 clip;
  if(!dango::transform(camera.worldToClip,point,clip,true)||std::abs(clip.x)>.98f||std::abs(clip.y)>.98f||clip.z<0||clip.z>1)continue;
  float cosine=(ray.x*eye.forward.x+ray.y*eye.forward.y+ray.z*eye.forward.z)/distance;
  if(cosine<.5f)continue;
  float score=(1-cosine)*1000+distance*.0001f;
  if(score>60.5f)continue;
  if(chosen.classId<0||score<chosen.score||(score==chosen.score&&r.handle<chosen.handle))
   chosen={r.classId,r.handle,r.className,score};
 }
 return chosen;
}
} // namespace visual
