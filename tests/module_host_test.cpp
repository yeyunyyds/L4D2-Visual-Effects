#include "module_host.hpp"
#include <iostream>
using namespace visual::modules;
void check(bool v){if(!v)throw std::runtime_error("Module host regression");}
int main(){
 Host host;std::vector<std::string> events;bool active=true,other=false;
 Module a;a.id="a";a.menuOrder=2;a.rootItem=[] {return std::string("A");};
 a.needsEntities=[&]{return active;};a.needsCombat=[&]{return active;};
 a.prepare=[&]{events.push_back("prepare-a");};a.frame=[&](const auto&){events.push_back("frame-a");};
 a.tick=[&](auto){events.push_back("tick-a");};a.pollInterval=[] {return 20u;};
 a.load=[&](const auto& j){active=j.value("a",false);};a.save=[&](auto& j){j["a"]=active;};
 a.page=[](auto id)->std::optional<std::string>{if(id==10)return "page-a";return std::nullopt;};
 a.action=[&](auto id){if(id!=11)return false;active=!active;return true;};
 a.quiesce=[&]{events.push_back("quiet-a");};a.stop=[&]{events.push_back("stop-a");};
 a.observing=[&]{return active;};a.observeModel=[&](auto,auto,const auto&){events.push_back("observe-a");};
 Module b;b.id="b";b.menuOrder=1;b.rootItem=[] {return std::string("B");};
 b.needsEntities=[&]{return other;};b.frame=[&](const auto&){events.push_back("frame-b");};
 b.pollInterval=[] {return 100u;};b.save=[&](auto& j){j["b"]=other;};
 b.quiesce=[&]{events.push_back("quiet-b");};b.stop=[&]{events.push_back("stop-b");};
 b.observing=[] {return true;};b.observeModel=[&](auto,auto,const auto&){events.push_back("observe-b");};
 b.modifying=[] {return true;};b.modifyPose=[](auto,auto,const auto*,const auto&,auto* out,auto,unsigned& count){out[0].m[2][3]=9;count=1;return true;};
 host.add(a);host.add(b);bool duplicate=false;try{host.add(a);}catch(const std::invalid_argument&){duplicate=true;}check(duplicate);
 host.freeze();bool frozen=false;try{Module c;c.id="c";host.add(c);}catch(const std::logic_error&){frozen=true;}check(frozen);
 check(host.rootItems()=="BA"&&host.needsEntities()&&host.needsCombat()&&host.pollInterval()==20);
 check(host.page(10)=="page-a"&&!host.page(99));check(!host.action(99));
 host.prepare();host.frame({});host.tick(1);check(events==std::vector<std::string>({"prepare-a","frame-a","frame-b","tick-a"}));events.clear();
 host.observeModel(1,2,{});check(events==std::vector<std::string>({"observe-a","observe-b"}));events.clear();
 visual::Matrix3x4 input{},out{};unsigned count=0;check(host.modifyPose(0,0,&input,{},&out,1,count)&&count==1&&out.m[2][3]==9&&input.m[2][3]==0);
 check(host.action(11)&&!host.needsEntities()&&!host.needsCombat());other=true;check(host.needsEntities());
 nlohmann::json j;host.save(j);check(j["a"]==false&&j["b"]==true);host.load({{"a",true}});check(active);
 host.quiesce();check(events==std::vector<std::string>({"quiet-b","quiet-a"}));events.clear();
 host.prepare();host.tick(2);host.observeModel(1,2,{});check(events.empty()&&!host.needsCombat()&&!host.modifying());
 host.frame({});check(events==std::vector<std::string>({"frame-a","frame-b"})); // restore callbacks must still drain
 events.clear();host.stop();host.stop();check(events==std::vector<std::string>({"stop-b","stop-a"}));events.clear();host.frame({});check(events.empty());
 Host reduced;reduced.add(b);reduced.freeze();j=nlohmann::json::object();reduced.save(j);check(!j.contains("a")&&j.contains("b")&&reduced.rootItems()=="B");
 Host overlays;Module failing;failing.id="bad-overlay";failing.drawOverlay=[](auto*,auto,auto){throw std::runtime_error("failure");};failing.unavailable=[&](auto){events.push_back("reported");};Module healthy;healthy.id="healthy";healthy.drawOverlay=[&](auto*,auto,auto){events.push_back("drawn");};overlays.add(failing);overlays.add(healthy);overlays.freeze();overlays.drawOverlay(nullptr,1,1);check(events==std::vector<std::string>({"reported","drawn"}));events.clear();overlays.quiesce();overlays.drawOverlay(nullptr,1,1);check(events.empty());
 Host directStop;int drained=0;Module cleanup;cleanup.id="cleanup";cleanup.frame=[&](const auto&){++drained;};cleanup.quiesce=[&]{directStop.frame({});};directStop.add(cleanup);directStop.freeze();directStop.stop();check(drained==1);directStop.frame({});check(drained==1);
 std::weak_ptr<int> weak;
 {Host lifetime;Module owned;owned.id="owned";auto state=std::make_shared<int>(12);weak=state;owned.stateOwner=state;owned.action=[pointer=state.get()](auto id){return id==unsigned(*pointer);};lifetime.add(std::move(owned));lifetime.freeze();state.reset();check(!weak.expired()&&lifetime.action(12));}
 check(weak.expired());
 std::cout<<"PASS: module registration, demand aggregation, ordered lifecycle/restore drain, independent config/menu dispatch, observer broadcast, pose ownership, overlay failure isolation and module omission\n";
}
