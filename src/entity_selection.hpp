#pragma once
#include "entity_categories.hpp"
#include <charconv>
#include <map>
#include <string>
namespace visual {
struct EntitySelection {
 bool all=false;
 std::map<int,bool> classes;
 std::map<unsigned,bool> groups;
 std::map<int,std::string> names;
 bool selected(int classId,std::string_view name={})const {
  if(classId<0 || classId>4096)return false;
  auto c=classes.find(classId);if(c!=classes.end())return c->second;
  if(name.empty()){auto label=names.find(classId);if(label!=names.end())name=label->second;}
  auto group=groups.find(unsigned(groupForName(name)));return group==groups.end()?all:group->second;
 }
 void setAll(bool value){all=value;classes.clear();groups.clear();}
 void setGroup(EntityGroup group,bool value){
  if(unsigned(group)>=EntityGroupCount)return;
  groups[unsigned(group)]=value;
  for(auto i=classes.begin();i!=classes.end();){
   auto label=names.find(i->first);auto current=groupForName(label==names.end()?std::string_view{}:std::string_view(label->second));
   if(current==group)i=classes.erase(i);else ++i;
  }
 }
 bool anyEnabled()const {
  if(all)return true;
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
 Json classes=Json::object(),names=Json::object(),groups=Json::object();
 for(const auto& c:s.classes)classes[std::to_string(c.first)]=c.second;
 for(const auto& c:s.names)names[std::to_string(c.first)]=c.second;
 for(const auto& g:s.groups)groups[std::to_string(g.first)]=g.second;
 j["entitySelection"]={{"all",s.all},{"classes",classes},{"names",names},{"groups",groups}};
}
} // namespace visual
