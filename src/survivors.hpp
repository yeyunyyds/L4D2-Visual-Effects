#pragma once
#include <string_view>
namespace visual {
// DangoMusic experiment/npc-dialogue: m_survivorCharacter ordering.
inline constexpr const char* SurvivorNames[]={"Nick","Rochelle","Coach","Ellis","Bill","Zoey","Francis","Louis"};
inline constexpr unsigned SurvivorCount=8;
inline int survivorIndex(std::string_view name){
 for(unsigned i=0;i<SurvivorCount;++i)if(name==SurvivorNames[i])return int(i);
 return -1;
}
inline bool survivorPlayerClass(std::string_view name){
 return name=="CTerrorPlayer"||name=="SurvivorBot"||name=="NextBotCombatCharacter"||name=="CCSPlayer"||name=="CBasePlayer";
}
template<class Record> bool survivorRecord(const Record& r){return survivorPlayerClass(r.className)&&r.team&&*r.team==2;}
template<class Record> int survivorCharacter(const Record& r){
 if(!survivorRecord(r)||!r.character||*r.character<0||*r.character>=int(SurvivorCount))return -1;
 return *r.character;
}
} // namespace visual
