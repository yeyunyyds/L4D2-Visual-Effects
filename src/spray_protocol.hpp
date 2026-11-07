#pragma once
#include "image_scene.hpp"
#include "dango/json.hpp"
#include <set>
#include <deque>
#include <stdexcept>
namespace visual::spraysync {
using Peer=std::uint64_t;
inline bool steamId(Peer id){return (id>>56)==1&&((id>>52)&15)==1&&((id>>32)&0xfffff)==1&&std::uint32_t(id)!=0;}
inline Peer peer(const std::string& text){if(text.empty()||text.size()>20)return 0;Peer value=0;for(char c:text){if(c<'0'||c>'9'||value>(UINT64_MAX-unsigned(c-'0'))/10)return 0;value=value*10+unsigned(c-'0');}return steamId(value)?value:0;}
struct Packet{Peer from;std::string bytes;};
struct Transport{virtual ~Transport()=default;virtual Peer self()const=0;virtual bool send(Peer,const std::string&)=0;virtual bool receive(Packet&)=0;};
struct Settings{bool enabled=false,automatic=false;std::string room;std::set<Peer> peers;};
template<class Json> Settings readSettings(const Json& j){Settings s;auto p=j.find("sprayP2P");if(p==j.end()||!p->is_object())return s;
 if(p->contains("enabled")&&(*p)["enabled"].is_boolean())s.enabled=(*p)["enabled"].template get<bool>();
 if(p->contains("automatic")&&(*p)["automatic"].is_boolean())s.automatic=(*p)["automatic"].template get<bool>();
 if(p->contains("room")&&(*p)["room"].is_string())s.room=(*p)["room"].template get<std::string>();
 if(s.room.size()>64)s.room.clear();
 if(p->contains("peers")&&(*p)["peers"].is_array())for(const auto& x:(*p)["peers"])if(x.is_string()&&s.peers.size()<8){auto id=peer(x.template get<std::string>());if(id)s.peers.insert(id);}
 return s;
}
template<class Json> void writeSettings(Json& j,const Settings& s){std::vector<std::string> peers;for(auto id:s.peers)peers.push_back(std::to_string(id));j["sprayP2P"]={{"enabled",s.enabled},{"automatic",s.automatic},{"room",s.room},{"peers",peers}};}
inline bool hashKey(const std::string& key){return key.size()==64&&std::all_of(key.begin(),key.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
inline std::string hex(const unsigned char* data,std::size_t size){static const char digits[]="0123456789abcdef";std::string out;out.reserve(size*2);for(std::size_t i=0;i<size;++i){out+=digits[data[i]>>4];out+=digits[data[i]&15];}return out;}
inline std::vector<unsigned char> unhex(const std::string& text){if(text.size()>2800||text.size()%2)throw std::runtime_error("Invalid image chunk");std::vector<unsigned char> out;for(std::size_t i=0;i<text.size();i+=2){auto digit=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid hex digit");};out.push_back(static_cast<unsigned char>(digit(text[i])*16+digit(text[i+1])));}return out;}
inline nlohmann::json vector(dango::Vec3 v){return nlohmann::json::array({v.x,v.y,v.z});}
inline dango::Vec3 readVector(const nlohmann::json& j){if(!j.is_array()||j.size()!=3)throw std::runtime_error("Invalid vector");dango::Vec3 v{j.at(0).get<float>(),j.at(1).get<float>(),j.at(2).get<float>()};if(!dango::wall::finite(v)||std::abs(v.x)>100000||std::abs(v.y)>100000||std::abs(v.z)>100000)throw std::runtime_error("Invalid position");return v;}
inline nlohmann::json describe(const pictures::Placement& p,const std::string& hash){return {{"id",p.id},{"hash",hash},{"c",vector(p.panel.center)},{"n",vector(p.panel.normal)},{"r",vector(p.panel.right)},{"u",vector(p.panel.up)},{"w",p.panel.width},{"h",p.panel.height},{"a",p.opacity}};}
inline pictures::Placement placement(const nlohmann::json& j,Peer owner,std::uint32_t epoch){using namespace dango; pictures::Placement p;p.owner=owner;p.id=j.at("id").get<std::uint64_t>();auto hash=j.at("hash").get<std::string>();if(!p.id||!hashKey(hash))throw std::runtime_error("Invalid picture identity");p.key="p2p_"+hash+".png";
 p.panel={readVector(j.at("c")),readVector(j.at("n")),readVector(j.at("r")),readVector(j.at("u")),j.at("w").get<float>(),j.at("h").get<float>(),epoch};p.opacity=j.at("a").get<unsigned>();const auto& q=p.panel;
 if(!std::isfinite(q.width)||!std::isfinite(q.height)||q.width<.01f||q.height<.01f||q.width>512||q.height>512||p.opacity<1||p.opacity>10||std::abs(length(q.normal)-1)>.005f||std::abs(length(q.right)-1)>.005f||std::abs(length(q.up)-1)>.005f||std::abs(wall::dot(q.normal,q.right))>.005f||std::abs(wall::dot(q.normal,q.up))>.005f||std::abs(wall::dot(q.right,q.up))>.005f||std::abs(q.normal.z)>.5f||wall::dot(wall::cross(q.right,q.up),q.normal)<.99f)throw std::runtime_error("Invalid picture plane");
 return p;
}
struct Assembly{std::size_t size=0,received=0;std::vector<unsigned char> bytes;std::vector<bool> chunks;std::uint64_t deadline=0;
 explicit Assembly(std::size_t total=0,std::uint64_t now=0):size(total),deadline(now+30000){if(!total||total>20u*1024u*1024u)throw std::runtime_error("Image exceeds transfer limit");bytes.resize(total);chunks.resize((total+1399)/1400);}
 bool put(std::size_t offset,const std::vector<unsigned char>& data){if(offset%1400||offset>=size||data.size()!=std::min(std::size_t(1400),size-offset))throw std::runtime_error("Invalid chunk range");auto index=offset/1400;if(!chunks[index]){std::copy(data.begin(),data.end(),bytes.begin()+offset);chunks[index]=true;received+=data.size();}return received==size;}
};
}
