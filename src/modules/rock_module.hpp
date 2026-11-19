#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../rock_ui.hpp"
namespace visual::rockfeature {
inline modules::Module create(){modules::Module m;m.id="rocks";m.menuOrder=3;
 m.rootItem=[] {return ui::item(rockui::title(),700);};
 m.page=[](std::uintptr_t id)->std::optional<std::string>{if(id==700||id==701)return rockui::menu(id);return std::nullopt;};
 m.action=[](std::uintptr_t id){if(!rockui::action(id))return false;ui::saveMessage=u8"石头预测设置未保存";return true;};
 m.needsEntities=rockruntime::active;m.frame=rockruntime::onFrame;
 m.load=[](const nlohmann::json& j){rockui::readSettings(j);};m.save=[](nlohmann::json& j){rockui::writeSettings(j);};
 m.pollOverlay=rockui::pollDisplay;m.needsNativeOverlay=rockui::needed;m.drawNativeOverlay=rockui::draw;return m;
}
}
