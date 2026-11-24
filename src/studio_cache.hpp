#pragma once
#include "studio_pose.hpp"
#include <map>
namespace visual {
struct EmptyStudioCacheData {};
template<class Data=EmptyStudioCacheData> class StudioCacheWith {
 struct Entry{std::array<std::uint32_t,42> fingerprint;StudioPose pose;Data data{};};
 std::map<std::uint32_t,Entry> entries;
 std::uint32_t epoch=0;
public:
 template<class Reader> const StudioPose* find(Reader& reader,std::uint32_t header,std::uint32_t currentEpoch,const char** reason,Data** data=nullptr){
  if(data)*data=nullptr;
  if(currentEpoch!=epoch){entries.clear();epoch=currentEpoch;}
  std::array<std::uint32_t,42> fingerprint{};
  if(!reader.read(header,fingerprint.data(),sizeof(fingerprint))){if(reason)*reason="Cached header unreadable";return nullptr;}
  auto found=entries.find(header);
  if(found!=entries.end()&&found->second.fingerprint==fingerprint){
   if(data)*data=&found->second.data;
   return &found->second.pose;
  }
  if(found!=entries.end())entries.erase(found);
  StudioPose pose;if(!studioPose(reader,header,pose,reason))return nullptr;
  if(entries.size()>=64)entries.clear();
  auto inserted=entries.emplace(header,Entry{fingerprint,std::move(pose)});
  if(data)*data=&inserted.first->second.data;
  return &inserted.first->second.pose;
 }
};
using StudioCache=StudioCacheWith<>;
}
