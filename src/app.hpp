#pragma once
#include "core.hpp"
#include "platform.hpp"
#include "updates.hpp"
#include <atomic>
#include <mutex>
#include <thread>

namespace shaker {
struct Snapshot {
    Mix mix;
    Frame frame;
    FlightFeedStatus flight;
    std::string source="Waiting for DCS",headset="Headset output idle",message;
    int requested=0,acknowledged=0;
    uint64_t ageMs=0;
    bool fresh=false,testing=false,flightDemo=false,demoWaiting=false,demoComplete=false,fault=false;
    double demoSeconds=0;
    std::vector<FlightDemoStage> timeline;
};
enum class Action { Connect,Stop,Raw,Effect,FlightDemo,GearDemo,EndTest };
struct Command {Action action;int value=0;};
class App {
    mutable std::mutex mutex_;
    Settings settings_;
    Snapshot snapshot_;
    std::vector<Command> commands_;
    std::jthread worker_;
    UpdateClient updates_;
    bool updateInstallRequested_=false;
    std::string updateLaunchError_;
    int page_=0,selectedEffect_=0,testMotor_=15,calibrationLow_=10,calibrationHigh_=18;
    std::array<bool,effectCount> expanded_{};
    bool settingsDirty_=false,showTimeline_=true;
    uint64_t saveAt_=0;
    uint64_t hookCheckAt_=0;
    size_t hookProfiles_=0,installedHooks_=0;
    std::string hookDetail_;
    std::string uiMessage_;
    std::vector<fs::path> profiles_;
    void run(std::stop_token stop);
    void save();
    void renderUpdates();
public:
    App();
    ~App();
    void command(Command c);
    void emergencyStop();
    void render();
    Settings settings()const;
    Snapshot snapshot()const;
};
int runWindow(HINSTANCE instance);
int runProbe(int motor,const fs::path& output);
}
