#pragma once
#include "tank_sequence.hpp"
#include <map>
#include <cmath>
#include <vector>
namespace visual::tank {
struct LayerSample {
 int index=0,sequence=0;float cycle=0,weight=0;std::optional<SequenceName> names;
};
struct LayerAttack {
 bool onset=false,active=false,sampled=false;int layer=-1,sequence=-1;float cycle=0,weight=0;SequenceName names;
};
// Each Tank owns its own detector; map/serial isolation is handled by its owner.
class LayerAttackDetector {
 struct Track{int sequence=-1;float cycle=0,time=0;bool active=false,seen=false;SequenceName names;};
 std::map<int,Track> tracks;std::optional<float> lastTime;
public:
 void reset(){tracks.clear();lastTime.reset();}
 LayerAttack update(std::optional<float> now,bool sampled,const std::vector<LayerSample>& layers){
  LayerAttack out;if(!now||!std::isfinite(*now)||!sampled)return out;out.sampled=true;
  if(lastTime&&*now<*lastTime-.001f)reset();
  // Repeated samples at a paused simulation time cannot create new onsets.
  bool repeated=lastTime&&*now<=*lastTime;bool gap=lastTime&&*now-*lastTime>.25f;
  auto previous=tracks;if(gap)previous.clear();
  bool wasActive=false;for(const auto& p:previous)wasActive=wasActive||p.second.active;
  std::map<int,Track> next;LayerAttack candidate;bool candidateRestart=false;
  for(const auto& s:layers){
   if(s.index<0||s.index>=32||s.sequence<0||s.sequence>4095||!std::isfinite(s.cycle)||s.cycle<0||s.cycle>1||!std::isfinite(s.weight)||s.weight<0||s.weight>1)continue;
   auto p=previous.find(s.index);bool fresh=p!=previous.end()&&*now>=p->second.time&&*now-p->second.time<=.25f;
   bool oldActive=fresh&&p->second.active;
   auto names=s.names;
   if(!names&&fresh&&p->second.sequence==s.sequence&&!p->second.names.label.empty())names=p->second.names;
   // An unresolved positive-weight layer does not prove the previous attack ended.
   if(!names&&s.weight>.001f){if(fresh)next[s.index]=p->second;continue;}
   bool attacking=names&&classify(*names)=="attack-animation"&&s.weight>=(oldActive?.001f:.05f);
   bool wrapped=fresh&&p->second.cycle>.65f&&s.cycle<.25f;
   bool seen=fresh&&p->second.sequence==s.sequence&&p->second.seen&&!wrapped;
   Track t{s.sequence,s.cycle,*now,attacking,seen||attacking,names?*names:SequenceName{}};next[s.index]=t;
   bool restarted=oldActive&&(s.sequence!=p->second.sequence||wrapped);
   bool began=attacking&&(restarted||(!oldActive&&!seen));
   // Initial/missed history only permits an onset near the animation's beginning.
   if((!fresh||!oldActive)&&s.cycle>=.2f)began=false;
   if(attacking){
    if(!out.active||s.index<out.layer){out.active=true;out.layer=s.index;out.sequence=s.sequence;out.cycle=s.cycle;out.weight=s.weight;out.names=*names;}
    // A second layer blending into an ongoing attack is not another punch.
    if(began&&!repeated&&(restarted||!wasActive)&&(!candidate.onset||(restarted&&!candidateRestart)||((restarted==candidateRestart)&&s.index<candidate.layer))){
     candidate={true,true,true,s.index,s.sequence,s.cycle,s.weight,*names};candidateRestart=restarted;
    }
   }
  }
  if(!repeated){tracks=std::move(next);lastTime=now;}
  if(candidate.onset)return candidate;
  return out;
 }
};
}
