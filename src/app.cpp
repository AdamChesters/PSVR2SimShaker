#include "app.hpp"
#include "telemetry.hpp"
#include "haptics.hpp"
#include "integration.hpp"
#include "version.hpp"
#include <imgui.h>
#include <commdlg.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace shaker {
namespace {
fs::path chooseFile(bool save,const wchar_t* filter,const wchar_t* extension){
    wchar_t file[32768]{};OPENFILENAMEW of{sizeof(of)};of.lpstrFile=file;of.nMaxFile=32768;
    of.lpstrFilter=filter;of.lpstrDefExt=extension;of.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    return (save?GetSaveFileNameW(&of):GetOpenFileNameW(&of))?fs::path(file):fs::path{};
}
void mutedText(const char* s){
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s",s);ImGui::PopStyleColor();
}
void continueRow(const char* nextLabel,float fixedWidth=0){
    const float width=fixedWidth>0?fixedWidth:ImGui::CalcTextSize(nextLabel).x+2*ImGui::GetStyle().FramePadding.x;
    const float right=ImGui::GetCursorScreenPos().x+ImGui::GetContentRegionAvail().x;
    if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=right)ImGui::SameLine();
}
template<class Draw> bool labeledControl(const char* label,Draw draw){
    bool changed=false;ImGui::PushID(label);
    if(ImGui::BeginTable("ControlRow",2,ImGuiTableFlags_SizingStretchProp)){
        ImGui::TableSetupColumn("Label",ImGuiTableColumnFlags_WidthStretch,.34f);
        ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthStretch,.66f);
        ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();ImGui::TextWrapped("%s",label);
        ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);changed=draw();ImGui::EndTable();
    }
    ImGui::PopID();return changed;
}
bool tuningInt(const char* label,int* value,int minimum,int maximum,const char* format="%d"){
    return labeledControl(label,[&]{return ImGui::SliderInt("##Value",value,minimum,maximum,format);});
}
bool tuningFloat(const char* label,float* value,float minimum,float maximum,const char* format="%.2f"){
    return labeledControl(label,[&]{return ImGui::SliderFloat("##Value",value,minimum,maximum,format);});
}
bool statusLight(const char* label,const std::string& detail,bool ready,const std::string& help,bool attention=false,bool tintText=false){
    const auto color=attention?ImVec4(1.f,.78f,.30f,1):ready?ImVec4(.38f,.83f,.60f,1):ImVec4(.55f,.59f,.62f,1);
    const auto lamp=attention&&std::fmod(ImGui::GetTime(),1.2)>=.6?ImVec4(.28f,.23f,.12f,1):color;
    ImGui::BeginGroup();
    const auto p=ImGui::GetCursorScreenPos();const float height=ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddCircleFilled({p.x+5,p.y+height*.5f},4,ImGui::GetColorU32(lamp));
    ImGui::Dummy({10,height});ImGui::SameLine();
    if(tintText)ImGui::PushStyleColor(ImGuiCol_Text,color);
    ImGui::TextWrapped("%s: %s",label,detail.c_str());
    if(tintText)ImGui::PopStyleColor();
    ImGui::EndGroup();const bool clicked=ImGui::IsItemClicked();
    if(ImGui::IsItemHovered()){
        ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetFontSize()*30);ImGui::TextUnformatted(help.c_str());ImGui::PopTextWrapPos();ImGui::EndTooltip();
    }
    return clicked;
}
void heading(const char* title,const char* description){
    auto& fonts=ImGui::GetIO().Fonts->Fonts;ImGui::PushFont(fonts.Size>1?fonts[1]:nullptr,34);
    ImGui::TextUnformatted(title);ImGui::PopFont();
    mutedText(description);ImGui::Dummy({0,8});
}
constexpr auto jsonFilter=L"JSON profile\0*.json\0\0";
Json frameJson(const Frame& f){return {{"version",1},{"state",f.state},{"aircraft",f.aircraft},{"values",f.values}};}

}
App::App(){
    try{auto p=dataDirectory()/L"settings.json";if(fs::exists(p))settings_=Settings::fromJson(Json::parse(readText(p)));}
    catch(const std::exception& e){uiMessage_=std::string("Settings could not load; using safe defaults. ")+e.what();}
    try{profiles_=dcsProfiles();removeLegacyStartup();}catch(const std::exception& e){uiMessage_=e.what();}
    calibrationLow_=settings_.motorFloor;calibrationHigh_=settings_.motorCap;testMotor_=std::min(15,settings_.motorCap);
    worker_=std::jthread([this](std::stop_token stop){run(stop);});
}
App::~App(){updates_.cancel();worker_.request_stop();if(worker_.joinable())worker_.join();try{save();}catch(...){} }
Settings App::settings()const{std::lock_guard lock(mutex_);return settings_;}
Snapshot App::snapshot()const{std::lock_guard lock(mutex_);return snapshot_;}
void App::command(Command c){std::lock_guard lock(mutex_);commands_.push_back(std::move(c));}
void App::save(){writeTextAtomic(dataDirectory()/L"settings.json",settings().json().dump(2));settingsDirty_=false;}
void App::emergencyStop(){
    {std::lock_guard lock(mutex_);settings_.muted=true;commands_.push_back({Action::Stop});}
    settingsDirty_=true;saveAt_=GetTickCount64()+200;
}
void App::renderUpdates(){
    const auto u=updates_.status();
    const bool available=u.release&&compareVersions(u.release->version,appVersion)>0;
    const bool busy=u.state==UpdateState::Checking||u.state==UpdateState::Downloading;
    const char* state=u.state==UpdateState::Checking?"checking":u.state==UpdateState::Downloading?"downloading":
        u.state==UpdateState::Ready?"ready to install":available?"update":
        u.state==UpdateState::Current?"current":"check unavailable";
    const auto detail=std::string(appVersionDisplay)+" / "+state;
    if(statusLight("Version",detail,u.state==UpdateState::Current,
        "Click for release details and updates. A flashing yellow light means a newer release is available; grey means the version could not be checked.",available,true))
        ImGui::OpenPopup("Application updates");
    if(ImGui::IsItemHovered())ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    auto install=[&]{
        updateInstallRequested_=false;
        try{
            if(!u.release)throw std::runtime_error("The release could not be verified. Check again.");
            verifyInstaller(u.installer,*u.release);save();
            if(!launchInstaller(u.installer,appDirectory()))throw std::runtime_error("The installer could not be opened. Retry or open Releases.");
            // The normal app shutdown stops the haptics helper and saves tuning.
            // Inno Setup opens visibly and offers to restart the app when finished.
            PostQuitMessage(0);
        }catch(const std::exception& e){updateLaunchError_=e.what();}
    };
    if(u.state==UpdateState::Ready&&updateInstallRequested_)install();
    ImGui::SetNextWindowSize({560,0});
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),ImGuiCond_Appearing,{.5f,.5f});
    if(ImGui::BeginPopupModal("Application updates",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+500);
        ImGui::Text("Installed: %s",appVersionDisplay);
        if(u.release)ImGui::TextWrapped("Newest published release: %s",u.release->version.c_str());
        if(u.state==UpdateState::Current)ImGui::TextWrapped("You're running the current version. No newer release is published on GitHub.");
        if(u.state==UpdateState::Checking)ImGui::TextWrapped("Checking the project's GitHub releases, including Alpha releases...");
        if(available){
            ImGui::TextWrapped("Download the Windows installer and update this installation. Your saved tuning is retained. PSVR2SimShaker will close when the installer opens; select Open PSVR2SimShaker at the end to restart it.");
            ImGui::TextWrapped("Close DCS before installing so its export bridge can be updated.");
            if(!u.release->installable())ImGui::TextWrapped("This release has no Windows installer with a GitHub SHA-256 checksum. Open Releases to download it manually.");
        }
        if(!u.message.empty())ImGui::TextWrapped("%s",u.message.c_str());
        if(!updateLaunchError_.empty())ImGui::TextWrapped("%s",updateLaunchError_.c_str());
        if(u.state==UpdateState::Downloading){
            ImGui::ProgressBar(u.release&&u.release->size?float(double(u.downloaded)/double(u.release->size)):0,{-1,0});
            ImGui::TextWrapped("Downloading and verifying the installer...");
            if(ImGui::Button("Cancel download")){updateInstallRequested_=false;updates_.cancel();}
        }else if(u.state==UpdateState::Ready){
            if(ImGui::Button("Open installer"))install();
        }else if(available&&u.release->installable()&&!busy){
            if(ImGui::Button("Download and install")){updateLaunchError_.clear();updateInstallRequested_=true;updates_.download();}
        }
        if(ImGui::Button("Open Releases")){
            const auto page=u.release?u.release->page:std::string(releasesUrl);
            ShellExecuteW(nullptr,L"open",wide(page).c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        }
        if(!busy){continueRow("Check again");if(ImGui::Button("Check again")){updateInstallRequested_=false;updateLaunchError_.clear();updates_.check();}}
        continueRow("Close");if(ImGui::Button("Close"))ImGui::CloseCurrentPopup();
        ImGui::PopTextWrapPos();ImGui::EndPopup();
    }
}
void App::run(std::stop_token stop){
    TelemetryReader reader;FlightFeed flightFeed;EffectEngine engine;HapticsClient haptics;Frame lastFrame;
    uint64_t lastSession=0,lastSequence=0,rawEnd=0,rawStart=0,rawWait=0,demoStart=0,idleSince=0;
    int rawMotor=0,cueDemo=-1;bool synthetic=false,gearDemo=false,demoWaiting=false,manualConnection=false;
    double gearTravelSeconds=8.,gearRestSeconds=1.1;
    std::vector<FlightDemoStage> timeline;Snapshot view;
    auto connect=[&](const Settings& s){if(!haptics.running())haptics.start(s.toolkitPath.empty()?toolkitFile():fs::path(wide(s.toolkitPath)));};
    auto endTest=[&]{
        rawMotor=0;rawWait=rawEnd=0;cueDemo=-1;synthetic=gearDemo=demoWaiting=false;
        engine.reset();view.demoComplete=false;view.demoSeconds=0;haptics.update(0,GetTickCount64());
    };
    while(!stop.stop_requested()){
        const auto now=GetTickCount64();Settings s;std::vector<Command> commands;
        {std::lock_guard lock(mutex_);s=settings_;commands.swap(commands_);}
        try{
            for(const auto& c:commands)switch(c.action){
            case Action::Connect:
                haptics.stop();manualConnection=true;connect(s);view.message="Connecting; SteamVR must be running.";break;
            case Action::Stop:
                endTest();view.message="Output stopped. Unmute to resume DCS effects.";break;
            case Action::EndTest:
                endTest();view.message="Test stopped.";break;
            case Action::Raw:
                endTest();rawMotor=std::clamp(c.value,10,25);rawWait=now+10000;
                connect(s);manualConnection=true;view.message="Waiting for headset, then playing four short vibrations.";break;
            case Action::Effect:case Action::FlightDemo:case Action::GearDemo:
                if(c.action==Action::Effect && (c.value<0 || !effectSupported(size_t(c.value))))break;
                endTest();synthetic=true;demoWaiting=true;gearDemo=c.action==Action::GearDemo;
                cueDemo=c.action==Action::Effect?c.value:-1;demoStart=now;
                gearTravelSeconds=s.gearDemoSeconds;gearRestSeconds=gearDemoRestSeconds(s);
                if(c.action==Action::FlightDemo){timeline=flightDemoStages(s);view.timeline=timeline;}
                connect(s);view.message="Waiting for the headset before starting the test.";break;
            }
            // Real DCS status keeps updating during tests, without mixing sources.
            TelemetryPacket packet;
            if(reader.read(packet) && (packet.sequence!=lastSequence || packet.session!=lastSession)){
                Frame f;std::string error;
                if(parseFrame(packet.payload,f,error)){
                    f.simTime=packet.simTime;f.session=packet.session;f.sequence=packet.sequence;f.receivedMs=now;
                    flightFeed.observe(f,packet.publishedMs);
                    if(!synthetic){lastFrame=f;if(!rawWait&&!rawEnd)engine.ingest(f,now,s.staleMs);}
                    lastSession=packet.session;lastSequence=packet.sequence;
                }else{view.message="Telemetry rejected: "+error;flightFeed.reset();if(!synthetic)engine.reset();}
            }
            if(synthetic){
                if(demoWaiting && haptics.status=="Headset connected" && !haptics.faulted){demoWaiting=false;demoStart=now;view.message="Test playing.";}
                const auto elapsed=double(now-demoStart)/1000.;
                const double duration=gearDemo?gearDemoDuration(gearTravelSeconds,gearRestSeconds):cueDemo>=0?
                    effectDemoDuration(size_t(cueDemo),s):timeline.back().start+timeline.back().duration;
                if(demoWaiting){if(elapsed>10 || haptics.faulted){endTest();view.message="Test cancelled: headset did not become ready.";}}
                else if(elapsed>=duration){
                    const bool completedFlight=!gearDemo && cueDemo<0;
                    endTest();view.demoComplete=completedFlight;view.demoSeconds=completedFlight?duration:0;view.message="Demo complete.";
                }else{
                    lastFrame=gearDemo?gearDemoFrame(elapsed,now,gearTravelSeconds,gearRestSeconds):cueDemo>=0?
                        effectDemoFrame(size_t(cueDemo),elapsed,now):flightDemoFrame(elapsed,now,timeline,gearTravelSeconds);
                    engine.ingest(lastFrame,now);view.demoSeconds=elapsed;
                }
            }
            const auto flight=flightFeed.status(now,s.staleMs);
            const bool fresh=flight.supported,live=!s.muted && fresh && !synthetic && !rawWait && !rawEnd;
            if(live || synthetic)connect(s);
            if(rawWait && haptics.status=="Headset connected" && !haptics.faulted){
                rawWait=0;rawStart=now;rawEnd=now+2750;view.message="Test playing; a stop command follows automatically.";
            }
            if(rawWait && now>rawWait){endTest();view.message="Test cancelled: headset did not become ready.";}
            if(rawEnd && now>=rawEnd){endTest();view.message="Demo complete.";}
            const bool rawPlaying=rawEnd && now<rawEnd;
            auto mixSettings=s;
            if(synthetic && (gearDemo || cueDemo>=0))for(size_t i=0;i<effectCount;++i)mixSettings.effects[i].enabled=int(i)==(gearDemo?int(Gear):cueDemo);
            auto mix=engine.tick(now,mixSettings);
            const int patternedMotor=rawMotor && now>=rawStart && (now-rawStart)%750<500?rawMotor:0;
            int motor=rawPlaying?patternedMotor:(live || (synthetic&&!demoWaiting)?mix.motor:0);
            if(s.muted)motor=0;
            haptics.update(motor,now);
            if(view.message=="Connecting; SteamVR must be running." && haptics.status=="Headset connected")view.message="Headset ready. DCS effects follow your flight automatically.";
            if(!live && !synthetic && !rawPlaying && !rawWait){if(!idleSince)idleSince=now;
                if(!manualConnection && now-idleSince>3000 && haptics.running())haptics.stop();
            }else idleSince=0;
            view.mix=mix;view.frame=lastFrame;view.requested=motor;view.acknowledged=haptics.acknowledgedMotor;
            view.headset=haptics.status;view.fault=haptics.faulted;view.fresh=fresh;view.flight=flight;view.ageMs=flight.ageMs;
            view.source=synthetic?(gearDemo?"Gear up/down demo":cueDemo>=0?std::string(effectNames[cueDemo])+" audition":"Demo flight"):rawPlaying||rawWait?"Headset test":reader.status;
            view.testing=synthetic||rawPlaying||rawWait;view.flightDemo=synthetic&&!gearDemo&&cueDemo<0;view.demoWaiting=demoWaiting||rawWait;
        }catch(const std::exception& e){endTest();view.message=e.what();view.requested=0;view.testing=view.flightDemo=view.demoWaiting=false;}
        {std::lock_guard lock(mutex_);snapshot_=view;}
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    haptics.update(0,GetTickCount64());haptics.stop();
}

void App::render(){
    auto s=settings();const auto v=snapshot();bool changed=false;
    const auto now=GetTickCount64();
    if(!hookCheckAt_ || now-hookCheckAt_>=2000){
        hookCheckAt_=now;installedHooks_=0;hookDetail_.clear();auto knownProfiles=profiles_;
        for(const auto& path:s.dcsProfiles){auto p=fs::path(wide(path));if(std::find(knownProfiles.begin(),knownProfiles.end(),p)==knownProfiles.end())knownProfiles.push_back(p);}
        hookProfiles_=knownProfiles.size();
        for(const auto& p:knownProfiles){const bool installed=integrationInstalled(p);installedHooks_+=installed?1:0;
            hookDetail_+=utf8(p.filename().wstring())+(installed?": installed\n":": not installed\n");}
        hookDetail_+="Checks the export entry and its Lua/DLL files. After installing or repairing the hook, restart DCS.";
    }
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("PSVR2SimShaker",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    if(ImGui::BeginTable("AppHeader",3)){
        ImGui::TableSetupColumn("Brand",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Pages",ImGuiTableColumnFlags_WidthFixed,336);
        ImGui::TableSetupColumn("Stop",ImGuiTableColumnFlags_WidthFixed,112);
        ImGui::TableNextColumn();ImGui::BeginGroup();
        auto& fonts=ImGui::GetIO().Fonts->Fonts;ImGui::PushFont(fonts.Size>1?fonts[1]:nullptr,34);
        ImGui::TextColored({.35f,.73f,.84f,1},"PSVR2SimShaker");ImGui::PopFont();
        ImGui::SameLine(0,10);ImGui::PushFont(nullptr,16);ImGui::TextColored({.35f,.73f,.84f,1},"(GitHub)");ImGui::PopFont();
        ImGui::PushFont(nullptr,20);ImGui::TextUnformatted("by Adam Chesters");ImGui::PopFont();ImGui::EndGroup();
        if(ImGui::IsItemHovered()){ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);ImGui::SetTooltip("Open PSVR2SimShaker on GitHub");}
        if(ImGui::IsItemClicked())ShellExecuteW(nullptr,L"open",L"https://github.com/AdamChesters/PSVR2SimShaker",nullptr,nullptr,SW_SHOWNORMAL);
        ImGui::TableNextColumn();const char* pages[]={"Effects","Headset","Settings"};
        for(int i=0;i<3;++i){
            if(i)ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,page_==i?ImVec4(.14f,.29f,.34f,1):ImVec4(.055f,.07f,.08f,1));
            if(ImGui::Button(pages[i],{100,38}))page_=i;ImGui::PopStyleColor();
        }
        ImGui::TableNextColumn();ImGui::PushStyleColor(ImGuiCol_Button,{.29f,.12f,.13f,1});
        if(ImGui::Button("STOP",{108,38})){emergencyStop();s.muted=true;changed=true;}ImGui::PopStyleColor();
        if(ImGui::IsItemHovered())ImGui::SetTooltip("Stop and mute all output. Ctrl + Alt + Space.");ImGui::EndTable();
    }
    ImGui::Spacing();
    const float statusWidth=ImGui::GetContentRegionAvail().x;
    const bool statusList=statusWidth<980.f;
    if(ImGui::BeginTable("ConnectionLights",statusList?1:4,ImGuiTableFlags_SizingStretchSame,{std::min(statusWidth,980.f),0})){
        ImGui::TableNextColumn();const auto hook=installedHooks_?(installedHooks_==hookProfiles_?std::string("Installed"):std::to_string(installedHooks_)+"/"+std::to_string(hookProfiles_)+" profiles"):"Not installed";
        statusLight("DCS hook",hook,installedHooks_>0,hookDetail_);
        ImGui::TableNextColumn();statusLight("DCS telemetry",v.flight.telemetryLive?"Live":"Waiting / paused",v.flight.telemetryLive,"Lights when real shared-memory telemetry has an advancing DCS clock. Tests do not change this light.");
        ImGui::TableNextColumn();const auto aircraft=v.flight.aircraftLive?(v.flight.supported?std::string("Live / Hornet"):std::string("Live / unsupported")):"Waiting";
        statusLight("Aircraft",aircraft,v.flight.aircraftLive,"Requires aircraft identity and numeric signals in the live DCS feed. Individual effects still depend on their own signals. Aircraft: "+(v.flight.aircraft.empty()?std::string("none"):v.flight.aircraft));
        ImGui::TableNextColumn();renderUpdates();
        ImGui::EndTable();
    }
    ImGui::Separator();ImGui::Spacing();
    const auto message=!uiMessage_.empty()?uiMessage_:v.message;
    const std::string status=v.testing?v.source+(v.demoWaiting?" / waiting for headset":" / playing"):
        v.fault?"Headset output fault. Reconnect in Headset.":s.muted?"Output muted":v.headset;
    const float footerWidth=std::max(100.f,ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ScrollbarSize);
    const float statusHeight=ImGui::CalcTextSize(status.c_str(),nullptr,false,footerWidth-(v.testing?100.f:0.f)).y;
    const float messageHeight=message.empty()?0:ImGui::CalcTextSize(message.c_str(),nullptr,false,footerWidth).y+ImGui::GetStyle().ItemSpacing.y;
    const float footerHeight=std::clamp(statusHeight+messageHeight+24.f,52.f,140.f);
    const auto pageId=std::string("Page")+std::to_string(page_);
    ImGui::BeginChild(pageId.c_str(),{0,-footerHeight},ImGuiChildFlags_None);ImGui::PushItemWidth(-260);
    auto audition=[&](size_t i){s.muted=false;changed=true;command({i==Gear?Action::GearDemo:Action::Effect,int(i)});};
    auto drawEffect=[&](size_t i){
        auto& e=s.effects[i];const auto& definition=effectDefinition(i);ImGui::PushID(int(i));
        ImGui::PushStyleColor(ImGuiCol_ChildBg,{.042f,.052f,.06f,1});ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{16,10});
        ImGui::BeginChild("Cue",{0,0},ImGuiChildFlags_Borders|ImGuiChildFlags_AutoResizeY);
        if(ImGui::BeginTable("Controls",5,ImGuiTableFlags_SizingStretchProp)){
            ImGui::TableSetupColumn("Enabled",ImGuiTableColumnFlags_WidthFixed,28);
            ImGui::TableSetupColumn("Name",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Strength",ImGuiTableColumnFlags_WidthFixed,210);
            ImGui::TableSetupColumn("Test",ImGuiTableColumnFlags_WidthFixed,68);
            ImGui::TableSetupColumn("Tune",ImGuiTableColumnFlags_WidthFixed,76);
            ImGui::TableNextColumn();changed|=ImGui::Checkbox("##Live",&e.enabled);
            if(ImGui::IsItemHovered())ImGui::SetTooltip("Enable %s during live flight",effectNames[i]);
            ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();ImGui::TextWrapped("%s",effectNames[i]);
            ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);
            if(ImGui::SliderInt("##Strength",&e.motorMax,10,25,"Strength %d")){e.motorMin=std::min(e.motorMin,e.motorMax);changed=true;}
            if(ImGui::IsItemHovered())ImGui::SetTooltip("Peak motor command. Your master response and ceiling also apply.");
            ImGui::TableNextColumn();if(ImGui::Button("Test",{64,0}))audition(i);
            ImGui::TableNextColumn();if(ImGui::Button(expanded_[i]?"Less":"Tune",{72,0}))expanded_[i]=!expanded_[i];
            ImGui::EndTable();
        }
        if(expanded_[i]){
            ImGui::Separator();ImGui::TextWrapped("%s",definition.description);
            ImGui::TextWrapped("Test plays this cue alone, even when its live switch is off.");
            ImGui::PushItemWidth(-280);
            if(i==Gear){
                changed|=tuningFloat("Demo travel per direction (s)",&s.gearDemoSeconds,3,15,"%.1f");
                mutedText("Live travel follows DCS gear movement.");
                if(ImGui::CollapsingHeader("Rhythm")){
                    changed|=tuningInt("Start hit (ms)",&s.gearStartMs,50,1000);
                    changed|=tuningInt("Quiet after start (ms)",&s.gearStartGapMs,0,1500);
                    changed|=tuningInt("Travel ramp (ms)",&s.gearRampMs,0,3000);
                    changed|=tuningInt("Quiet before lock (ms)",&s.gearLockGapMs,0,1500);
                    changed|=tuningInt("Lock hit (ms)",&s.gearLockMs,50,1000);
                    if(ImGui::Button("Apply gear preset")){applyGearPreset(s);changed=true;uiMessage_="Gear preset applied, including 4.8-second demo travel per direction.";}
                }
            }else if(ImGui::CollapsingHeader("Timing")){
                if(definition.shape==CueShape::Continuous)changed|=tuningInt("Stop fade (ms)",&e.settleMs,0,1000);
                else{
                    changed|=tuningInt(definition.shape==CueShape::Burst?"Burst hold (ms)":"Hit length (ms)",&e.holdMs,80,1000);
                    changed|=tuningInt(definition.shape==CueShape::Surge?"Rumble tail (ms)":"Settle tail (ms)",&e.settleMs,0,1000);
                    changed|=tuningInt("Quiet recovery (ms)",&e.coastMs,0,1500);
                    if(definition.shape!=CueShape::Burst)changed|=tuningInt("Repeat cooldown (ms)",&e.cooldownMs,0,3000);
                }
            }
            if(ImGui::CollapsingHeader("Response and mixing")){
                changed|=tuningInt("Minimum strength",&e.motorMin,10,e.motorMax);
                changed|=tuningFloat("Response gain",&e.gain,0,2,"%.2f");
                changed|=tuningFloat("Threshold",&e.threshold,0,.9f,"%.3f");
                changed|=tuningFloat("Curve",&e.curve,.2f,3,"%.2f");
                changed|=tuningFloat("Attack (ms)",&e.attackMs,0,500,"%.0f");
                changed|=tuningFloat("Release (ms)",&e.releaseMs,20,1000,"%.0f");
                changed|=tuningInt("Priority",&e.priority,0,100);
                ImGui::TextWrapped("Higher-priority cues take over. Quiet recovery holds lower-priority ambience back while the motor coasts.");
            }
            if(ImGui::CollapsingHeader("Command preview and activity")){
                static std::array<std::string,effectCount> keys;
                static std::array<std::array<float,400>,effectCount> previews{};
                const auto key=s.json().dump();auto& preview=previews[i];
                if(key!=keys[i]){
                    keys[i]=key;auto previewSettings=s;
                    for(size_t j=0;j<effectCount;++j)previewSettings.effects[j].enabled=j==i;
                    EffectEngine engine;const double duration=effectDemoDuration(i,s);
                    for(size_t j=0;j<preview.size();++j){
                        const double t=duration*j/(preview.size()-1);const auto now=1000+uint64_t(t*1000);
                        engine.ingest(i==Gear?gearDemoFrame(t,now,s.gearDemoSeconds,gearDemoRestSeconds(s)):effectDemoFrame(i,t,now),now);
                        preview[j]=float(engine.tick(now,previewSettings).motor);
                    }
                }
                ImGui::PlotLines("##Preview",preview.data(),int(preview.size()),0,"Demo timeline / zero = coast",0,25,{-1,90});
                mutedText("Motor commands, not measured physical force.");
                ImGui::TextWrapped("Current activity: %s",v.mix.effects[i].reason.c_str());
            }
            ImGui::PopItemWidth();
        }
        ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopStyleColor();ImGui::PopID();
    };
    if(page_==0){
        heading("Feel your flight.","Your headset mix. Effects follow your flight automatically.");
        if(ImGui::BeginTable("FlightControls",3)){
            ImGui::TableSetupColumn("Flight status",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Master",ImGuiTableColumnFlags_WidthFixed,230);
            ImGui::TableSetupColumn("Mute",ImGuiTableColumnFlags_WidthFixed,110);
            ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();mutedText(s.muted?"Output muted":v.fresh?"Hornet connected":"Waiting for a Hornet flight");
            ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);float percent=s.master*100;
            if(ImGui::SliderFloat("##Master",&percent,0,100,"Master %.0f%%")){s.master=percent/100;changed=true;}
            ImGui::TableNextColumn();changed|=ImGui::Checkbox("Muted",&s.muted);ImGui::EndTable();
        }
        ImGui::Spacing();
        for(size_t i:{size_t(Gear),size_t(Gun),size_t(Touchdown),size_t(Afterburner),size_t(AfterburnerRumble),size_t(Stores),size_t(Countermeasures),size_t(Buffet),size_t(Airflow)})drawEffect(i);
        if(ImGui::CollapsingHeader("Optional ambience")){
            ImGui::TextWrapped("Extra continuous feedback if you do not use a haptic seat or stick. Off in the default mix.");
            drawEffect(Engine);drawEffect(Taxi);
        }
    }else if(page_==1){
        heading("Find your feel.","Connect the headset and choose a comfortable strength.");
        ImGui::TextWrapped("%s",v.headset.c_str());
        if(ImGui::Button("Connect / reconnect"))command({Action::Connect});continueRow("Start SteamVR");
        if(ImGui::Button("Start SteamVR"))ShellExecuteW(nullptr,L"open",L"steam://rungameid/250820",nullptr,nullptr,SW_SHOWNORMAL);
        ImGui::Dummy({0,12});
        ImGui::SeparatorText("Test the headset");
        tuningInt("Test strength",&testMotor_,10,25);
        if(ImGui::Button("Test headset",{180,38})){s.muted=false;changed=true;command({Action::Raw,testMotor_});}
        continueRow("BRR  BRR  BRR  BRR");mutedText("BRR  BRR  BRR  BRR");
        ImGui::TextWrapped("Four 500 ms vibrations with quiet gaps. This direct test uses the selected strength, independently of your flight mix and ceiling.");
        ImGui::Spacing();
        if(ImGui::Button(v.flightDemo?"Restart demo flight":"Demo flight",{180,38})){s.muted=false;changed=true;command({Action::FlightDemo});}
        continueRow("Flight timeline");
        if(ImGui::Button("Flight timeline"))showTimeline_=!showTimeline_;
        mutedText("Plays your enabled mix through the headset, including your saved gear travel time. Master and flight ceiling apply.");
        if(showTimeline_ || v.flightDemo){
            const auto stages=(v.flightDemo||v.demoComplete)&&!v.timeline.empty()?v.timeline:flightDemoStages(s);
            const double duration=stages.back().start+stages.back().duration;
            const double elapsed=v.flightDemo||v.demoComplete?v.demoSeconds:0;
            const int active=v.flightDemo&&!v.demoWaiting?flightDemoStageAt(stages,elapsed):-1;
            char progress[96];snprintf(progress,sizeof(progress),"%s / %.1f of %.1f s",v.demoWaiting&&v.flightDemo?"Waiting for headset":v.flightDemo?"Playing":v.demoComplete?"Complete":"Ready",elapsed,duration);
            ImGui::ProgressBar(float(elapsed/duration),{-1,22},progress);
            if(ImGui::BeginTable("FlightTimeline",2,ImGuiTableFlags_SizingStretchProp)){
                ImGui::TableSetupColumn("Time",ImGuiTableColumnFlags_WidthFixed,124);
                ImGui::TableSetupColumn("Stage",ImGuiTableColumnFlags_WidthStretch);
                for(size_t i=0;i<stages.size();++i){const auto& stage=stages[i];const bool current=int(i)==active;
                    ImGui::TableNextRow();
                    if(current)ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,ImGui::GetColorU32(ImVec4(.10f,.25f,.30f,1)));
                    ImGui::TableNextColumn();ImGui::Text("%s %04.1f-%04.1f s",current?">":elapsed>=stage.start+stage.duration?"+":" ",stage.start,stage.start+stage.duration);
                    ImGui::TableNextColumn();ImGui::TextWrapped("%s",stage.title);
                    if(current)ImGui::TextWrapped("%s",stage.description);
                    else if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::PushTextWrapPos(420);ImGui::TextUnformatted(stage.description);ImGui::PopTextWrapPos();ImGui::EndTooltip();}
                }ImGui::EndTable();
            }
            mutedText("Highlighted = current stage; + = finished. Disabled cues stay quiet. This is a test sequence, not DCS telemetry.");
        }
        ImGui::Dummy({0,12});
        if(tuningInt("Flight strength ceiling",&s.motorCap,10,25)){s.motorFloor=std::min(s.motorFloor,s.motorCap);changed=true;}
        mutedText("Every effect and effect test respects this ceiling.");
        ImGui::Dummy({0,10});
        if(ImGui::CollapsingHeader("Fine-tune your useful range")){
            tuningInt("Lowest clearly felt",&calibrationLow_,10,25);calibrationHigh_=std::max(calibrationLow_,calibrationHigh_);
            tuningInt("Comfortable ceiling",&calibrationHigh_,calibrationLow_,25);
            if(ImGui::Button("Fit all cues to this range")){fitEffectRanges(s,calibrationLow_,calibrationHigh_);changed=true;uiMessage_="Cue ranges fitted to your chosen motor range.";}
            changed|=tuningFloat("Motor response curve",&s.motorCurve,.25f,3,"%.2f");
            ImGui::TextWrapped("1.00 spaces command settings evenly. Lower values strengthen medium cues; higher values soften them. This is a subjective response curve.");
        }
        if(ImGui::CollapsingHeader("Connection help")){
            ImGui::TextWrapped("Start a headset session with vr2jb.exe (v1.0.1). For a headset already set up with compatible PSVR2Toolkit and firmware 6.00:");
            static constexpr auto steps=
                "1. Exit DCS, SteamVR, the PlayStation VR2 App and PSVR2SimShaker. In SimShaker use Settings > Exit application, or the tray's Exit; X only hides the window.\n\n"
                "2. Turn on the PSVR2 headset and keep it awake. Leave SteamVR closed.\n\n"
                "3. In the extracted vr2jb-windows-linux-builds-v1.0.1 folder, run vr2jb.exe with no arguments. Wait for success; the console closes after about 8 seconds. A white LED blink every 2 seconds indicates the unlock.\n\n"
                "4. Start SteamVR and wait until the headset is connected.\n\n"
                "5. Open PSVR2SimShaker, then start a DCS Hornet mission. The top lights show the installed hook, advancing DCS telemetry and aircraft data. Effects start automatically; headset tests are optional.";
            ImGui::TextWrapped("%s",steps);
            ImGui::TextWrapped("Repeat after a red-LED headset shutdown. vr2jb.exe and the Toolkit test app do not need to stay running. First-time firmware setup is covered by the official guide below.");
            if(ImGui::Button("Copy startup steps"))ImGui::SetClipboardText(steps);
            continueRow("Download vr2jb v1.0.1");
            if(ImGui::Button("Download vr2jb v1.0.1"))ShellExecuteW(nullptr,L"open",L"https://github.com/BnuuySolutions/vr2jb/releases/tag/v1.0.1",nullptr,nullptr,SW_SHOWNORMAL);
            continueRow("Official setup guide");
            if(ImGui::Button("Official setup guide"))ShellExecuteW(nullptr,L"open",L"https://github.com/BnuuySolutions/PSVR2Toolkit/wiki/Jailbreaking-your-headset",nullptr,nullptr,SW_SHOWNORMAL);
            ImGui::TextWrapped("The app's strength setting is the toolkit's 10–25 motor command. Actual frequency and force have not been measured. A toolkit acknowledgement confirms the call completed, not physical vibration.");
            ImGui::TextWrapped("If output faults, recover the VR runtime and reconnect. A stalled driver can delay even a stop command.");
        }
    }else{
        heading("Make it yours.","Profiles, DCS integration and the occasional deeper adjustment.");
        if(ImGui::CollapsingHeader("Profiles and presets")){
            char name[128]{};strncpy_s(name,s.profile.c_str(),_TRUNCATE);
            if(labeledControl("Profile name",[&]{return ImGui::InputText("##Name",name,sizeof(name));})){s.profile=name;changed=true;}
            if(ImGui::Button("Apply default headset mix")){applyHeadsetMix(s);changed=true;uiMessage_="Flight cues updated. Your gear rhythm, demo travel and headset range were preserved.";}
            ImGui::TextWrapped("Nine flight cues on; optional engine and runway ambience off. Preserves gear tuning, master response and the headset ceiling.");
            if(ImGui::Button("Export profile...")){auto p=chooseFile(true,jsonFilter,L"json");if(!p.empty())try{
                auto j=s.json();for(const char* k:{"toolkitPath","dcsProfiles","muted"})j.erase(k);
                writeTextAtomic(p,j.dump(2));uiMessage_="Profile exported.";
            }catch(const std::exception& e){uiMessage_=e.what();}}
            continueRow("Import profile...");if(ImGui::Button("Import profile...")){auto p=chooseFile(false,jsonFilter,L"json");if(!p.empty())try{
                auto imported=Settings::fromJson(Json::parse(readText(p)));fitEffectRanges(imported,s.motorFloor,s.motorCap);
                s.effects=imported.effects;s.master=imported.master;s.gearStartMs=imported.gearStartMs;s.gearLockMs=imported.gearLockMs;
                s.gearStartGapMs=imported.gearStartGapMs;s.gearLockGapMs=imported.gearLockGapMs;s.gearRampMs=imported.gearRampMs;
                s.gearDemoSeconds=imported.gearDemoSeconds;s.profile=imported.profile;changed=true;uiMessage_="Profile imported and fitted to your local motor range.";
            }catch(const std::exception& e){uiMessage_=e.what();}}
            if(ImGui::TreeNode("More presets")){
                auto defaults=[&]{auto mix=Settings{};fitEffectRanges(mix,s.motorFloor,s.motorCap);s.effects=mix.effects;};
                if(ImGui::Button("Comfort")){defaults();s.master=.65f;s.profile="Hornet - Comfort";changed=true;}
                continueRow("Events only");if(ImGui::Button("Events only")){defaults();for(size_t i:{size_t(Buffet),size_t(Airflow),size_t(AfterburnerRumble),size_t(Engine),size_t(Taxi)})s.effects[i].enabled=false;s.profile="Hornet - Events";changed=true;}
                ImGui::TreePop();
            }
        }
        if(ImGui::CollapsingHeader("DCS integration")){
            if(profiles_.empty())ImGui::TextWrapped("No DCS Saved Games profiles found. Launch DCS once, then refresh.");
            for(const auto& p:profiles_){const auto path=utf8(p.wstring());ImGui::PushID(path.c_str());ImGui::TextWrapped("%s",path.c_str());
                const bool installed=integrationInstalled(p);mutedText(installed?"Export installed":"Export not installed");
                if(ImGui::Button(installed?"Repair export":"Install export"))try{
                    installIntegration(p,appDirectory());if(std::find(s.dcsProfiles.begin(),s.dcsProfiles.end(),path)==s.dcsProfiles.end())s.dcsProfiles.push_back(path);
                    changed=true;uiMessage_="Export installed. Restart DCS to load it.";
                }catch(const std::exception& e){uiMessage_=e.what();}
                if(installed){continueRow("Remove our export");if(ImGui::Button("Remove our export"))try{removeIntegration(p);std::erase(s.dcsProfiles,path);changed=true;uiMessage_="Our export removed.";}catch(const std::exception& e){uiMessage_=e.what();}}
                ImGui::PopID();ImGui::Spacing();
            }
            if(ImGui::Button("Refresh DCS profiles"))try{profiles_=dcsProfiles();}catch(const std::exception& e){uiMessage_=e.what();}
        }
        if(ImGui::CollapsingHeader("Advanced connection and diagnostics")){
            auto capi=s.toolkitPath.empty()?toolkitFile():fs::path(wide(s.toolkitPath));
            ImGui::TextWrapped("%s",capi.empty()?"Toolkit CAPI DLL not found":utf8(capi.wstring()).c_str());
            if(ImGui::Button("Select toolkit DLL...")){auto p=chooseFile(false,L"PSVR2Toolkit CAPI\0psvr2_toolkit_capi.dll\0\0",L"dll");if(!p.empty()){s.toolkitPath=utf8(p.wstring());changed=true;}}
            continueRow("Detect automatically");if(ImGui::Button("Detect automatically")){s.toolkitPath.clear();changed=true;}
            changed|=tuningInt("Stale cutoff (ms)",&s.staleMs,150,1000);
            ImGui::Text("Motor requested %d / acknowledged %d",v.requested,v.acknowledged);
            ImGui::TextWrapped("Telemetry: %s",v.source.c_str());
            if(ImGui::Button("Export diagnostic report...")){auto p=chooseFile(true,jsonFilter,L"json");if(!p.empty())try{
                Json j={{"appVersion",appVersion},{"headset",v.headset},{"source",v.source},{"requested",v.requested},{"acknowledged",v.acknowledged},{"telemetry",frameJson(v.frame)},{"ageMs",v.ageMs},{"toolkitSha256",capi.empty()?"missing":sha256(capi)}};
                writeTextAtomic(p,j.dump(2));uiMessage_="Diagnostic report saved; review before sharing.";
            }catch(const std::exception& e){uiMessage_=e.what();}}
            if(ImGui::TreeNode("Live signal values")){
                if(ImGui::BeginTable("Signals",2,ImGuiTableFlags_RowBg)){
                    for(const auto& [key,value]:v.frame.values){ImGui::TableNextColumn();ImGui::TextUnformatted(key.c_str());ImGui::TableNextColumn();ImGui::Text("%.5g",value);}ImGui::EndTable();
                }ImGui::TreePop();
            }
            ImGui::TextWrapped("Damage and ejection are deferred until reliable own-aircraft events are verified. Missing signals cannot trigger an effect.");
        }
        ImGui::Dummy({0,18});mutedText((std::string("PSVR2SimShaker ")+appVersionDisplay).c_str());
        if(ImGui::Button("Source and credits"))ShellExecuteW(nullptr,L"open",L"https://github.com/AdamChesters/PSVR2SimShaker",nullptr,nullptr,SW_SHOWNORMAL);
        continueRow("Settings folder");if(ImGui::Button("Settings folder"))openPath(dataDirectory());
        continueRow("Exit application");if(ImGui::Button("Exit application"))PostQuitMessage(0);
        mutedText("Closing the window leaves the app in the tray. Settings save automatically.");
    }
    ImGui::PopItemWidth();ImGui::EndChild();ImGui::Separator();
    ImGui::BeginChild("StatusFooter",{0,0},ImGuiChildFlags_None);
    if(v.testing && ImGui::BeginTable("TestStatus",2)){
        ImGui::TableSetupColumn("Status",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Stop",ImGuiTableColumnFlags_WidthFixed,90);
        ImGui::TableNextColumn();ImGui::TextWrapped("%s",status.c_str());
        ImGui::TableNextColumn();if(ImGui::SmallButton("Stop test"))command({Action::EndTest});ImGui::EndTable();
    }else{
        ImGui::PushStyleColor(ImGuiCol_Text,v.fault?ImVec4(1,.48f,.43f,1):ImVec4(.55f,.67f,.70f,1));
        ImGui::TextWrapped("%s",status.c_str());ImGui::PopStyleColor();
    }
    if(!message.empty())mutedText(message.c_str());
    ImGui::EndChild();
    ImGui::End();
    if(changed){std::lock_guard lock(mutex_);settings_=s;settingsDirty_=true;saveAt_=GetTickCount64()+400;}
    if(settingsDirty_ && GetTickCount64()>=saveAt_)try{save();}catch(const std::exception& e){uiMessage_=e.what();settingsDirty_=false;}
}

