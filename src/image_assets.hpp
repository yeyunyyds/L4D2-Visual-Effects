#pragma once
#include <windows.h>
#include <wincodec.h>
#include <filesystem>
#include <atomic>
#include <map>
#include <set>
#include "image_scene.hpp"
#include "dango/source_camera.hpp"
namespace visual::pictures {
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::shared_ptr<const std::vector<std::string>> demand=std::make_shared<const std::vector<std::string>>();
inline std::atomic<bool> refreshRequested{true};
inline std::filesystem::path folder;
struct Assets {
 std::uint64_t generation=0;std::vector<std::string> files;
 std::map<std::string,std::shared_ptr<const Pixels>> decoded;
 std::map<std::string,std::string> errors;std::string message=u8"图片目录等待刷新";
};
inline std::shared_ptr<const Assets> assets=std::make_shared<const Assets>();
template<class T> struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T** out(){return &p;}};
inline thread_local const char* decodeStage="not started";
inline std::shared_ptr<const Pixels> decode(const std::filesystem::path& path,std::uint64_t revision)try{
 decodeStage="COM/WIC setup";
 struct Apartment {HRESULT result=CoInitializeEx(nullptr,COINIT_MULTITHREADED);~Apartment(){if(SUCCEEDED(result))CoUninitialize();}};
 static thread_local Apartment apartment;
 if(FAILED(apartment.result)&&apartment.result!=RPC_E_CHANGED_MODE)return {};
 Com<IWICImagingFactory> factory;Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;Com<IWICBitmapScaler> scaler;Com<IWICFormatConverter> converter;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.out())))||
    FAILED(factory.p->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,decoder.out()))||
    FAILED(decoder.p->GetFrame(0,frame.out())))return {};
 decodeStage="source size";
 UINT width=0,height=0;if(FAILED(frame.p->GetSize(&width,&height))||!width||!height||width>32768||height>32768||std::uint64_t(width)*height>32u*1024u*1024u)return {};
 auto output=std::make_shared<Pixels>();double scale=std::min(1.,double(MaxEdge)/std::max(width,height));
 output->width=std::max(1u,unsigned(width*scale));output->height=std::max(1u,unsigned(height*scale));output->revision=revision;
 decodeStage="image scaling";
 IWICBitmapSource* source=frame.p;
 if(output->width!=width||output->height!=height){
  if(FAILED(factory.p->CreateBitmapScaler(scaler.out()))||FAILED(scaler.p->Initialize(frame.p,output->width,output->height,WICBitmapInterpolationModeFant)))return {};
  source=scaler.p;
 }
 decodeStage="BGRA conversion";
 if(FAILED(factory.p->CreateFormatConverter(converter.out()))||FAILED(converter.p->Initialize(source,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return {};
 decodeStage="pixel copy";
 output->bgra.resize(std::size_t(output->width)*output->height*4);
 if(FAILED(converter.p->CopyPixels(nullptr,output->width*4,UINT(output->bgra.size()),output->bgra.data())))return {};
 return output;
}catch(...){return {};}
// Called exclusively by the existing background worker. No file IO/decoding in drawing callbacks.
inline void pollAssets(){
 if(folder.empty())return;
 static Assets state;static std::set<std::string> previousDemand,budgetErrors;static std::uint64_t revision=0;
 bool changed=false;
 try{
  if(refreshRequested.exchange(false)){
   ++state.generation;state.files.clear();state.decoded.clear();state.errors.clear();budgetErrors.clear();previousDemand.clear();
   std::error_code error;std::filesystem::create_directories(folder,error);
   if(error){state.message=u8"无法创建图片目录";changed=true;}
   else{
    std::filesystem::directory_iterator it(folder,error),end;
    for(unsigned visited=0;it!=end&&!error&&visited<1024;it.increment(error),++visited){
     if(it->is_regular_file(error)){auto key=it->path().filename().u8string();if(imageKey(key))state.files.push_back(std::move(key));}
    }
    std::sort(state.files.begin(),state.files.end());if(state.files.size()>128)state.files.resize(128);
    state.message=error?u8"图片目录读取不完整":u8"图片目录已刷新";changed=true;
   }
  }
  auto settings=std::atomic_load(&options);std::set<std::string> needed;
  if(settings->enabled){auto live=std::atomic_load(&demand);for(const auto& key:*live)if(imageKey(key))needed.insert(key);if(imageKey(settings->selected))needed.insert(settings->selected);}
  if(needed!=previousDemand){for(const auto& key:budgetErrors)state.errors.erase(key);budgetErrors.clear();previousDemand=needed;changed=true;}
  for(auto i=state.decoded.begin();i!=state.decoded.end();)if(!needed.count(i->first)){i=state.decoded.erase(i);changed=true;}else ++i;
  std::size_t used=0;for(const auto& p:state.decoded)used+=p.second->bgra.size();
  for(const auto& key:needed){
   if(state.decoded.count(key)||state.errors.count(key))continue;
   if(std::find(state.files.begin(),state.files.end(),key)==state.files.end()){state.errors[key]=u8"未找到图片，请刷新目录";changed=true;continue;}
   auto path=folder/std::filesystem::u8path(key);std::error_code error;auto bytes=std::filesystem::file_size(path,error);
   if(error||bytes>32u*1024u*1024u){state.errors[key]=u8"图片读取失败或文件超过32MB";changed=true;continue;}
   auto pixels=decode(path,++revision);
   if(!pixels){state.errors[key]=u8"图片解码失败或尺寸过大";source_camera::log("Wall picture decode failed: "+key+" stage="+decodeStage);}
   else if(pixels->bgra.size()>CacheBudget-used){state.errors[key]=u8"图片缓存已满，请删除旧挂画";budgetErrors.insert(key);}
   else{used+=pixels->bgra.size();state.decoded[key]=std::move(pixels);}
   changed=true;
  }
 }catch(const std::exception&){state.message=u8"图片目录处理失败，请刷新重试";changed=true;}
 if(changed)std::atomic_store(&assets,std::make_shared<const Assets>(state));
}
} // namespace visual::pictures
