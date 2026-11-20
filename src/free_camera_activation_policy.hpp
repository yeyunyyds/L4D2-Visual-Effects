#pragma once
namespace visual::freecamera::activation {
enum class Phase {Off,Binding,Starting,Active,Paused};
inline Phase phase(bool requested,bool bound,bool active,bool paused) noexcept {
 if(!requested)return Phase::Off;
 if(!bound)return Phase::Binding;
 if(paused)return Phase::Paused;
 return active?Phase::Active:Phase::Starting;
}
inline const char* label(Phase p) noexcept {
 switch(p){
 case Phase::Off:return "off";
 case Phase::Binding:return "binding";
 case Phase::Starting:return "starting";
 case Phase::Active:return "active";
 case Phase::Paused:return "paused";
 }
 return "invalid";
}
inline bool shouldBind(bool requested,bool bound) noexcept {return requested&&!bound;}
inline bool shouldConsumeInput(bool requested,bool active) noexcept {return requested&&active;}
}
