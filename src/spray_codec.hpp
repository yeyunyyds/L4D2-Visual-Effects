#pragma once
#include "image_assets.hpp"
#include "spray_protocol.hpp"
#include <wincrypt.h>
namespace visual::spraysync {
inline std::string digest(const std::vector<unsigned char>& bytes){HCRYPTPROV provider=0;HCRYPTHASH hash=0;
 if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))throw std::runtime_error("SHA256 provider unavailable");
 struct Cleanup{HCRYPTPROV p;HCRYPTHASH& h;~Cleanup(){if(h)CryptDestroyHash(h);CryptReleaseContext(p,0);}}cleanup{provider,hash};
 unsigned char result[32];DWORD size=32;
 if(!CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)||!CryptHashData(hash,bytes.data(),DWORD(bytes.size()),0)||!CryptGetHashParam(hash,HP_HASHVAL,result,&size,0)||size!=32)throw std::runtime_error("SHA256 failed");return hex(result,32);
}
// Re-encode decoded pixels: filenames and source image metadata never leave this client.
inline std::vector<unsigned char> png(const pictures::Pixels& pixels){using pictures::Com;
 Com<IWICImagingFactory> factory;Com<IWICBitmapEncoder> encoder;Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> properties;Com<IStream> stream;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.out())))||FAILED(CreateStreamOnHGlobal(nullptr,TRUE,stream.out()))||FAILED(factory.p->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.out()))||FAILED(encoder.p->Initialize(stream.p,WICBitmapEncoderNoCache))||FAILED(encoder.p->CreateNewFrame(frame.out(),properties.out()))||FAILED(frame.p->Initialize(properties.p))||FAILED(frame.p->SetSize(pixels.width,pixels.height)))throw std::runtime_error("PNG encoder unavailable");
 WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;if(FAILED(frame.p->SetPixelFormat(&format))||!IsEqualGUID(format,GUID_WICPixelFormat32bppBGRA))throw std::runtime_error("PNG format unavailable");
 auto straight=pixels.bgra;for(std::size_t i=0;i<straight.size();i+=4)for(unsigned c=0;c<3;++c)straight[i+c]=straight[i+3]?static_cast<unsigned char>(std::min(255u,(unsigned(straight[i+c])*255+straight[i+3]/2)/straight[i+3])):0;
 if(FAILED(frame.p->WritePixels(pixels.height,pixels.width*4,UINT(straight.size()),straight.data()))||FAILED(frame.p->Commit())||FAILED(encoder.p->Commit()))throw std::runtime_error("PNG encoding failed");
 STATSTG stat{};if(FAILED(stream.p->Stat(&stat,STATFLAG_NONAME))||stat.cbSize.QuadPart>20u*1024u*1024u)throw std::runtime_error("PNG transfer limit exceeded");
 LARGE_INTEGER zero{};if(FAILED(stream.p->Seek(zero,STREAM_SEEK_SET,nullptr)))throw std::runtime_error("PNG stream seek failed");std::vector<unsigned char> result(std::size_t(stat.cbSize.QuadPart));ULONG read=0;
 if(FAILED(stream.p->Read(result.data(),ULONG(result.size()),&read))||read!=result.size())throw std::runtime_error("PNG stream read failed");return result;
}
}
