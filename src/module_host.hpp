#pragma once
#include <functional>
#include <memory>
#include <atomic>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
#include "dango/world_camera.hpp"
#include "dango/json.hpp"
#include "stretch.hpp"
struct IDirect3DDevice9;
namespace visual::modules {
struct Module {
 std::string id;
 std::shared_ptr<void> stateOwner;
 std::function<bool()> observing,modifying;
 std::function<void(std::uint32_t,int,const dango::Vec3&)> observeModel;
 std::function<void(std::uint32_t,std::uint32_t,const dango::Vec3&)> observeStudio;
 std::function<bool(std::uint32_t,std::uint32_t,const Matrix3x4*,const dango::Vec3&,Matrix3x4*,unsigned,unsigned&)> modifyPose;
 std::function<bool()> needsEntities,needsCombat,needsCompatibility;
 std::function<void()> prepare,quiesce,stop;
 std::function<void(const dango::GameCameraFrame&)> frame;
 std::function<void(std::uint64_t)> tick;
 std::function<unsigned()> pollInterval;
 std::function<void(const nlohmann::json&)> load;
 std::function<void(nlohmann::json&)> save;
 std::function<std::optional<std::string>(std::uintptr_t)> page;
 std::function<bool(std::uintptr_t)> action;
 std::function<bool(std::uintptr_t,int)> adjust;
 std::function<std::string()> rootItem,diagnostics;
 int menuOrder=0;
 std::function<void(const dango::GameCameraFrame&,bool,bool,bool)> pollOverlay;
 std::function<bool()> needsOverlay,needsNativeOverlay;
 std::function<void(IDirect3DDevice9*,float,float)> drawOverlay;
 std::function<void(float,float)> drawNativeOverlay;
 std::function<void(const char*)> unavailable;
 std::function<bool(std::uintptr_t)> previewKeys;
 std::function<void(std::uintptr_t,float,float,bool)> preview;
};
// Composition is frozen before threads start. Dispatch never mutates membership.
class Host {
 std::vector<Module> entries;
 bool frozen=false;std::atomic<bool> quiet{false},stopped{false},stopStarted{false};
public:
 void add(Module m){if(frozen)throw std::logic_error("module host is frozen");if(m.id.empty())throw std::invalid_argument("empty module id");for(auto& e:entries)if(e.id==m.id)throw std::invalid_argument("duplicate module id");entries.push_back(std::move(m));}
 void freeze(){frozen=true;}
 void prepare(){if(!quiet)for(auto& e:entries)if(e.prepare)e.prepare();}
 bool needsEntities()const{if(stopped)return false;for(auto& e:entries)if(e.needsEntities&&e.needsEntities())return true;return false;}
 bool needsCombat()const{if(quiet)return false;for(auto& e:entries)if(e.needsCombat&&e.needsCombat())return true;return false;}
 bool needsCompatibility()const{if(quiet)return false;for(auto& e:entries)if(e.needsCompatibility&&e.needsCompatibility())return true;return false;}
 void frame(const dango::GameCameraFrame& f){if(!stopped)for(auto& e:entries)if(e.frame)e.frame(f);}
 void tick(std::uint64_t t){if(!quiet)for(auto& e:entries)if(e.tick)e.tick(t);}
 unsigned pollInterval()const{unsigned result=200;for(auto& e:entries)if(e.pollInterval)result=std::min(result,e.pollInterval());return std::max(1u,result);}
 void load(const nlohmann::json& j){for(auto& e:entries)if(e.load)e.load(j);}
 void save(nlohmann::json& j)const{for(auto& e:entries)if(e.save)e.save(j);}
 std::optional<std::string> page(std::uintptr_t id)const{for(auto& e:entries)if(e.page){auto result=e.page(id);if(result)return result;}return std::nullopt;}
 bool action(std::uintptr_t id){for(auto& e:entries)if(e.action&&e.action(id))return true;return false;}
 bool adjust(std::uintptr_t id,int delta){for(auto& e:entries)if(e.adjust&&e.adjust(id,delta))return true;return false;}
 std::string rootItems()const{std::vector<const Module*> ordered;for(auto& e:entries)if(e.rootItem)ordered.push_back(&e);std::stable_sort(ordered.begin(),ordered.end(),[](auto a,auto b){return a->menuOrder<b->menuOrder;});std::string out;for(auto e:ordered)out+=e->rootItem();return out;}
 std::string diagnostics()const{std::string out;for(auto& e:entries)if(e.diagnostics)out+=" ["+e.id+" "+e.diagnostics()+"]";return out;}
 void pollOverlay(const dango::GameCameraFrame& f,bool escape,bool menu,bool foreground){if(quiet)return;for(auto& e:entries)if(e.pollOverlay)e.pollOverlay(f,escape,menu,foreground);}
 bool needsOverlay()const{if(quiet)return false;for(auto& e:entries)if((e.needsOverlay&&e.needsOverlay())||(e.needsNativeOverlay&&e.needsNativeOverlay()))return true;return false;}
 bool needsNativeOverlay()const{if(quiet)return false;for(auto& e:entries)if(e.needsNativeOverlay&&e.needsNativeOverlay())return true;return false;}
 void drawOverlay(IDirect3DDevice9* d,float w,float h){if(quiet)return;for(auto& e:entries)if(e.drawOverlay)try{e.drawOverlay(d,w,h);}catch(...){if(e.unavailable)e.unavailable("overlay exception");}}
 void drawNativeOverlay(float w,float h){if(quiet)return;for(auto& e:entries)if(e.drawNativeOverlay)e.drawNativeOverlay(w,h);}
 void unavailable(const char* reason){for(auto& e:entries)if(e.unavailable)e.unavailable(reason);}
 bool previewKeys(std::uintptr_t page)const{if(quiet)return false;for(auto& e:entries)if(e.previewKeys&&e.previewKeys(page))return true;return false;}
 void preview(std::uintptr_t page,float w,float h,bool focused){if(quiet)return;for(auto& e:entries)if(e.preview&&e.previewKeys&&e.previewKeys(page)){e.preview(page,w,h,focused);return;}}
 bool observing()const{if(quiet)return false;for(auto& e:entries)if(e.observing&&e.observing())return true;return false;}
 bool modifying()const{if(quiet)return false;for(auto& e:entries)if(e.modifying&&e.modifying())return true;return false;}
 void observeModel(std::uint32_t header,int index,const dango::Vec3& origin){if(quiet)return;for(auto& e:entries)if(e.observeModel&&e.observing&&e.observing())e.observeModel(header,index,origin);}
 void observeStudio(std::uint32_t header,std::uint32_t entity,const dango::Vec3& origin){if(quiet)return;for(auto& e:entries)if(e.observeStudio&&e.observing&&e.observing())e.observeStudio(header,entity,origin);}
 bool modifyPose(std::uint32_t header,std::uint32_t entity,const Matrix3x4* input,const dango::Vec3& origin,Matrix3x4* output,unsigned capacity,unsigned& count){if(quiet)return false;for(auto& e:entries)if(e.modifyPose&&e.modifying&&e.modifying()&&e.modifyPose(header,entity,input,origin,output,capacity,count))return true;return false;}
 void quiesce(){if(quiet.exchange(true))return;for(auto i=entries.rbegin();i!=entries.rend();++i)if(i->quiesce)i->quiesce();}
 void stop(){if(stopStarted.exchange(true))return;quiesce();stopped=true;for(auto i=entries.rbegin();i!=entries.rend();++i)if(i->stop)i->stop();}
};
}
