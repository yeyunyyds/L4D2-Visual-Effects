#pragma once
#include "modules/deformation_module.hpp"
#include "modules/tank_module.hpp"
#include "modules/rock_module.hpp"
#include "modules/animation_module.hpp"
#include "modules/first_person_body_module.hpp"
#include "modules/free_camera_module.hpp"
namespace visual::features {
inline modules::Host& host(){static modules::Host instance;return instance;}
inline void prepareRender(){
 auto& h=host();h.prepare();
 render::modelObserver=h.observing()?+[](std::uint32_t header,int index,const dango::Vec3& origin){host().observeModel(header,index,origin);}:nullptr;
 render::studioObserver=h.observing()?+[](std::uint32_t header,std::uint32_t entity,const dango::Vec3& origin){host().observeStudio(header,entity,origin);}:nullptr;
}
inline void finishRender(){render::poseModifier=host().modifying()?+[](std::uint32_t header,std::uint32_t entity,const Matrix3x4* input,const dango::Vec3& origin,Matrix3x4* output,unsigned capacity,unsigned& count){return host().modifyPose(header,entity,input,origin,output,capacity,count);}:nullptr;}
inline void disconnectRender(){render::modelObserver=nullptr;render::studioObserver=nullptr;render::poseModifier=nullptr;render::studioDecorator=nullptr;render::drawSuppressor=nullptr;}
inline void install(){
 auto& h=host();modules::Module aim;aim.id="aim";aim.needsEntities=aimruntime::hasPending;aim.frame=aimruntime::onFrame;aim.quiesce=aimruntime::stop;
 // Preserve the original game-thread order. Menu order is separately declared.
 h.add(std::move(aim));
 h.add(deformationfeature::create());
 h.add(tankfeature::create());
 h.add(rockfeature::create());
 h.add(animationfeature::create());
 h.add(firstpersonfeature::create());
 h.add(freecamerafeature::create());
 h.freeze();
 // Source Studio remains shared infrastructure, with a generic render callback.
 render::drawSuppressor=+[](std::uint32_t renderable){return freecamera::viewmodel::shouldHide(renderable);};
 render::studioDecorator=+[](const void* info,Matrix3x4* bones,const dango::Vec3& origin,void* data,render::StudioNext next){
  host().decorateStudio(info,bones,origin,data,next);
 };
 hudruntime::configureModules(h);
 hudruntime::hud.configureOverlay([] {return host().needsNativeOverlay();},[](float w,float h){host().drawNativeOverlay(w,h);});
 hudruntime::hud.configureAddon([](std::uintptr_t page,float w,float h,bool focused){host().preview(page,w,h,focused);},[](std::uintptr_t page){return host().previewKeys(page);});
 hudruntime::hud.configureAdjustment([](std::uintptr_t page,int delta){return host().adjust(page,delta);});
}
}
