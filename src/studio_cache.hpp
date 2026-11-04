#pragma once
#include "studio_pose.hpp"
#include <map>
namespace visual {
class StudioCache {
 struct Entry{std::array<std::uint32_t,42> fingerprint;StudioPose pose;};
 std::map<std::uint32_t,Entry> entries;
 std::uint32_t epoch=0;
public:
 template<class Reader> const StudioPose* find(Reader& reader,std::uint32_t header,std::uint32_t currentEpoch,const char** reason){
  if(currentEpoch!=epoch){entries.clear();epoch=currentEpoch;}
  std::array<std::uint32_t,42> fingerprint{};
  if(!reader.read(header,fingerprint.data(),sizeof(fingerprint))){if(reason)*reason="Cached header unreadable";return nullptr;}
  auto found=entries.find(header);
  if(found!=entries.end()&&found->second.fingerprint==fingerprint)return &found->second.pose;
  if(found!=entries.end())entries.erase(found);
  StudioPose pose;if(!studioPose(reader,header,pose,reason))return nullptr;
  if(entries.size()>=64)entries.clear();
  auto inserted=entries.emplace(header,Entry{fingerprint,std::move(pose)});
  return &inserted.first->second.pose;
 }
};
}
