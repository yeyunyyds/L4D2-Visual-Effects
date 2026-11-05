#include "spray_protocol.hpp"
#include <iostream>
#include <stdexcept>
using namespace visual::spraysync;
void check(bool ok){if(!ok)throw std::runtime_error("spray protocol regression");}
template<class Function> void rejects(Function function){bool rejected=false;try{function();}catch(const std::exception&){rejected=true;}check(rejected);}
int main(){
 constexpr Peer alice=76561198000000001ull,bob=76561198000000002ull;
 check(peer(std::to_string(alice))==alice&&steamId(bob));for(auto s:{"","-1","0","76561198000000001x","18446744073709551616","1"})check(peer(s)==0);
 nlohmann::json config={{"sprayP2P",{{"enabled",true},{"room","test-room"},{"peers",nlohmann::json::array({std::to_string(alice),"invalid",std::to_string(bob)})}}}};
 auto settings=readSettings(config);check(settings.enabled&&settings.peers.size()==2);writeSettings(config,settings);check(readSettings(config).peers==settings.peers&&!readSettings(nlohmann::json{}).enabled);
 visual::pictures::Placement p;p.id=7;p.opacity=8;check(dango::wall::makePanel({100,0,0},{-1,0,0},60,30,9,p.panel));std::string hash(64,'a');auto wire=describe(p,hash);auto restored=placement(wire,bob,12);check(restored.id==7&&restored.owner==bob&&restored.panel.epoch==12&&restored.key=="p2p_"+hash+".png"&&restored.panel.width==60);
 for(auto value:{-1.,0.,513.,1e100}){auto bad=wire;bad["w"]=value;rejects([&]{placement(bad,bob,12);});}
 auto bad=wire;bad["hash"]="../../bad";rejects([&]{placement(bad,bob,12);});bad=wire;bad["n"]=nlohmann::json::array({0,0,0});rejects([&]{placement(bad,bob,12);});bad=wire;bad["r"]=bad["u"];rejects([&]{placement(bad,bob,12);});
 std::vector<unsigned char> image(4201);for(unsigned i=0;i<image.size();++i)image[i]=static_cast<unsigned char>(i*37);
 Assembly transfer(image.size(),100);check(transfer.deadline==30100);
 for(std::size_t offset:{4200u,1400u,0u,1400u,2800u}){auto size=std::min(std::size_t(1400),image.size()-offset);auto data=unhex(hex(image.data()+offset,size));bool complete=transfer.put(offset,data);check(complete==(offset==2800));}
 check(transfer.bytes==image&&transfer.received==image.size());rejects([&]{transfer.put(1,{0});});rejects([&]{transfer.put(image.size(),{0});});rejects([&]{transfer.put(0,{0});});rejects([]{Assembly tooLarge(20u*1024u*1024u+1);});rejects([]{Assembly empty(0);});rejects([]{unhex("xx");});rejects([]{unhex("0");});
 std::cout<<"PASS: SteamID allowlists, opt-in settings, bounded geometry, owner/epoch assignment, chunk reordering/duplicates/completion and malformed transfer rejection\n";
}
