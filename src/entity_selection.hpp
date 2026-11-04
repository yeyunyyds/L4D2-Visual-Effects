#pragma once
#include "entity_categories.hpp"
#include "survivors.hpp"
#include "special_infected.hpp"
#include <charconv>
#include <map>
#include <string>
namespace visual {
struct EntitySelection {
 bool all=false;
 std::map<int,bool> classes;
 std::map<int,bool> survivors,specials;
 std::map<std::string,bool> types;
 std::map<unsigned,bool> groups;
 std::map<int,std::string> names;
 bool selected(int classId,std::string_view name={})const {
  if(classId<0 || classId>4096)return false;
  if(name.empty()){auto label=names.find(classId);if(label!=names.end())name=label->second;}
  int special=specialIndex(name);if(special>=0){auto flag=specials.find(special);if(flag!=specials.end())return flag->second;}
  auto c=classes.find(classId);if(c!=classes.end())return c->second;
  return selectedType(name);
 }
 bool selectedSpecial(int role)const {
  if(role<0||role>=int(SpecialCount))return false;
  auto flag=specials.find(role);if(flag!=specials.end())return flag->second;
  for(const auto& name:names)if(name.second==SpecialNames[role]){auto c=classes.find(name.first);if(c!=classes.end())return c->second;}
  return selectedType(SpecialNames[role]);
 }
 bool selectedSurvivor(int role)const {
  if(role<0||role>=int(SurvivorCount))return false;
  auto flag=survivors.find(role);if(flag!=survivors.end())return flag->second;
  // Old class/type settings remain the fallback until a role is explicitly set.
  auto inherited=[&](const char* type){
   for(const auto& name:names)if(name.second==type){auto c=classes.find(name.first);if(c!=classes.end())return c->second;}
   return selectedType(type);
  };
  return inherited("CTerrorPlayer")||inherited("SurvivorBot");
 }
 template<class Record> bool selectedRecord(const Record& r)const {
  int special=specialCharacter(r);if(special>=0)return selectedSpecial(special);
  if(survivorPlayerClass(r.className)&&(!r.team||*r.team==3))return false;
  if(survivorRecord(r)){
   int role=survivorCharacter(r);if(role<0)return false;
   return selectedSurvivor(role);
  }
  return selected(r.classId,r.className);
 }
 bool selectedType(std::string_view name)const {
  auto type=types.find(std::string(name));if(type!=types.end())return type->second;
  auto group=groups.find(unsigned(groupForName(name)));return group==groups.end()?all:group->second;
 }
 void setType(const std::string& name,bool enabled){
  if(!validTypeName(name))return;
  types[name]=enabled;
  for(auto c=classes.begin();c!=classes.end();){auto label=names.find(c->first);
   if(label!=names.end()&&label->second==name)c=classes.erase(c);else ++c;
  }
 }
 void setClass(int id,const std::string& name,bool enabled){classes[id]=enabled;names[id]=name;types.erase(name);}
 void setAll(bool value){all=value;classes.clear();types.clear();groups.clear();survivors.clear();specials.clear();}
 void setGroup(EntityGroup group,bool value){
  if(unsigned(group)>=EntityGroupCount)return;
  if(group==EntityGroup::Special){for(unsigned role=0;role<SpecialCount;++role)specials[int(role)]=value;return;}
  if(group==EntityGroup::Survivors){for(unsigned role=0;role<SurvivorCount;++role)survivors[int(role)]=value;return;}
  groups[unsigned(group)]=value;
  for(auto t=types.begin();t!=types.end();){if(groupForName(t->first)==group)t=types.erase(t);else ++t;}
  for(auto i=classes.begin();i!=classes.end();){
   auto label=names.find(i->first);auto current=groupForName(label==names.end()?std::string_view{}:std::string_view(label->second));
   if(current==group)i=classes.erase(i);else ++i;
  }
 }
 bool anyEnabled()const {
  if(all)return true;
  for(const auto& s:survivors)if(s.second)return true;
  for(const auto& s:specials)if(s.second)return true;
  for(const auto& t:types)if(t.second)return true;
  for(const auto& g:groups)if(g.second)return true;
  for(const auto& c:classes)if(c.second)return true;
  return false;
 }
};
inline bool parseClassId(const std::string& key,int& value){
 auto result=std::from_chars(key.data(),key.data()+key.size(),value);
 return result.ec==std::errc{} && result.ptr==key.data()+key.size() && value>=0 && value<=4096;
}
template<class Json> EntitySelection readEntitySelection(const Json& j){
 EntitySelection result;auto i=j.find("entitySelection");if(i==j.end()||!i->is_object())return result;
 auto all=i->find("all");if(all!=i->end()&&all->is_boolean())result.all=all->template get<bool>();
 auto classes=i->find("classes");if(classes!=i->end()&&classes->is_object()){
  for(auto c=classes->begin();c!=classes->end()&&result.classes.size()<4097;++c){
   int id=-1;if(c.value().is_boolean()&&parseClassId(c.key(),id))result.classes[id]=c.value().template get<bool>();
  }
 }
 auto specials=i->find("specials");if(specials!=i->end()&&specials->is_object()){
  for(auto r=specials->begin();r!=specials->end();++r){int role=specialIndex(r.key());
   if(role>=0&&r.value().is_boolean())result.specials[role]=r.value().template get<bool>();
  }
 }
 auto survivors=i->find("survivors");if(survivors!=i->end()&&survivors->is_object()){
  for(auto r=survivors->begin();r!=survivors->end();++r){int role=survivorIndex(r.key());
   if(role>=0&&r.value().is_boolean())result.survivors[role]=r.value().template get<bool>();
  }
 }
 auto types=i->find("types");if(types!=i->end()&&types->is_object()){
  for(auto t=types->begin();t!=types->end()&&result.types.size()<4096;++t)
   if(validTypeName(t.key())&&t.value().is_boolean())result.types[t.key()]=t.value().template get<bool>();
 }
 auto groups=i->find("groups");if(groups!=i->end()&&groups->is_object()){
  for(auto g=groups->begin();g!=groups->end();++g){int id=-1;if(parseClassId(g.key(),id)&&unsigned(id)<EntityGroupCount&&g.value().is_boolean())result.groups[unsigned(id)]=g.value().template get<bool>();}
 }
 auto names=i->find("names");if(names!=i->end()&&names->is_object()){
  for(auto c=names->begin();c!=names->end()&&result.names.size()<4097;++c){
   int id=-1;if(c.value().is_string()&&parseClassId(c.key(),id)){
    auto label=c.value().template get<std::string>();if(!label.empty()&&label.size()<=127)result.names[id]=std::move(label);
   }
  }
 }
 return result;
}
template<class Json> void writeEntitySelection(Json& j,const EntitySelection& s){
 Json classes=Json::object(),names=Json::object(),groups=Json::object(),types=Json::object(),survivors=Json::object(),specials=Json::object();
 for(const auto& c:s.classes)classes[std::to_string(c.first)]=c.second;
 for(const auto& c:s.names)names[std::to_string(c.first)]=c.second;
 for(const auto& g:s.groups)groups[std::to_string(g.first)]=g.second;
 for(const auto& t:s.types)types[t.first]=t.second;
 for(const auto& r:s.survivors)if(r.first>=0&&r.first<int(SurvivorCount))survivors[SurvivorNames[r.first]]=r.second;
 for(const auto& r:s.specials)if(r.first>=0&&r.first<int(SpecialCount))specials[SpecialNames[r.first]]=r.second;
 j["entitySelection"]={{"specials",specials},{"survivors",survivors},{"all",s.all},{"classes",classes},{"names",names},{"groups",groups},{"types",types}};
}
} // namespace visual
