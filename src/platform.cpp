#include "platform.hpp"
#include <shlobj.h>
#include <bcrypt.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

namespace shaker {
std::string utf8(const std::wstring& s) {
    if(s.empty())return {};
    int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);
    std::string result(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),result.data(),n,nullptr,nullptr);return result;
}
std::wstring wide(const std::string& s) {
    if(s.empty())return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);
    if(!n)throw std::runtime_error("Invalid UTF-8 path");
    std::wstring result(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),result.data(),n);return result;
}
fs::path appDirectory(){wchar_t p[32768]{};GetModuleFileNameW(nullptr,p,32768);return fs::path(p).parent_path();}
static fs::path known(REFKNOWNFOLDERID id){PWSTR p=nullptr; if(FAILED(SHGetKnownFolderPath(id,0,nullptr,&p)))throw std::runtime_error("Windows folder unavailable");fs::path result(p);CoTaskMemFree(p);return result;}
fs::path dataDirectory(){auto p=known(FOLDERID_LocalAppData)/L"PSVR2SimShaker";fs::create_directories(p);return p;}
std::string readText(const fs::path& f){std::ifstream in(f,std::ios::binary);if(!in)throw std::runtime_error("Cannot read "+utf8(f.wstring()));return {std::istreambuf_iterator<char>(in),{}};}
void writeTextAtomic(const fs::path& f,const std::string& s){
    fs::create_directories(f.parent_path());auto temp=f;temp+=L".tmp";
    {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write(s.data(),std::streamsize(s.size()));out.flush();if(!out)throw std::runtime_error("Unable to save file");}
    if(!MoveFileExW(temp.c_str(),f.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Unable to replace saved file");
}
std::string sha256(const fs::path& f){
    std::string data=readText(f); BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
    DWORD size=0,n=0;BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&n,0);
    std::vector<unsigned char> object(size);unsigned char digest[32]{};
    bool ok=BCryptCreateHash(alg,&hash,object.data(),size,nullptr,0,0)>=0 &&
        BCryptHashData(hash,reinterpret_cast<PUCHAR>(data.data()),ULONG(data.size()),0)>=0 && BCryptFinishHash(hash,digest,32,0)>=0;
    if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
    if(!ok)throw std::runtime_error("SHA256 failed");std::ostringstream out;for(auto b:digest)out<<std::hex<<std::setfill('0')<<std::setw(2)<<int(b);return out.str();
}
std::vector<fs::path> dcsProfiles(){
    std::vector<fs::path> result;std::error_code ec;auto saved=known(FOLDERID_SavedGames);
    for(auto& d:fs::directory_iterator(saved,ec))if(d.is_directory() && d.path().filename().wstring().rfind(L"DCS",0)==0 && fs::exists(d.path()/L"Config"))result.push_back(d.path());
    std::sort(result.begin(),result.end());return result;
}
fs::path dcsInstall(){
    for(auto key:{L"Software\\Eagle Dynamics\\DCS World",L"Software\\Eagle Dynamics\\DCS World OpenBeta"}) {
        wchar_t buffer[32768]{};DWORD n=sizeof(buffer);
        if(RegGetValueW(HKEY_CURRENT_USER,key,L"Path",RRF_RT_REG_SZ,nullptr,buffer,&n)==ERROR_SUCCESS && fs::exists(fs::path(buffer)/L"bin/DCS.exe"))return buffer;
    }return {};
}
fs::path toolkitFile(){
    wchar_t tmp[32768]{};GetTempPathW(32768,tmp);
    auto hint=fs::path(tmp)/L"psvr2tk_capi_path.txt";
    try {if(fs::exists(hint)){auto s=readText(hint);while(!s.empty()&&(s.back()=='\n'||s.back()=='\r'))s.pop_back();auto p=fs::path(wide(s))/L"psvr2_toolkit_capi.dll";if(fs::is_regular_file(p))return p;}}catch(...){}
    wchar_t steam[32768]{};DWORD n=sizeof(steam);
    if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath",RRF_RT_REG_SZ,nullptr,steam,&n)==ERROR_SUCCESS) {
        auto p=fs::path(steam)/L"steamapps/common/PlayStation VR2 App/SteamVR_Plug-In/bin/win64/psvr2_toolkit_capi.dll";
        if(fs::exists(p))return p;
    }return {};
}
void removeLegacyStartup(){
    HKEY key=nullptr;
    const auto opened=RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key);
    if(opened==ERROR_FILE_NOT_FOUND)return;
    if(opened!=ERROR_SUCCESS)throw std::runtime_error("Could not remove the old SimShaker Windows startup entry");
    const auto result=RegDeleteValueW(key,L"PSVR2SimShaker");RegCloseKey(key);
    if(result!=ERROR_SUCCESS&&result!=ERROR_FILE_NOT_FOUND)throw std::runtime_error("Could not remove the old SimShaker Windows startup entry");
}
bool spawnHidden(const std::wstring& args,HANDLE* process){
    wchar_t executable[32768]{};if(!GetModuleFileNameW(nullptr,executable,32768))return false;
    fs::path exe=executable;std::wstring command=L"\""+exe.wstring()+L"\" "+args;
    STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;PROCESS_INFORMATION pi{};
    if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,appDirectory().c_str(),&si,&pi))return false;
    CloseHandle(pi.hThread);if(process)*process=pi.hProcess;else CloseHandle(pi.hProcess);return true;
}
void openPath(const fs::path& path){ShellExecuteW(nullptr,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
}
