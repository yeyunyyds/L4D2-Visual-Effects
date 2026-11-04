#pragma once
#include "target_identity.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
namespace visual {
template<class Frame,std::size_t Limit> struct TargetIndex {
 std::array<const typename decltype(Frame::records)::value_type*,Limit> records{};
 std::unordered_map<std::uint32_t,std::size_t> bases;
 template<class EntryReader,class Predicate> void rebuild(const Frame& frame,EntryReader entryReader,Predicate selected){
  records.fill(nullptr);bases.clear();
  for(const auto& r:frame.records){
   if(r.index>=Limit || !eligibleRecord(r) || !selected(r))continue;
   records[r.index]=&r;
   auto entry=entryReader(r.index);
   if(entry.entity && entry.serial==r.serial)bases.emplace(entry.entity,r.index);
  }
 }
 template<class EntryReader> void rebuild(const Frame& frame,EntryReader entryReader){rebuild(frame,entryReader,[](const auto&){return true;});}
};
} // namespace visual
