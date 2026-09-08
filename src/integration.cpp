#include "integration.hpp"
#include <stdexcept>
#include <cstring>

namespace shaker {
static constexpr auto startTag="-- BEGIN PSVR2SimShaker";
static constexpr auto endTag="-- END PSVR2SimShaker";
std::string withoutHook(const std::string& text){
    auto result=text;
    for(;;){const auto start=result.find(startTag);if(start==std::string::npos)break;
        auto end=result.find(endTag,start);if(end==std::string::npos)throw std::runtime_error("Incomplete PSVR2SimShaker hook; repair manually before continuing");
        end+=std::strlen(endTag);if(end<result.size()&&result[end]=='\r')++end;if(end<result.size()&&result[end]=='\n')++end;
        result.erase(start,end-start);
    }return result;
}
bool integrationInstalled(const fs::path& p){
    try {
        if(!fs::is_regular_file(p/L"Scripts/PSVR2SimShaker/PSVR2SimShakerDcsBridge.dll") ||
           !fs::is_regular_file(p/L"Scripts/PSVR2SimShaker/Export.lua"))return false;
        const auto text=readText(p/L"Scripts/Export.lua");const auto start=text.find(startTag);
        if(start==std::string::npos)return false;
        const auto end=text.find(endTag,start),load=text.find("Scripts/PSVR2SimShaker/Export.lua",start);
        return end!=std::string::npos && load!=std::string::npos && load<end;
    }catch(...){return false;}
}
void installIntegration(const fs::path& p,const fs::path& source){
    if(!fs::is_directory(p)||!fs::exists(p/L"Config"))throw std::runtime_error("Select a DCS Saved Games profile");
    const auto script=source/L"dcs/Export.lua",bridge=source/L"PSVR2SimShakerDcsBridge.dll";
    if(!fs::exists(script)||!fs::exists(bridge))throw std::runtime_error("Installation package is incomplete");
    auto scripts=p/L"Scripts",dest=scripts/L"PSVR2SimShaker",exportFile=scripts/L"Export.lua";
    fs::create_directories(dest);std::string original=fs::exists(exportFile)?readText(exportFile):"";
    std::string updated=withoutHook(original);
    if(!updated.empty()&&updated.back()!='\n')updated+="\n";
    updated+="-- BEGIN PSVR2SimShaker\n";
    updated+="pcall(function() dofile(require('lfs').writedir() .. 'Scripts/PSVR2SimShaker/Export.lua') end)\n";
    updated+="-- END PSVR2SimShaker\n";
    if(fs::exists(exportFile)&&!fs::exists(scripts/L"Export.lua.before-PSVR2SimShaker"))fs::copy_file(exportFile,scripts/L"Export.lua.before-PSVR2SimShaker");
    // A loaded DLL is not replaced. Failure leaves the existing export chain intact.
    if(!fs::exists(dest/L"PSVR2SimShakerDcsBridge.dll")||sha256(bridge)!=sha256(dest/L"PSVR2SimShakerDcsBridge.dll"))
        fs::copy_file(bridge,dest/L"PSVR2SimShakerDcsBridge.dll",fs::copy_options::overwrite_existing);
    fs::copy_file(script,dest/L"Export.lua",fs::copy_options::overwrite_existing);
    writeTextAtomic(exportFile,updated);
}
void removeIntegration(const fs::path& p){
    auto file=p/L"Scripts/Export.lua";
    if(fs::exists(file))writeTextAtomic(file,withoutHook(readText(file)));
    // Remove only exact owned filenames, never recursively delete a user directory.
    std::error_code ec;
    fs::remove(p/L"Scripts/PSVR2SimShaker/Export.lua",ec);
    fs::remove(p/L"Scripts/PSVR2SimShaker/PSVR2SimShakerDcsBridge.dll",ec);
    fs::remove(p/L"Scripts/PSVR2SimShaker",ec);
}
}
