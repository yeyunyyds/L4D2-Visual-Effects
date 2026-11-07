#include "tank_visual.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("Tank visual regression");}
int main(){using namespace visual::tank;using nlohmann::json;
 check(readVisual(json{{"tankVisual",{{"preset",2},{"step",8}}}}).preset==2);
 auto s=readVisual(json::object());check(s.preset==0&&!s.vertical&&s.y==-58);s.preset=2;s.vertical=true;s.x=44;s.y=72;json j;writeVisual(j,s);auto back=readVisual(j);check(back.preset==2&&back.vertical&&back.x==44&&back.y==72);
 j["tankVisual"]["x"]=1e50;j["tankVisual"]["y"]=-1e50;j["tankVisual"]["thickness"]=-99;back=readVisual(j);check(back.x==0&&back.y==-58&&back.thickness==4);
 for(unsigned preset=0;preset<3;++preset){s.preset=preset;for(bool vertical:{false,true}){s.vertical=vertical;std::vector<Segment> geometry;barGeometry(geometry,{300,200},s,1,.5f);demoRange(geometry,{300,200},1,preset);check(!geometry.empty());for(const auto& l:geometry)check(std::isfinite(l.a.x)&&std::isfinite(l.a.y)&&std::isfinite(l.b.x)&&std::isfinite(l.b.y));}}
 // Perspective camera looking along +X, with Z up and a square-pixel projection.
 dango::GameCameraFrame projection;projection.worldToView={0,-1,0,0,0,0,1,0,1,0,0,0,0,0,0,1};projection.worldToClip={0,-.5625f,0,0,0,0,1,0,1,0,0,-1,1,0,0,0};
 s=VisualSettings{};auto nearBar=placeBar(projection,{200,0,45},1920,1080,s,180),farBar=placeBar(projection,{800,0,45},1920,1080,s,180);check(nearBar&&farBar&&std::abs(nearBar->scale/farBar->scale-4)<.001f);
 auto expected=attachedBarAnchor({200,0,45},180,s);check(std::abs(nearBar->anchor.z-expected.z)<.001f);check(!placeBar(projection,{-200,0,45},1920,1080,s));check(!placeBar(projection,{200,1000,45},1920,1080,s));
 // FOV/viewport changes scale the bar and offsets together, not just the Tank.
 auto zoom=projection;for(unsigned i=0;i<8;++i)zoom.worldToClip[i]*=1.5f;auto zoomBar=placeBar(zoom,{200,0,45},1920,1080,s,180);check(zoomBar&&std::abs(zoomBar->scale/nearBar->scale-1.5f)<.001f);
 auto small=placeBar(projection,{200,0,45},1280,720,s,180);check(small&&std::abs(small->scale/nearBar->scale-2.f/3)<.001f);
 auto edge=placeBar(projection,{200,350,45},1920,1080,s,180);check(edge&&edge->x<30);check(!placeBar(projection,{1.01f,0,45},1920,1080,s));
 // The world anchor is camera-independent and follows the Tank's own right axis.
 s.x=40;auto anchor=attachedBarAnchor({200,0,45},0,s);check(std::abs(anchor.x-200)<.001f&&std::abs(anchor.y+30)<.001f);auto turned=attachedBarAnchor({200,0,45},90,s);check(std::abs(turned.x-230)<.001f&&std::abs(turned.y)<.001f);
 auto movedCamera=projection;movedCamera.worldToClip[3]=.1f;auto moved=placeBar(movedCamera,{200,0,45},1920,1080,s,0);check(moved&&std::abs(moved->anchor.x-anchor.x)<.001f&&std::abs(moved->anchor.y-anchor.y)<.001f);
 s.showDirection=false;s.showBar=false;s.showRange=false;s.showTarget=false;writeVisual(j,s);back=readVisual(j);check(!back.showDirection&&!back.showBar&&!back.showRange&&!back.showTarget&&j["tankVisual"]["coordinateSpace"]=="tank-world");
 RenderAnchor pose{2,0x1001,1000,{200,0,45}};check(validRenderAnchor(pose,2,0x1001,1050,{210,0,45}));check(!validRenderAnchor(pose,3,0x1001,1050,{210,0,45}));check(!validRenderAnchor(pose,2,0x2001,1050,{210,0,45}));check(!validRenderAnchor(pose,2,0x1001,1101,{210,0,45}));check(!validRenderAnchor(pose,2,0x1001,1050,{800,0,45}));
 OverlayGate g;dango::GameCameraFrame camera;camera.active=1;camera.epoch=1;camera.tick=1;camera.worldToView[0]=1;
 g.poll(camera,false,false,true,true);check(!g.paused);g.poll(camera,true,false,true,true);check(g.paused);camera.tick=2;g.poll(camera,false,false,true,true);check(g.paused);camera.tick=3;camera.worldToView[0]=.99f;g.poll(camera,false,false,true,true);check(!g.paused);
 g.poll(camera,true,false,true,true);check(g.paused);g.poll(camera,false,false,true,true,true);check(!g.paused);g.poll(camera,true,true,true,true);check(!g.paused);g.poll(camera,false,false,true,true);g.poll(camera,true,false,true,true);check(g.paused);camera.epoch=2;g.poll(camera,false,false,true,true);check(!g.paused);
 std::cout<<"PASS: visual presets, bounded config roundtrip, horizontal/vertical shared geometry, entity-space anchoring, perspective/FOV/distance scaling, display flags and independent Escape recovery\n";
}
