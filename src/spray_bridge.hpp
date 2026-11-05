#pragma once
#include "spray_protocol.hpp"
#include <atomic>
#include <mutex>
namespace visual::spraysync {
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
struct RemoteBatch{std::uint32_t epoch=0;Peer owner=0;std::vector<pictures::Placement> items;};
inline std::mutex remoteMutex;inline std::deque<RemoteBatch> remoteBatches;
inline std::atomic<Peer> ownId{0};inline std::atomic<unsigned> connected{0};
inline std::shared_ptr<const std::string> status=std::make_shared<const std::string>(u8"联机喷漆关闭");
inline void report(std::string text){if(*std::atomic_load(&status)==text)return;std::atomic_store(&status,std::make_shared<const std::string>(std::move(text)));}
inline void enqueue(RemoteBatch batch){std::lock_guard<std::mutex> lock(remoteMutex);for(auto& old:remoteBatches)if(old.owner==batch.owner){old=std::move(batch);return;}if(remoteBatches.size()<8)remoteBatches.push_back(std::move(batch));}
inline std::deque<RemoteBatch> take(){std::lock_guard<std::mutex> lock(remoteMutex);std::deque<RemoteBatch> out;out.swap(remoteBatches);return out;}
}
