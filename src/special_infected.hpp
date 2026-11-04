#pragma once
#include "survivors.hpp"
namespace visual {
inline constexpr const char* SpecialNames[]={"Smoker","Boomer","Hunter","Spitter","Jockey","Charger","Tank","Witch"};
inline constexpr unsigned SpecialCount=8;
inline int specialIndex(std::string_view name){
 for(unsigned i=0;i<SpecialCount;++i)if(name==SpecialNames[i])return int(i);
 return -1;
}
template<class Record> int specialCharacter(const Record& r){
 int named=specialIndex(r.className);if(named>=0)return named;
 if(!survivorPlayerClass(r.className)||!r.team||*r.team!=3||!r.zombieClass)return -1;
 // L4D2 m_zombieClass: 1..6 regular specials, 7 Witch, 8 Tank (not Class ID).
 int value=*r.zombieClass;
 if(value>=1&&value<=6)return value-1;
 return value==8?6:value==7?7:-1;
}
} // namespace visual
