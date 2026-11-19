#pragma once
#include "dango/world_camera.hpp"
namespace visual {
struct OverlayGate {
 bool paused=false,escapeHeld=false,wasEnabled=false;std::uint32_t epoch=0;std::uint64_t tick=0;dango::Matrix4 view{};
 void poll(const dango::GameCameraFrame& camera,bool escape,bool pluginMenu,bool foreground,bool enabled,bool reset=false){
  if(!enabled||!camera.active||camera.epoch!=epoch||!wasEnabled||reset)paused=false;
  if(enabled&&camera.active&&foreground&&escape&&!escapeHeld&&!pluginMenu)paused=true;
  // No OS-cursor guesses: DXVK relative input can report the wrong cursor state.
  // A new in-game view rotation after Escape is our bounded recovery signal.
  if(paused&&foreground&&!escape&&!pluginMenu&&camera.active&&camera.epoch==epoch&&camera.tick>tick){float change=0;for(unsigned i:{0u,1u,2u,4u,5u,6u,8u,9u,10u})change=std::max(change,std::abs(camera.worldToView[i]-view[i]));if(change>.0001f)paused=false;}
  escapeHeld=escape;wasEnabled=enabled&&camera.active;epoch=camera.epoch;tick=camera.tick;view=camera.worldToView;
 }
};
}
