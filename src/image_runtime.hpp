#pragma once
#include "image_assets.hpp"
#include "image_trace.hpp"
namespace visual::pictures {
struct Frame {std::uint32_t epoch=0;std::uint64_t tick=0;std::vector<Placement> items;std::string message=u8"图片挂画关闭";};
inline std::shared_ptr<const Frame> sceneFrame=std::make_shared<const Frame>();
struct PlaceRequest {std::uint32_t epoch=0;std::uint64_t tick=0;Settings settings;};
inline std::shared_ptr<const PlaceRequest> placeRequest=std::make_shared<const PlaceRequest>();
inline std::atomic<bool> clearRequested{false},undoRequested{false};
inline void requestPlace(){auto camera=std::atomic_load(&source_camera::snapshot);auto settings=std::atomic_load(&options);
 std::atomic_store(&placeRequest,std::make_shared<const PlaceRequest>(PlaceRequest{camera->epoch,GetTickCount64(),*settings}));}
inline void onFrame(const dango::GameCameraFrame& camera)noexcept{
 static Scene scene;static bool wasEnabled=false;static std::uint64_t consumed=0,nextVisibility=0;static unsigned cursor=0;static std::string message=u8"图片挂画关闭";
 static std::shared_ptr<const Settings> previous;bool changed=false;
 try{
  auto settings=std::atomic_load(&options);if(previous!=settings){previous=settings;changed=true;}
  if(scene.epoch!=camera.epoch){scene.observe(camera.epoch);picturetrace::reset();nextVisibility=0;changed=true;}
  bool clear=clearRequested.exchange(false),undo=undoRequested.exchange(false);
  if(clear){scene.clear();message=u8"挂画已清空";changed=true;}
  if(undo){scene.undo();message=u8"已删除最后一张";changed=true;}
  auto request=std::atomic_load(&placeRequest);
  if(!settings->enabled||!camera.active){
   if(wasEnabled||!scene.items.empty()){scene.clear();picturetrace::reset();changed=true;}
   wasEnabled=false;consumed=request->tick;
   if(!settings->enabled)message=u8"图片挂画关闭";
  }else{
   if(!wasEnabled){picturetrace::reset();wasEnabled=true;message=u8"选择图片后对准墙面放置";changed=true;}
   dango::CameraPose eye;
   if(dango::cameraPose(camera,eye)){
    if(!picturetrace::object&&!picturetrace::faulted){if(!picturetrace::connect()){picturetrace::faulted=true;message=u8"墙面接口校验失败，请查看日志";source_camera::log("Picture TraceRay profile mismatch; no trace dispatched");changed=true;}}
    if(request->tick!=consumed){
     consumed=request->tick;changed=true;auto decoded=std::atomic_load(&assets);auto pixels=decoded->decoded.find(request->settings.selected);
     if(request->epoch!=camera.epoch||camera.tick<request->tick||camera.tick-request->tick>2000)message=u8"放置请求已过期，请重新对准墙面";
     else if(picturetrace::faulted)message=u8"墙面接口不可用，请关闭后重开";
     else if(!request->settings.enabled||pixels==decoded->decoded.end())message=u8"图片尚未加载，请选择图片并稍等";
     else if(scene.items.size()>=MaxPictures)message=u8"挂画已达8张，请先删除";
     else{
      dango::trace::Result hit;
      if(picturetrace::cast(eye.eye,eye.eye+eye.forward*1024,hit)&&dango::trace::wallHit(hit)&&
         scene.place(request->settings,pixels->second->width,pixels->second->height,hit.end,hit.plane.normal,picturetrace::cast)){
       message=u8"图片已放置（仅本机可见）";nextVisibility=0;source_camera::log("Local wall picture placed: "+request->settings.selected+" count="+std::to_string(scene.items.size()));
      }else message=u8"未找到足够平整的墙面，请调整位置或缩小图片";
     }
    }
    if(!picturetrace::faulted&&camera.tick>=nextVisibility&&!scene.items.empty()){
     nextVisibility=camera.tick+50;unsigned count=std::min<unsigned>(2,unsigned(scene.items.size()));
     for(unsigned i=0;i<count;++i){auto& item=scene.items[cursor++%scene.items.size()];item.visible=dango::trace::visible(item.panel,eye.eye,picturetrace::cast);item.visibleTick=camera.tick;}
     changed=true;
    }
    if(picturetrace::faulted){
     for(auto& item:scene.items){changed|=item.visible;item.visible=false;}
     if(message!=u8"墙面接口失效，请关闭后重开"){message=u8"墙面接口失效，请关闭后重开";changed=true;}
    }
   }
  }
  if(changed){Frame state;state.epoch=camera.epoch;state.tick=camera.tick;state.items=scene.items;state.message=message;
   std::vector<std::string> needed;for(const auto& item:scene.items)needed.push_back(item.key);
   std::atomic_store(&demand,std::make_shared<const std::vector<std::string>>(std::move(needed)));
   std::atomic_store(&sceneFrame,std::make_shared<const Frame>(std::move(state)));
  }
 }catch(...){scene.clear();picturetrace::faulted=true;Frame f;f.epoch=camera.epoch;f.tick=camera.tick;f.message=u8"挂画处理异常，请关闭后重开";std::atomic_store(&sceneFrame,std::make_shared<const Frame>(std::move(f)));}
}
} // namespace visual::pictures
