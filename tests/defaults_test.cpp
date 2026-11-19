#include "tank_model.hpp"
#include "rock_settings.hpp"
#include "entity_transform.hpp"
#include "tank_visual.hpp"
#include "proximity.hpp"
#include "entity_selection.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("Release defaults regression");}
int main(){using nlohmann::json;std::ifstream input("defaults/hud.default.json");json j;input>>j;
 for(const auto& config:{json::object(),j}){auto rock=visual::rocks::readSettings(config);check(!rock.enabled&&rock.trajectory&&rock.landingCircle&&rock.horizonMs==500);auto tank=visual::tank::readSettings(config);auto style=visual::tank::readVisual(config);check(!tank.enabled&&tank.draw&&!tank.warnBar&&tank.warnRange&&!tank.warnFill&&!tank.warnOutline);check(!visual::deform::readSettings(config).enabled);check(style.showBar&&style.showDirection&&style.showRange&&style.showTarget);check(style.preset==2&&style.vertical&&style.x==-32&&style.y==-24&&std::abs(style.barSize-.55f)<.001f&&style.arrowWidth==3&&style.rangeWidth==3);}
 json legacy={{"tankPrediction",{{"logging",true},{"markKey",true}}}};auto clean=visual::tank::readSettings(legacy);visual::tank::writeSettings(legacy,clean);check(!legacy["tankPrediction"].contains("logging")&&!legacy["tankPrediction"].contains("markKey"));
 check(!visual::EntitySelection{}.anyEnabled());check(!visual::ProximitySettings{}.enabled&&!visual::ProximitySettings{}.includeCommon);check(j["f10"]==false&&j["entitySelection"]["all"]==false&&!j.contains("sprayP2P")&&!j.contains("wallPictures"));std::cout<<"PASS: compiled and packaged defaults match supplied style with Tank master off, display preferences on, other features off\n";
}
