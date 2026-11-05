#include "image_projection.hpp"
#include "json.hpp"
#include "hud_menu_policy.hpp"
#include <iostream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("wall picture scene regression");}
bool close(float a,float b){return std::abs(a-b)<.001f;}
int main(){using namespace visual::pictures;using namespace dango;
 for(auto key:{"a.png","PHOTO.JPG","shot.jpeg",u8"图片.bmp"})check(imageKey(key));
 for(auto key:{"","../a.png","C:a.png","a/b.jpg","a\\b.png","evil\".png","a.exe","a.png\n",".."})check(!imageKey(key));
 nlohmann::json j={{"font",27}};auto settings=readSettings(j);check(!settings.enabled&&settings.selected.empty()&&settings.size==60&&settings.rotation==0&&settings.opacity==10);
 settings.enabled=true;settings.selected="sample.png";settings.size=9999;settings.rotation=91;settings.opacity=0;
 writeSettings(j,settings);settings=readSettings(nlohmann::json::parse(j.dump()));
 check(j["font"]==27&&settings.enabled&&settings.size==512&&settings.rotation==90&&settings.opacity==1);
 auto invalid=readSettings(nlohmann::json{{"wallPictures",{{"enabled","yes"},{"selected","../a.png"},{"size",-1}}}});check(!invalid.enabled&&invalid.selected.empty()&&invalid.size==60);
 unsigned casts=0;
 auto flatWall=[&](Vec3 start,Vec3 end,trace::Result& hit){++casts;hit={};hit.start=start;hit.end=end;float d=end.y-start.y;
  if(std::abs(d)<.001f)return true;
  float t=-start.y/d;if(t<0||t>1)return true;
  hit.fraction=t;hit.end=start+(end-start)*t;hit.plane.normal={0,-1,0};hit.plane.distance=0;hit.contents=1;return true;
 };
 Scene scene;scene.observe(7);settings=Settings{};settings.enabled=true;settings.selected="sample.png";settings.size=120;settings.opacity=5;
 check(scene.place(settings,1000,500,{0,0,50},{0,-1,0},flatWall));check(casts==9&&scene.items.size()==1);
 const auto first=scene.items.front();
 check(trace::visible(first.panel,{0,-100,50},flatWall));
 auto blocked=[](Vec3 start,Vec3 end,trace::Result& hit){hit={};hit.start=start;hit.fraction=.5f;hit.end=start+(end-start)*.5f;return true;};
 check(!trace::visible(first.panel,{0,-100,50},blocked));check(first.key=="sample.png"&&first.opacity==5&&close(first.panel.width,120)&&close(first.panel.height,60)&&close(first.panel.center.y,-.75f));
 check(close(wall::dot(first.panel.right,first.panel.normal),0)&&close(wall::dot(first.panel.up,first.panel.normal),0));
 settings.rotation=90;check(scene.place(settings,500,1000,{0,0,50},{0,-1,0},flatWall));check(close(scene.items.back().panel.width,60)&&close(scene.items.back().panel.height,120)&&close(scene.items.back().panel.right.z,1)&&close(scene.items.back().panel.up.x,-1));
 auto count=scene.items.size();auto unsupported=[](Vec3,Vec3,trace::Result&){return false;};
 check(!scene.place(settings,500,1000,{0,0,50},{0,-1,0},unsupported)&&scene.items.size()==count);
 check(!scene.place(settings,0,1000,{0,0,50},{0,-1,0},flatWall));check(!scene.place(settings,MaxEdge+1,1000,{0,0,50},{0,-1,0},flatWall));
 check(!scene.place(settings,500,1000,{0,0,50},{0,0,1},flatWall));settings.enabled=false;check(!scene.place(settings,500,1000,{0,0,50},{0,-1,0},flatWall));settings.enabled=true;
 for(unsigned n=0;n<12;++n)check(scene.place(settings,500,500,{0,0,50},{0,-1,0},flatWall));
 check(scene.items.size()==2&&scene.items.front().id==scene.items.back().id-1);
 // Limits are per author; undo must never delete somebody else's spray.
 auto remote=scene.items.back();remote.owner=76561198000000001ull;remote.id=100;scene.items.push_back(remote);remote.id=101;scene.items.push_back(remote);remote.id=102;scene.items.push_back(remote);
 scene.limit(2);check(scene.items.size()==4&&scene.items[2].id==101);scene.undo();check(scene.items.size()==3&&scene.items.back().owner!=0);
 scene.items.erase(std::remove_if(scene.items.begin(),scene.items.end(),[](const auto& p){return p.owner!=0;}),scene.items.end());
 check(scene.place(settings,500,500,{0,0,50},{0,-1,0},flatWall));
 settings.sizes["sample.png"]=40;check(scene.resize(settings,flatWall)==2&&close(scene.items.front().panel.width,40));
 Scene profiles;profiles.observe(7);auto custom=settings;custom.selected="other.png";custom.sizes["other.png"]=90;
 check(profiles.place(custom,500,500,{0,0,50},{0,-1,0},flatWall));custom.selected="sample.png";check(profiles.place(custom,500,500,{100,0,50},{0,-1,0},flatWall));
 check(close(profiles.items[0].panel.width,90)&&close(profiles.items[1].panel.width,40));
 nlohmann::json customJson;writeSettings(customJson,custom);auto loaded=readSettings(customJson);check(effectiveSize(loaded,"other.png")==90&&effectiveSize(loaded,"sample.png")==40);
 custom.size=30;custom.sizes.clear();check(profiles.resize(custom,flatWall)==2&&close(profiles.items[0].panel.width,30)&&close(profiles.items[1].panel.width,30));
 scene.limit(1);check(scene.items.size()==1);scene.undo();check(scene.items.empty());
 scene.observe(7);check(scene.items.empty());scene.observe(8);check(scene.items.empty());scene.undo();scene.clear();check(scene.items.empty());
 auto item=first;item.visible=true;item.visibleTick=1000;GameCameraFrame camera;camera.active=1;camera.epoch=7;
 check(visibleLease(item,camera,1350)&&!visibleLease(item,camera,1351)&&!visibleLease(item,camera,999));camera.epoch=8;check(!visibleLease(item,camera,1100));camera.epoch=7;camera.active=0;check(!visibleLease(item,camera,1100));
 check(textureEdge(1)==1&&textureEdge(512)==512&&textureEdge(513)==1024&&textureEdge(MaxEdge)==MaxEdge);
 // Verify trace result shape/canaries and world hit rejection used by the actual adapter.
 trace::Result hit;check(flatWall({0,-10,50},{0,10,50},hit)&&trace::valid(hit,{0,-10,50},{0,10,50})&&trace::wallHit(hit));
 hit.surface.flags=4;check(!trace::wallHit(hit));trace::GuardedResult guarded;check(guarded.intact());guarded.after[0]=0;check(!guarded.intact());
 // Perspective quad uses placement-time wall axes and rejects other-map/backface draws.
 wall::Panel panel;check(wall::makePanel({100,0,0},{-1,0,0},120,60,7,panel));
 camera.active=1;camera.width=1920;camera.height=1080;camera.epoch=7;
 camera.worldToView={0,-1,0,0,0,0,1,0,1,0,0,0,0,0,0,1};camera.worldToClip={0,-1,0,0,0,0,1,0,1.0001f,0,0,-1.0001f,1,0,0,0};
 std::array<wall::Vertex,4> vertices;check(wall::projectPanel(panel,camera,1920,1080,vertices));check(vertices[0].u==0&&vertices[3].u==1&&vertices[3].v==1&&vertices[0].rhw>0);
 check(clippedPanel(panel,camera,1920,1080).size()>=3);
 // A wide tilted panel crosses the near plane while still covering the screen.
 panel.center={2,0,0};panel.right={.70710678f,.70710678f,0};panel.normal={-.70710678f,.70710678f,0};panel.width=20;panel.height=4;
 check(!wall::projectPanel(panel,camera,1920,1080,vertices));check(clippedPanel(panel,camera,1920,1080).size()>=3);
 auto legacy=readSettings(nlohmann::json{{"wallPictures",{{"size",120}}}});check(legacy.size==60&&legacy.maximum==2&&!legacy.bindT&&!legacy.autoNext);
 nlohmann::json saved;writeSettings(saved,legacy);check(readSettings(saved).size==60);
 panel.normal={1,0,0};check(!wall::projectPanel(panel,camera,1920,1080,vertices));
 for(auto id:{150u,151u,152u,153u,154u,300u})check(hud::opensChild(id));
 for(auto id:{200u,201u,202u,203u,204u,205u,211u,0x70000u})check(!hud::opensChild(id));
 std::cout<<"PASS: local picture settings/paths, aspect ratio/rotation, plane support, capacity/undo/map clear, visibility leases, guarded trace ABI and perspective projection\n";
}
