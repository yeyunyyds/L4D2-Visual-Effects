#pragma once
#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <mutex>
#include <vector>
#include <filesystem>
#include "dango/source_camera.hpp"
namespace visual {
inline bool verifiedModuleFile(HMODULE module,const std::string& expected) {
 static std::mutex mutex;static std::vector<std::pair<HMODULE,std::string>> verified;
 std::lock_guard<std::mutex> lock(mutex);
 if(std::find(verified.begin(),verified.end(),std::make_pair(module,expected))!=verified.end())return true;
 wchar_t path[32768]{};
 if(!GetModuleFileNameW(module,path,32768))return false;
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return false;
 HCRYPTPROV provider{};HCRYPTHASH hash{};
 bool ok=CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash);
 BYTE buffer[65536];DWORD count{};
 while(ok){if(!ReadFile(file,buffer,sizeof(buffer),&count,nullptr)){ok=false;break;}if(!count)break;ok=CryptHashData(hash,buffer,count,0)!=FALSE;}
 BYTE digest[32];DWORD size=sizeof(digest);if(ok)ok=CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)!=FALSE&&size==32;
 std::string actual;if(ok){const char* hex="0123456789abcdef";for(auto b:digest){actual+=hex[b>>4];actual+=hex[b&15];}}
 if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);CloseHandle(file);
 ok=ok&&actual==expected;
 source_camera::log(std::string("Render disk SHA256 ")+(ok?"match: ":"FAILED: ")+std::filesystem::path(path).filename().u8string()+" actual="+actual);
 if(ok)verified.emplace_back(module,expected);return ok;
}
}
