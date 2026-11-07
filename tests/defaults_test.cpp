#include "tank_model.hpp"
#include "tank_visual.hpp"
#include "spray_protocol.hpp"
#include "proximity.hpp"
#include "entity_selection.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("Release defaults regression");}
int main(){using nlohmann::json;std::ifstream input("defaults/hud.default.json");json j;input>>j;
 for(const auto& config:{json::object(),j}){auto tank=visual::tank::readSettings(config);auto pictures=visual::pictures::readSettings(config);auto p2p=visual::spraysync::readSettings(config);auto style=visual::tank::readVisual(config);check(!tank.enabled&&!tank.draw&&!tank.logging&&!tank.markKey);check(!pictures.enabled&&!pictures.bindT&&!pictures.autoNext);check(!p2p.enabled&&!p2p.automatic);check(!style.showBar&&!style.showDirection&&!style.showRange&&!style.showTarget);check(style.preset==2&&style.vertical&&style.x==-32&&style.y==-24&&std::abs(style.barSize-.55f)<.001f&&style.arrowWidth==3&&style.rangeWidth==3);}
 check(!visual::EntitySelection{}.anyEnabled());check(!visual::ProximitySettings{}.enabled&&!visual::ProximitySettings{}.includeCommon);check(j["f10"]==false&&j["entitySelection"]["all"]==false&&j["sprayP2P"]["peers"].empty()&&j["wallPictures"]["selected"]=="");std::cout<<"PASS: compiled and packaged defaults match supplied style with all feature switches off\n";
}
