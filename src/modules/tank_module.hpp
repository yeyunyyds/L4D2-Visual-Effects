#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../tank_overlay.hpp"
#include "../tank_visual_menu.hpp"
#include "../tank_outline.hpp"
#include "../source_render.hpp"
namespace visual::tankfeature {
inline bool active(){return std::atomic_load(&tank::options)->enabled&&!tank::faulted;}
inline modules::Module create(){modules::Module m;m.id="tank";m.menuOrder=2;
 m.rootItem=[] {return ui::item(u8"Tank 行为预测",320);};
 m.page=[](std::uintptr_t id)->std::optional<std::string>{if(id==328)return tank::menu(id,tank::outline::status.load());if(id==320||id==322||id==323||id==324)return tank::menu(id);if(id==321)return tank::visualMenu();if(tank::finePage(id))return tank::visualFineMenu(id);return std::nullopt;};
 m.action=[](std::uintptr_t id){if(tank::visualAction(id)){ui::saveMessage=u8"Tank 样式未保存";return true;}if(tank::action(id)){ui::saveMessage=u8"Tank预测设置未保存";return true;}return false;};
 m.adjust=tank::visualAdjust;
 m.observing=active;m.observeModel=tank::observeModel;m.observeStudio=tank::observeStudio;
 m.needsCombat=active;m.needsEntities=active;
 m.frame=tank::onFrame;
 m.tick=tank::poll;m.pollInterval=[] {return std::atomic_load(&tank::options)->enabled?20u:200u;};
 m.load=[](const nlohmann::json& j){std::atomic_store(&tank::options,std::make_shared<const tank::Settings>(tank::readSettings(j)));std::atomic_store(&tank::visualSettings,std::make_shared<const tank::VisualSettings>(tank::readVisual(j)));};
 m.save=[](nlohmann::json& j){tank::writeSettings(j,*std::atomic_load(&tank::options));tank::writeVisual(j,*std::atomic_load(&tank::visualSettings));};
 m.pollOverlay=tank::pollDisplay;m.needsOverlay=tank::needed;m.drawOverlay=tank::draw;
 m.unavailable=[](const char* reason){tank::display::stage=reason;};
 m.previewKeys=tank::visualPreviewKeys;m.preview=tank::visualPreview;
 return m;
}
}
