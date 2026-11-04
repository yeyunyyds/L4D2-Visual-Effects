#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <atomic>
namespace visual::diagnostics {
inline std::mutex mutex;
inline std::filesystem::path path;
inline std::atomic<bool> writable{false};
inline void write(const std::string& text) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex);
        if(path.empty()) {
            HMODULE module{};wchar_t name[32768]{};
            if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&write),&module)&&GetModuleFileNameW(module,name,32768)) {
                auto dir=std::filesystem::path(name).parent_path()/L"L4D2Visual";
                std::filesystem::create_directories(dir);path=dir/L"L4D2Visual.log";
            }
        }
        SYSTEMTIME t{};GetLocalTime(&t);
        std::ofstream file(path,std::ios::app|std::ios::binary);
        file<<t.wYear<<'-'<<t.wMonth<<'-'<<t.wDay<<' '<<t.wHour<<':'<<t.wMinute<<':'<<t.wSecond
            <<" pid="<<GetCurrentProcessId()<<" tick="<<GetTickCount64()<<' '<<text<<'\n';
        file.flush();writable=bool(file);
        OutputDebugStringA(("L4D2Visual: "+text+"\n").c_str());
    }catch(...){writable=false;OutputDebugStringA("L4D2Visual: file log write failed\n");}
}
}
