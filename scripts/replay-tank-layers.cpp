// Optional offline replay of user-provided diagnostics; logs are never bundled.
#include "tank_layer_attack.hpp"
#include "tank_observed_timer.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char** argv){using namespace visual::tank;using nlohmann::json;
 std::map<uint32_t,LayerAttackDetector> detectors;std::map<uint32_t,SwingStopwatch> timers;uint32_t epoch=0;uint64_t lastTick=0;unsigned rows=0,onsets=0,starts=0,positive=0;std::map<std::string,unsigned> names;
 for(int file=1;file<argc;++file){std::ifstream in(argv[file]);if(!in){std::cerr<<"Cannot open replay input\n";return 1;}std::string line;
 while(std::getline(in,line)){auto j=json::parse(line);if(j.value("schema",0)<6)continue;auto tick=j.value("tick",uint64_t{});if(lastTick&&tick<=lastTick){std::cerr<<"Replay inputs must be in chronological order\n";return 1;}lastTick=tick;
  auto e=j.value("epoch",0u);if(e!=epoch){detectors.clear();timers.clear();epoch=e;}++rows;
  auto optional=[&](const char* name)->std::optional<float>{return j.contains(name)?std::optional<float>(j[name].get<float>()):std::nullopt;};
  for(const auto& t:j["tanks"]){auto h=t["handle"].get<uint32_t>();std::vector<LayerSample> layers;bool sampled=false;
   for(const auto& a:j["actors"])if(a["handle"]==h){sampled=a.value("animationLayerReason","")=="sampled read-only; network values, not an event callback";for(const auto& l:a.value("animationLayers",json::array())){LayerSample s;s.index=l["index"];s.sequence=l["sequence"];s.cycle=l["cycle"];s.weight=l["weight"];if(l.contains("label"))s.names=SequenceName{l.value("label",""),l.value("activity","")};layers.push_back(s);}break;}
   auto layer=detectors[h].update(optional("observedGameTime"),sampled,layers);if(layer.onset){++onsets;++names[layer.names.label];}
   std::optional<SequenceName> body;if(t.contains("animationProbe")){auto a=t["animationProbe"];if(a.value("sequence",-2)==t.value("sequence",-1)&&!a.value("label","").empty())body=SequenceName{a.value("label",""),a.value("activity","")};}
   std::optional<float> cycle;if(t.contains("cycle"))cycle=t["cycle"].get<float>();
   auto r=timers[h].update(optional("observedGameTime"),tick,t.value("sequence",-1),cycle,body,optional("interval"),optional("missInterval"),false,layer.onset,optional("windup"));if(r.triggered)++starts;if(r.remaining&&*r.remaining>0)++positive;
   // Duplicate sampling must not restart either layer detection or the timer.
   auto duplicate=detectors[h].update(optional("observedGameTime"),sampled,layers);auto again=timers[h].update(optional("observedGameTime"),tick,t.value("sequence",-1),cycle,body,optional("interval"),optional("missInterval"),false,duplicate.onset,optional("windup"));
   if(duplicate.onset||again.triggered){std::cerr<<"Duplicate replay sample restarted a swing\n";return 2;}
  }
 }}
 std::cout<<"rows="<<rows<<" layerOnsets="<<onsets<<" stopwatchStarts="<<starts<<" positiveCountdownSamples="<<positive<<" duplicateRestarts=0\n";for(const auto& p:names)std::cout<<p.first<<"="<<p.second<<'\n';
 return rows&&starts==onsets?0:3;
}
