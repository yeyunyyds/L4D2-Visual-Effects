#include "tank_visual.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("Tank visual regression");}
int main(){using namespace visual::tank;using nlohmann::json;
 check(readVisual(json{{"tankVisual",{{"preset",2},{"step",8}}}}).preset==2);
 auto s=readVisual(json::object());check(s.preset==2&&s.vertical&&s.y==-24);s.preset=2;s.vertical=true;s.x=44;s.y=72;json j;writeVisual(j,s);auto back=readVisual(j);check(back.preset==2&&back.vertical&&back.x==44&&back.y==72);
 j["tankVisual"]["x"]=1e50;j["tankVisual"]["y"]=-1e50;j["tankVisual"]["thickness"]=-99;back=readVisual(j);check(back.x==-32&&back.y==-24&&back.thickness==1);
 for(unsigned preset=0;preset<3;++preset){s.preset=preset;for(bool vertical:{false,true}){s.vertical=vertical;std::vector<Segment> geometry;barGeometry(geometry,{300,200},s,1,.5f);demoRange(geometry,{300,200},1,preset);check(!geometry.empty());for(const auto& l:geometry)check(std::isfinite(l.a.x)&&std::isfinite(l.a.y)&&std::isfinite(l.b.x)&&std::isfinite(l.b.y));}}
 // New defaults apply to legacy files without rewriting their offsets/aspect.
 auto defaults=readVisual(json{{"tankVisual",{{"x",17},{"length",100}}}});check(std::abs(defaults.barSize-.55f)<.001f&&defaults.arrowWidth==3&&defaults.rangeWidth==3&&defaults.x==17&&defaults.length==100);
 auto before=defaults;adjustFine(defaults,FineField::BarSize,1);check(std::abs(defaults.barSize-.56f)<.001f&&defaults.x==before.x&&defaults.length==before.length&&defaults.arrowWidth==before.arrowWidth);
 adjustFine(defaults,FineField::ArrowWidth,1);check(std::abs(defaults.arrowWidth-3.1f)<.001f&&defaults.rangeWidth==3);adjustFine(defaults,FineField::RangeWidth,-1);check(std::abs(defaults.rangeWidth-2.9f)<.001f);adjustFine(defaults,FineField::Thickness,-1);check(std::abs(defaults.thickness-7.9f)<.001f);
 writeVisual(j,defaults);auto fine=readVisual(j);check(std::abs(fine.barSize-.56f)<.001f&&std::abs(fine.arrowWidth-3.1f)<.001f&&std::abs(fine.rangeWidth-2.9f)<.001f&&std::abs(fine.thickness-7.9f)<.001f);
 defaults.barSize=.2f;adjustFine(defaults,FineField::BarSize,-1);check(std::abs(defaults.barSize-.2f)<.001f);defaults.arrowWidth=6;adjustFine(defaults,FineField::ArrowWidth,1);check(defaults.arrowWidth==6);check(finePage(325)&&finePage(326)&&finePage(327)&&!finePage(321)&&!finePage(320));
 // Entire bar geometry (including the frame) shrinks by 80%, without moving its center.
 auto extent=[](const std::vector<Segment>& geometry){float lo=1e9f,hi=-1e9f;for(const auto& l:geometry){lo=std::min(lo,std::min(l.a.x,l.b.x));hi=std::max(hi,std::max(l.a.x,l.b.x));}return hi-lo;};
 VisualSettings full,smallStyle;full.barSize=1;full.vertical=false;smallStyle.barSize=.8f;smallStyle.vertical=false;std::vector<Segment> fullLines,smallLines;barGeometry(fullLines,{200,200},full,1,.5f);barGeometry(smallLines,{200,200},smallStyle,1,.5f);check(std::abs(extent(smallLines)/extent(fullLines)-.8f)<.001f);
 for(unsigned preset=0;preset<3;++preset){VisualSettings style;style.preset=preset;style.arrowWidth=2;style.rangeWidth=2;std::vector<Segment> geometry;demoRange(geometry,{0,0},1,style);auto colors=palette(preset);check(geometry.front().width==colors.width*2);check(geometry.back().width==colors.width*2);style.arrowWidth=3.2f;style.rangeWidth=1.4f;geometry.clear();demoRange(geometry,{0,0},1,style);check(std::abs(geometry.front().width-colors.width*1.4f)<.001f&&std::abs(geometry.back().width-colors.width*3.2f)<.001f);}
 // Perspective camera looking along +X, with Z up and a square-pixel projection.
 dango::GameCameraFrame projection;projection.worldToView={0,-1,0,0,0,0,1,0,1,0,0,0,0,0,0,1};projection.worldToClip={0,-.5625f,0,0,0,0,1,0,1,0,0,-1,1,0,0,0};
 s=VisualSettings{};s.x=0;s.y=-58;auto nearBar=placeBar(projection,{200,0,45},1920,1080,s,180),farBar=placeBar(projection,{800,0,45},1920,1080,s,180);check(nearBar&&farBar&&std::abs(nearBar->scale/farBar->scale-4)<.001f);
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
