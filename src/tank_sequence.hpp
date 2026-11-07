#pragma once
#include <optional>
#include <string>
namespace visual::tank {
struct SequenceName{std::string label,activity;int activityId=-1,flags=0;};
inline std::string lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=char(c+32);return s;}
inline std::string classify(SequenceName name){auto activity=lower(name.activity),label=lower(name.label);
 // Semantic activity beats cosmetic sequence names, including misleading Mod labels.
 if(!activity.empty()&&activity!="act_invalid"&&activity!="act_reset"){
  if(activity.find("throw")!=std::string::npos||activity.find("range_attack")!=std::string::npos)return "throw-animation";
  if(activity.find("melee_attack")!=std::string::npos||activity.rfind("act_terror_attack",0)==0||activity.rfind("act_tank_attack",0)==0||activity.rfind("act_hulk_attack",0)==0)return "attack-animation";
  if(activity.find("run")!=std::string::npos||activity.find("walk")!=std::string::npos)return "movement-animation";
  return "other-animation";
 }
 if(label.find("throw")!=std::string::npos)return "throw-animation";
 if(label.find("idle")==std::string::npos&&(label.find("punch")!=std::string::npos||label.find("melee")!=std::string::npos||label.find("attack")!=std::string::npos))return "attack-animation";
 if(label.find("run")!=std::string::npos||label.find("walk")!=std::string::npos)return "movement-animation";
 return "other-animation";
}
}
