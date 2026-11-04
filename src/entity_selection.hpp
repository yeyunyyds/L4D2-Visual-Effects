#pragma once
#include <charconv>
#include <map>
#include <string>
namespace visual {
struct EntitySelection {
 bool all=false;
 std::map<int,bool> classes;
 std::map<int,std::string> names;
 bool selected(int classId)const {
  if(classId<0 || classId>4096)return false;
  auto c=classes.find(classId);return c==classes.end()?all:c->second;
 }
 void setAll(bool value){all=value;classes.clear();}
 bool anyEnabled()const {
  if(all)return true;
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
 Json classes=Json::object(),names=Json::object();
 for(const auto& c:s.classes)classes[std::to_string(c.first)]=c.second;
 for(const auto& c:s.names)names[std::to_string(c.first)]=c.second;
 j["entitySelection"]={{"all",s.all},{"classes",classes},{"names",names}};
}
} // namespace visual
