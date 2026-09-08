#pragma once
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <vector>

namespace shaker {
namespace fs=std::filesystem;
std::string utf8(const std::wstring& text);
std::wstring wide(const std::string& text);
fs::path appDirectory();
fs::path dataDirectory();
std::string readText(const fs::path& file);
void writeTextAtomic(const fs::path& file,const std::string& text);
std::string sha256(const fs::path& file);
std::vector<fs::path> dcsProfiles();
fs::path dcsInstall();
fs::path toolkitFile();
// Upgrade cleanup only; Windows startup is no longer offered.
void removeLegacyStartup();
bool spawnHidden(const std::wstring& args,HANDLE* process=nullptr);
void openPath(const fs::path& path);
}
