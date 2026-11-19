#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../entity_transform_menu.hpp"
#include "../source_render.hpp"
namespace visual::deformationfeature {
inline modules::Module create(){modules::Module m;m.id="deformation";m.menuOrder=1;
 m.rootItem=[] {return ui::item(u8"实体缩放与拉伸",800);};
 m.page=[](std::uintptr_t id)->std::optional<std::string>{if(deform::isPage(id)){
  auto content=deform::page(id);
  if(id==800){
   // Navigation only; no dependency on another feature's implementation.
   auto last=content.rfind('}');
   if(last!=std::string::npos)content.insert(last,ui::item(u8"邦友的酒 · 设置",300));
  }
  return content;
 }return std::nullopt;};
 m.action=[](std::uintptr_t id){if(!deform::action(id))return false;ui::saveMessage=deform::notice;return true;};
 m.adjust=deform::adjust;
 m.needsEntities=[] {return deform::any(*std::atomic_load(&deform::options))||!deform::clean.load();};
 m.needsCompatibility=[] {return deform::live.load();};
 m.frame=[](const dango::GameCameraFrame& frame){auto camera=frame;if(!render::bound)camera.active=false;deform::onFrame(camera,render::boneLimit);};
 m.load=[](const nlohmann::json& j){std::atomic_store(&deform::options,std::make_shared<const deform::Settings>(deform::readSettings(j)));};
 m.save=[](nlohmann::json& j){deform::writeSettings(j,*std::atomic_load(&deform::options));};
 m.modifying=[] {return deform::live.load();};m.modifyPose=deform::studio;
 m.quiesce=[] {deform::stop();};return m;
}
}
