#pragma once
#include "image_renderer.hpp"
#include "entity_menu.hpp"
namespace visual::picturemenu {
inline std::vector<std::string> displayedFiles;
inline std::atomic<const char*> notice{u8"挂画操作已提交"};
inline std::string page(std::uintptr_t id){
 auto settings=std::atomic_load(&pictures::options);auto files=std::atomic_load(&pictures::assets);auto scene=std::atomic_load(&pictures::sceneFrame);
 if(id==151){displayedFiles=files->files;std::string body;
  for(unsigned i=0;i<displayedFiles.size();++i)body+=menuItem(displayedFiles[i]+(displayedFiles[i]==settings->selected?u8"（已选择）":""),0x70000+i);
  if(body.empty())body=menuRow(u8"目录暂无图片，请放入PNG/JPG/BMP后刷新");
  return menuPage(u8"选择本地图片",body+menuItem(u8"刷新目录（清空现有挂画）",201));
 }
 if(id==152)return menuPage(u8"长边 "+std::to_string(settings->size)+u8" / 角度 "+std::to_string(settings->rotation)+u8" / 透明度 "+std::to_string(settings->opacity*10)+"%",
  menuItem(u8"缩小 -20",205)+menuItem(u8"放大 +20",206)+menuItem(u8"逆时针 15度",207)+menuItem(u8"顺时针 15度",208)+
  menuItem(u8"透明度 -10%",209)+menuItem(u8"透明度 +10%",210)+menuItem(u8"恢复图片参数默认",211)+menuItem(u8"保存设置",101)+menuRow(u8"参数应用于之后放置的新图片"));
 std::string message=scene->message;
 if(settings->selected.empty()&&files->files.empty())message=files->message;
 auto error=files->errors.find(settings->selected);auto decoded=files->decoded.find(settings->selected);
 if(error!=files->errors.end())message=error->second;
 else if(!settings->selected.empty()&&decoded==files->decoded.end())message=settings->enabled?u8"图片等待后台加载":u8"开启挂画后加载所选图片";
 else if(decoded!=files->decoded.end())message=std::to_string(decoded->second->width)+"x"+std::to_string(decoded->second->height)+" / "+message;
 if(!scene->items.empty())message+=" / "+std::string(picturerender::status.load());
 return menuPage(u8"图片挂画（仅本机） "+std::to_string(scene->items.size())+"/8",
  menuItem(settings->enabled?u8"图片挂画：开":u8"图片挂画：关（默认）",200)+
  menuItem(u8"图片："+(settings->selected.empty()?std::string(u8"未选择"):settings->selected),151)+
  menuItem(u8"刷新目录（清空现有挂画）",201)+menuItem(u8"对准墙面放置所选图片",202)+menuItem(u8"大小、旋转与透明度",152)+
  menuItem(u8"删除最后一张",203)+menuItem(u8"清空当前地图挂画",204)+menuItem(u8"保存设置",101)+menuRow(message));
}
inline bool action(std::uintptr_t id){
 notice=u8"挂画操作已提交";
 if(id==201){pictures::clearRequested=true;pictures::refreshRequested=true;return true;}
 if(id==202){
  auto settings=std::atomic_load(&pictures::options);auto images=std::atomic_load(&pictures::assets);
  if(!settings->enabled){notice=u8"请先开启图片挂画";return true;}
  if(settings->selected.empty()){notice=u8"请先选择图片";return true;}
  if(!images->decoded.count(settings->selected)){notice=u8"图片尚未加载，请稍等或刷新";return true;}
  auto camera=std::atomic_load(&source_camera::snapshot);if(!dango::usableCamera(*camera,GetTickCount64())){notice=u8"请先进入地图再放置";return true;}
  pictures::requestPlace();return true;
 }
 if(id==203){pictures::undoRequested=true;return true;}
 if(id==204){pictures::clearRequested=true;return true;}
 bool file=id>=0x70000&&id<0x70000+128;
 if(id!=200&&!(id>=205&&id<=211)&&!file)return false;
 auto settings=*std::atomic_load(&pictures::options);
 if(file){auto index=std::size_t(id-0x70000);if(index>=displayedFiles.size())return true;settings.selected=displayedFiles[index];}
 if(id==200)settings.enabled=!settings.enabled;
 if(id==205)settings.size=unsigned(std::max(20,int(settings.size)-20));if(id==206)settings.size=std::min(512u,settings.size+20);
 if(id==207)settings.rotation=(settings.rotation+345)%360;if(id==208)settings.rotation=(settings.rotation+15)%360;
 if(id==209)settings.opacity=std::max(1u,settings.opacity-1);if(id==210)settings.opacity=std::min(10u,settings.opacity+1);
 if(id==211){settings.size=120;settings.rotation=0;settings.opacity=10;}
 notice=u8"挂画设置未保存";settings=pictures::normalized(std::move(settings));std::atomic_store(&pictures::options,std::make_shared<const pictures::Settings>(std::move(settings)));return true;
}
} // namespace visual::picturemenu
