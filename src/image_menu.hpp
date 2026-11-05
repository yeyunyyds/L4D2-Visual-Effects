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
  return menuPage(u8"选择本地图片",body+menuItem(u8"刷新目录（清空自己的喷漆）",201));
 }
 if(id==152)return menuPage(u8"长边 "+std::to_string(pictures::effectiveSize(*settings,settings->selected))+u8" / 角度 "+std::to_string(settings->rotation)+u8" / 透明度 "+std::to_string(settings->opacity*10)+"%",
  menuItem(u8"缩小 -10",205)+menuItem(u8"放大 +10",206)+menuItem(u8"逆时针 15度",207)+menuItem(u8"顺时针 15度",208)+
  menuItem(u8"透明度 -10%",209)+menuItem(u8"透明度 +10%",210)+menuItem(u8"仅应用当前图片大小",217)+menuItem(u8"当前大小应用所有图片",216)+menuItem(u8"恢复图片参数默认",211)+menuItem(u8"保存设置",101)+menuRow(u8"尺寸立即应用于墙上同名图片"));
 if(id==153){auto network=std::atomic_load(&spraysync::options);return menuPage(u8"联机喷漆 · Steam P2P 实验",
  menuItem(network->enabled?u8"联机喷漆：开":u8"联机喷漆：关（默认）",218)+
  menuRow(u8"本机 SteamID64："+std::to_string(spraysync::ownId.load()))+menuRow(*std::atomic_load(&spraysync::status))+
  menuRow(u8"配置 hud.json 的 sprayP2P.room 与 peers")+menuRow(u8"双方同地图、同房间、互填 SteamID，均需本插件")+menuItem(u8"保存设置",101));}
 if(id==154)return menuPage(u8"自定义喷漆 · 操作设置",
  menuItem(settings->bindT?u8"绑定 T 键：开":u8"绑定 T 键：关（默认）",212)+
  menuItem(settings->autoNext?u8"喷漆后自动下一张：开":u8"喷漆后自动下一张：关（默认）",213)+
  menuRow(u8"每人最大数量："+std::to_string(settings->maximum))+menuItem(u8"数量上限 -1",214)+menuItem(u8"数量上限 +1",215)+
  menuItem(u8"保存设置",101)+menuRow(u8"超出上限自动删除最早一张"));
 std::string message=scene->message;
 if(settings->selected.empty()&&files->files.empty())message=files->message;
 auto error=files->errors.find(settings->selected);auto decoded=files->decoded.find(settings->selected);
 if(error!=files->errors.end())message=error->second;
 else if(!settings->selected.empty()&&decoded==files->decoded.end())message=settings->enabled?u8"图片等待后台加载":u8"开启挂画后加载所选图片";
 else if(decoded!=files->decoded.end())message=std::to_string(decoded->second->width)+"x"+std::to_string(decoded->second->height)+" / "+message;
 if(!scene->items.empty())message+=" / "+std::string(picturerender::status.load());
 return menuPage(u8"自定义喷漆 · 当前 "+std::to_string(scene->items.size())+u8" 张",
  menuItem(settings->enabled?u8"自定义喷漆：开":u8"自定义喷漆：关（默认）",200)+
  menuItem(u8"图片："+(settings->selected.empty()?std::string(u8"未选择"):settings->selected),151)+
  menuItem(u8"刷新目录（清空自己的喷漆）",201)+menuItem(u8"对准墙面放置所选图片",202)+menuItem(u8"大小、旋转与透明度",152)+menuItem(u8"按键、切图与数量限制",154)+menuItem(u8"联机喷漆（实验）",153)+
  menuItem(u8"删除自己的最后一张",203)+menuItem(u8"清空自己的喷漆",204)+menuItem(u8"保存设置",101)+menuRow(message));
}
inline bool action(std::uintptr_t id){
 notice=u8"挂画操作已提交";
 if(id==218){auto settings=*std::atomic_load(&spraysync::options);settings.enabled=!settings.enabled;std::atomic_store(&spraysync::options,std::make_shared<const spraysync::Settings>(std::move(settings)));notice=u8"联机喷漆设置未保存";return true;}
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
 if(id!=200&&!(id>=205&&id<=217)&&!file)return false;
 auto settings=*std::atomic_load(&pictures::options);
 if(file){auto index=std::size_t(id-0x70000);if(index>=displayedFiles.size())return true;settings.selected=displayedFiles[index];}
 if(id==200)settings.enabled=!settings.enabled;
 if(id==205||id==206){auto value=pictures::effectiveSize(settings,settings.selected);value=unsigned(std::clamp(int(value)+(id==205?-10:10),10,512));if(settings.selected.empty())settings.size=value;else settings.sizes[settings.selected]=value;}
 if(id==212)settings.bindT=!settings.bindT;if(id==213)settings.autoNext=!settings.autoNext;
 if(id==214)settings.maximum=std::max(1u,settings.maximum-1);if(id==215)settings.maximum=std::min(pictures::MaxPictures,settings.maximum+1);
 if(id==216){settings.size=pictures::effectiveSize(settings,settings.selected);settings.sizes.clear();}
 if(id==217&&!settings.selected.empty())settings.sizes[settings.selected]=pictures::effectiveSize(settings,settings.selected);
 if(id==207)settings.rotation=(settings.rotation+345)%360;if(id==208)settings.rotation=(settings.rotation+15)%360;
 if(id==209)settings.opacity=std::max(1u,settings.opacity-1);if(id==210)settings.opacity=std::min(10u,settings.opacity+1);
 if(id==211){if(settings.selected.empty())settings.size=60;else settings.sizes[settings.selected]=60;settings.rotation=0;settings.opacity=10;}
 notice=u8"挂画设置未保存";settings=pictures::normalized(std::move(settings));std::atomic_store(&pictures::options,std::make_shared<const pictures::Settings>(std::move(settings)));return true;
}
} // namespace visual::picturemenu
