#include "tank_visual.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("Tank visual regression");}
int main(){using namespace visual::tank;using nlohmann::json;
 check(readVisual(json{{"tankVisual",{{"preset",2},{"step",8}}}}).preset==2);
 auto s=readVisual(json::object());check(s.preset==0&&!s.vertical&&s.y==-58);s.preset=2;s.vertical=true;s.x=44;s.y=72;json j;writeVisual(j,s);auto back=readVisual(j);check(back.preset==2&&back.vertical&&back.x==44&&back.y==72);
 j["tankVisual"]["x"]=1e50;j["tankVisual"]["y"]=-1e50;j["tankVisual"]["thickness"]=-99;back=readVisual(j);check(back.x==0&&back.y==-58&&back.thickness==4);
 for(unsigned preset=0;preset<3;++preset){s.preset=preset;for(bool vertical:{false,true}){s.vertical=vertical;std::vector<Segment> geometry;barGeometry(geometry,{300,200},s,1,.5f);demoRange(geometry,{300,200},1,preset);check(!geometry.empty());for(const auto& l:geometry)check(std::isfinite(l.a.x)&&std::isfinite(l.a.y)&&std::isfinite(l.b.x)&&std::isfinite(l.b.y));auto c=clampBar({-1000,10000},s,1,640,480);check(c.x>0&&c.y<480);}}
 dango::GameCameraFrame projection;projection.worldToClip={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};auto bar=placeBar(projection,{0,0,.5f},640,480,s);check(bar&&bar->x>0&&bar->y>0);check(!placeBar(projection,{2,0,.5f},640,480,s));projection.worldToClip[15]=-1;check(!placeBar(projection,{0,0,.5f},640,480,s));
 OverlayGate g;dango::GameCameraFrame camera;camera.active=1;camera.epoch=1;camera.tick=1;camera.worldToView[0]=1;
 g.poll(camera,false,false,true,true);check(!g.paused);g.poll(camera,true,false,true,true);check(g.paused);camera.tick=2;g.poll(camera,false,false,true,true);check(g.paused);camera.tick=3;camera.worldToView[0]=.99f;g.poll(camera,false,false,true,true);check(!g.paused);
 g.poll(camera,true,false,true,true);check(g.paused);g.poll(camera,false,false,true,true,true);check(!g.paused);g.poll(camera,true,true,true,true);check(!g.paused);g.poll(camera,false,false,true,true);g.poll(camera,true,false,true,true);check(g.paused);camera.epoch=2;g.poll(camera,false,false,true,true);check(!g.paused);
 std::cout<<"PASS: visual presets, bounded config roundtrip, horizontal/vertical shared geometry, screen clamping and independent Escape recovery\n";
}
