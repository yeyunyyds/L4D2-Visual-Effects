#pragma once
#include "spray_bridge.hpp"
#include "source_roster.hpp"
#include "spray_steam.hpp"
#include "spray_codec.hpp"
#include "image_runtime.hpp"
namespace visual::spraysync {
struct Upload{std::string hash;std::shared_ptr<const std::vector<unsigned char>> data;std::size_t offset=0;};
struct Connection{std::uint64_t nonce=0,seen=0,revision=0;std::set<std::uint64_t> retired; nlohmann::json state=nlohmann::json::array();std::deque<Upload> uploads;std::uint64_t requestAt=0;};
class Network {
 std::unique_ptr<steam::Transport> transport;std::shared_ptr<const Settings> previous;
 std::uint32_t epoch=0;std::uint64_t nonce=0,sequence=0,nextHello=0,nextState=0,retryAt=0;
 std::map<Peer,Connection> connections;
 struct Incoming{Peer owner;Assembly data;};std::map<std::string,Incoming> incoming;std::map<std::string,std::pair<Peer,std::uint64_t>> requested;
 struct Encoded{std::uint64_t revision;std::string hash;std::shared_ptr<const std::vector<unsigned char>> data;};std::map<std::string,Encoded> encoded;
 std::map<std::string,std::shared_ptr<const std::vector<unsigned char>>> outgoing;
 std::set<std::string> cached;std::filesystem::path cache;std::size_t cacheBytes=0;
 bool send(Peer to,nlohmann::json message,bool session=true){message["v"]=1;message["app"]="LVSP";message["room"]=previous->room;message["n"]=nonce;if(session)message["to"]=connections.at(to).nonce;auto bytes=message.dump();return bytes.size()<=4096&&transport->send(to,bytes);}
 void clearPeer(Peer p){enqueue({epoch,p,{}});connections.erase(p);for(auto i=requested.begin();i!=requested.end();)if(i->second.first==p)i=requested.erase(i);else ++i;for(auto i=incoming.begin();i!=incoming.end();)if(i->second.owner==p)i=incoming.erase(i);else ++i;}
 void reset(){for(const auto& p:connections)enqueue({epoch,p.first,{}});connections.clear();incoming.clear();requested.clear();encoded.clear();outgoing.clear();cached.clear();cacheBytes=0;cache.clear();transport.reset();connected=0;ownId=0;nextHello=nextState=0;nonce=steam::nonce();sequence=0;}
 void prepareCache(){if(!cache.empty())return;cache=pictures::folder.parent_path()/L"spray-cache";std::filesystem::create_directories(cache);
  // Received images are session-scoped; remove only our hash-derived cache files on first use.
  std::error_code error;for(const auto& e:std::filesystem::directory_iterator(cache,error)){auto name=e.path().filename().u8string();if(name.size()==72&&name.substr(0,4)=="p2p_"&&hashKey(name.substr(4,64)))std::filesystem::remove(e.path(),error);}
 }
 void pruneCache(){std::set<std::string> used;for(const auto& c:connections)for(const auto& row:c.second.state)used.insert(row.at("hash").get<std::string>());
  for(auto i=cached.begin();i!=cached.end();)if(!used.count(*i)){auto path=cache/std::filesystem::u8path("p2p_"+*i+".png");std::error_code error;auto size=std::filesystem::file_size(path,error);if(!error&&std::filesystem::remove(path,error))cacheBytes-=std::min(cacheBytes,std::size_t(size));i=cached.erase(i);}else ++i;
 }
 bool store(const std::string& hash,const std::vector<unsigned char>& bytes){if(digest(bytes)!=hash)return false;auto path=cache/std::filesystem::u8path("p2p_"+hash+".png");
  if(cacheBytes+bytes.size()>pictures::CacheBudget)pruneCache();
  if(cacheBytes+bytes.size()>pictures::CacheBudget){report(u8"联机图片缓存已满，重新开启联机可清理");return false;}
  auto temp=path;temp+=L".tmp";{std::ofstream file(temp,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));if(!file)return false;}
  // Validate with the same bounded decoder before making the file available to rendering.
  auto decoded=pictures::decode(temp,0);if(!decoded){std::filesystem::remove(temp);return false;}std::filesystem::rename(temp,path);cacheBytes+=bytes.size();cached.insert(hash);return true;
 }
 void deliver(Peer owner,Connection& connection){std::vector<pictures::Placement> items;
  for(const auto& description:connection.state){auto p=placement(description,owner,epoch);auto hash=description.at("hash").get<std::string>();if(cached.count(hash))items.push_back(std::move(p));}
  enqueue({epoch,owner,std::move(items)});
 }
 void receive(const Packet& packet,std::uint64_t now){if(!previous->peers.count(packet.from)||packet.bytes.size()>4096)return;
  auto j=nlohmann::json::parse(packet.bytes);if(!j.is_object()||j.value("app",std::string())!="LVSP"||j.value("v",0)!=1||j.value("room",std::string())!=previous->room)return;
  auto remote=j.at("n").get<std::uint64_t>();if(!remote)return;auto kind=j.at("k").get<std::string>();
  if(kind=="hello"){send(packet.from,{{"k","ack"},{"to",remote}},false);return;}
  if(j.at("to").get<std::uint64_t>()!=nonce)return;
  if(kind=="ack"){auto& c=connections[packet.from];if(c.retired.count(remote))return;if(c.nonce!=remote){if(c.nonce){c.retired.insert(c.nonce);if(c.retired.size()>32){clearPeer(packet.from);return;}enqueue({epoch,packet.from,{}});}c.nonce=remote;c.revision=0;c.state=nlohmann::json::array();c.uploads.clear();nextState=0;}c.seen=now;return;}
  auto found=connections.find(packet.from);if(found==connections.end()||found->second.nonce!=remote)return;auto& c=found->second;c.seen=now;
  if(kind=="state"){auto rev=j.at("seq").get<std::uint64_t>();auto state=j.at("items");if(rev<=c.revision||!state.is_array()||state.size()>pictures::MaxPictures)return;
   std::set<std::uint64_t> ids;for(const auto& row:state){auto p=placement(row,packet.from,epoch);if(!ids.insert(p.id).second)return;}
   c.state=std::move(state);c.revision=rev;
   for(auto i=requested.begin();i!=requested.end();){bool remains=false;for(const auto& row:c.state)if(row.at("hash")==i->first)remains=true;
    if(i->second.first==packet.from&&!remains){incoming.erase(i->first);i=requested.erase(i);}else ++i;
   }deliver(packet.from,c);
  }else if(kind=="need"){auto hash=j.at("hash").get<std::string>();auto asset=outgoing.find(hash);if(asset==outgoing.end()||c.uploads.size()>=pictures::MaxPictures)return;
   if(std::none_of(c.uploads.begin(),c.uploads.end(),[&](const auto& u){return u.hash==hash;}))c.uploads.push_back({hash,asset->second,0});
  }else if(kind=="chunk"){auto hash=j.at("hash").get<std::string>();if(!hashKey(hash)||cached.count(hash)||!requested.count(hash)||requested.at(hash).first!=packet.from)return;
   bool wanted=false;for(const auto& row:c.state)if(row.at("hash")==hash)wanted=true;if(!wanted)return;
   auto total=j.at("total").get<std::size_t>(),offset=j.at("offset").get<std::size_t>();auto bytes=unhex(j.at("data").get<std::string>());
   auto entry=incoming.find(hash);if(entry==incoming.end()){std::size_t used=0;for(const auto& p:incoming)used+=p.second.data.size;if(incoming.size()>=2||total>32u*1024u*1024u-used)return;entry=incoming.emplace(hash,Incoming{packet.from,Assembly(total,now)}).first;}
   if(entry->second.owner!=packet.from||entry->second.data.size!=total)return;
   auto before=entry->second.data.received;bool complete=entry->second.data.put(offset,bytes);if(entry->second.data.received>before)entry->second.data.deadline=now+30000;
   if(complete){if(store(hash,entry->second.data.bytes))for(auto& p:connections)deliver(p.first,p.second);incoming.erase(entry);requested.erase(hash);}
  }
 }
public:
 void stop(){reset();}
 void tick(std::uint64_t now){
  auto configured=std::atomic_load(&options);auto roster=std::atomic_load(&sprayroster::snapshot);auto resolved=*configured;
  if(resolved.automatic){resolved.peers.clear();resolved.room.clear();if(roster->fresh(std::int64_t(now))){resolved.peers=roster->others();resolved.room="auto/"+std::to_string(roster->cohort());}}
  std::shared_ptr<const Settings> settings=previous;
  if(!settings||settings->enabled!=resolved.enabled||settings->automatic!=resolved.automatic||settings->room!=resolved.room||settings->peers!=resolved.peers)settings=std::make_shared<const Settings>(std::move(resolved));
  auto frame=std::atomic_load(&pictures::sceneFrame);auto camera=std::atomic_load(&source_camera::snapshot);
  if(previous!=settings||epoch!=camera->epoch){reset();previous=settings;epoch=camera->epoch;retryAt=0;}
  if(!settings->enabled||!std::atomic_load(&pictures::options)->enabled||!camera->active){if(transport)reset();report(u8"联机喷漆关闭或未进入地图");return;}
  if(settings->room.empty()||settings->peers.empty()){if(transport)reset();report(settings->automatic?std::string(u8"自动匹配：")+roster->status:std::string(u8"请在 hud.json 配置相同 room 与双方 SteamID64"));return;}
  try{
   prepareCache();if(!transport){if(now<retryAt)return;retryAt=now+5000;auto candidate=std::make_unique<steam::Transport>();candidate->connect();if(settings->automatic&&candidate->self()!=roster->local)throw std::runtime_error("Game roster/Steam identity mismatch");candidate->allow(settings->peers);ownId=candidate->self();transport=std::move(candidate);}
   transport->pump(std::int64_t(now));Packet packet;for(unsigned budget=0;budget<32&&transport->receive(packet);++budget){try{receive(packet,now);}catch(const std::exception&){/* Invalid peer data is rejected without changing local state. */}}
   for(auto i=incoming.begin();i!=incoming.end();)if(now>i->second.data.deadline){requested.erase(i->first);i=incoming.erase(i);}else ++i;
   for(auto i=requested.begin();i!=requested.end();)if(!incoming.count(i->first)&&now>i->second.second)i=requested.erase(i);else ++i;
   for(auto i=connections.begin();i!=connections.end();)if(now-i->second.seen>10000){auto owner=i->first;++i;clearPeer(owner);}else ++i;
   if(now>=nextHello){nextHello=now+1000;for(auto p:settings->peers)if(p!=transport->self())send(p,{{"k","hello"}},false);}
   if(now>=nextState&&frame->epoch==epoch){nextState=now+1000;auto decoded=std::atomic_load(&pictures::assets);nlohmann::json state=nlohmann::json::array();std::set<std::string> used;outgoing.clear();
    for(const auto& p:frame->items){if(p.owner)continue;auto pixels=decoded->decoded.find(p.key);if(pixels==decoded->decoded.end())continue;used.insert(p.key);auto e=encoded.find(p.key);
     if(e==encoded.end()||e->second.revision!=pixels->second->revision){auto bytes=std::make_shared<const std::vector<unsigned char>>(png(*pixels->second));std::size_t memory=bytes->size();for(const auto& old:encoded)if(old.first!=p.key)memory+=old.second.data->size();if(memory>pictures::CacheBudget)continue;encoded[p.key]={pixels->second->revision,digest(*bytes),bytes};}
     auto& asset=encoded.at(p.key);outgoing[asset.hash]=asset.data;state.push_back(describe(p,asset.hash));
    }
    for(auto i=encoded.begin();i!=encoded.end();)if(!used.count(i->first))i=encoded.erase(i);else ++i;
    ++sequence;for(const auto& p:connections)send(p.first,{{"k","state"},{"seq",sequence},{"items",state}});
   }
   unsigned budget=16;for(auto& entry:connections){auto& c=entry.second;
    if(now>=c.requestAt){c.requestAt=now+3000;for(const auto& row:c.state){auto hash=row.at("hash").get<std::string>();if(!cached.count(hash)&&!incoming.count(hash)&&requested.empty()){
      // Reserve one receiver slot before requesting: simultaneous peers cannot flood
      // the assembler or lose early chunks while another large image is arriving.
      if(send(entry.first,{{"k","need"},{"hash",hash}}))requested[hash]={entry.first,now+30000};
     }}}
    while(budget&&!c.uploads.empty()){auto& upload=c.uploads.front();if(!outgoing.count(upload.hash)){c.uploads.pop_front();continue;}auto size=std::min(std::size_t(1400),upload.data->size()-upload.offset);
     if(!send(entry.first,{{"k","chunk"},{"hash",upload.hash},{"total",upload.data->size()},{"offset",upload.offset},{"data",hex(upload.data->data()+upload.offset,size)}}))break;
     --budget;upload.offset+=size;if(upload.offset==upload.data->size())c.uploads.pop_front();
    }
   }
   connected=unsigned(connections.size());report(u8"Steam P2P 已握手 "+std::to_string(connections.size())+u8" 人 / 缓存 "+std::to_string(cached.size())+u8" 张");
  }catch(const std::exception& error){report(std::string(u8"联机接口：")+error.what());source_camera::log(std::string("Spray P2P: ")+error.what());reset();retryAt=now+5000;}
 }
};
inline Network network;
}