int runProbe(int motor,const fs::path& output){
    HapticsClient client;Json report={{"appVersion",appVersion},{"requestedMotor",motor}};auto path=toolkitFile();
    try{report["toolkitSha256"]=sha256(path);client.start(path);}catch(const std::exception&e){report["error"]=e.what();writeTextAtomic(output,report.dump(2));return 2;}
    const auto start=GetTickCount64();uint64_t began=0,stopSent=0;bool acknowledged=false,stopped=false;
    while(GetTickCount64()-start<10000){
        auto now=GetTickCount64();int target=0;
        if(!began && client.status=="Headset connected")began=now;
        if(began && now-began<250)target=motor;
        if(began && now-began>=250 && !stopSent)stopSent=now;
        client.update(target,now);
        if(motor && client.acknowledgedMotor==motor)acknowledged=true;
        if(stopSent && now-stopSent>100 && client.acknowledgedMotor==0 && client.status=="Headset connected"){stopped=true;break;}
        if(client.faulted)break;
        Sleep(20);
    }
    report["status"]=client.status;report["commandAcknowledged"]=motor?acknowledged:began!=0;report["stopAcknowledged"]=stopped;
    client.update(0,GetTickCount64());client.stop();writeTextAtomic(output,report.dump(2));return stopped && (!motor||acknowledged)?0:1;
}
}
