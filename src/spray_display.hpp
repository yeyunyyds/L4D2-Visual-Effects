#pragma once
#include <atomic>
namespace visual::pictures {
inline std::atomic<bool> displayPaused{false};
struct DisplayGate {
 bool escapeHeld=false;
 void poll(bool escape,bool pluginMenu,bool foreground,bool active){
  if(!active){displayPaused=false;escapeHeld=escape;return;}
  if(foreground&&escape&&!escapeHeld&&!pluginMenu)displayPaused=true;
  escapeHeld=escape;
 }
};
}
