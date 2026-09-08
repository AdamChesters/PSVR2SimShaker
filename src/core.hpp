#pragma once
#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace shaker {
using Json = nlohmann::json;
inline constexpr size_t effectCount = 12;
enum EffectId : size_t { Buffet, Gun, Touchdown, Taxi, Gear, Afterburner, Stores, Engine, Countermeasures, Airflow, Damage, AfterburnerRumble };
inline constexpr std::array<const char*, effectCount> effectNames = {
    "Airborne buffet", "Gun burst", "Touchdown", "Runway bumps", "Gear up / down", "Afterburner onset", "Store release", "Engine ambience", "Countermeasures", "Gear / brake airflow", "Damage impact", "Afterburner rumble"};
inline constexpr std::array<const char*, effectCount> effectKeys = {
    "buffet","gun","touchdown","taxi","gear","afterburner","stores","engine","countermeasures","airflow","damage","afterburner_rumble"};
struct Frame {
    uint64_t session = 0, sequence = 0, receivedMs = 0;
    double simTime = 0;
    std::string aircraft, state;
    std::map<std::string,double> values;
    std::optional<double> value(const char* key) const;
};
bool parseFrame(const std::string& payload, Frame& result, std::string& error);
struct FlightFeedStatus {
    bool telemetryLive=false, aircraftLive=false, supported=false;
    uint64_t ageMs=0;
    std::string aircraft;
};
// Observe only the real shared-memory feed, independently of tests.
class FlightFeed {
    std::optional<Frame> current_;
    uint64_t advancedAt_=0;
public:
    void reset();
    void observe(const Frame& frame,uint64_t publishedMs);
    FlightFeedStatus status(uint64_t now,int staleMs) const;
};
struct EffectConfig {
    bool enabled = true;
    float gain = 0.6f, threshold = 0.05f, curve = 1.0f, attackMs = 30, releaseMs = 180;
    int motorMin = 10, motorMax = 18, priority = 40;
    int holdMs = 180, settleMs = 140, coastMs = 300, cooldownMs = 650;
};
enum class CueShape { Continuous, Burst, Impact, Surge, Mechanism, Unavailable };
struct EffectDefinition { CueShape shape; bool recommended; const char* description; };
const EffectDefinition& effectDefinition(size_t effect);
bool effectSupported(size_t effect);
struct Settings {
    std::string profile = "Hornet - Headset essentials";
    bool muted = false;
    float master = 1.0f, motorCurve = 1.0f;
    int motorFloor = 10, motorCap = 25, staleMs = 300;
    int gearStartMs = 200, gearLockMs = 250;
    int gearStartGapMs = 300, gearLockGapMs = 460, gearRampMs = 1200;
    float gearDemoSeconds = 8.f;
    std::array<EffectConfig,effectCount> effects;
    std::vector<std::string> dcsProfiles;
    std::string toolkitPath;
    Settings();
    Json json() const;
    static Settings fromJson(const Json& j);
};
struct EffectView {
    float level = 0;
    bool available = false;
    std::string reason = "Waiting for telemetry";
};
struct Mix {
    int motor = 0, dominant = -1;
    float intensity = 0;
    std::array<EffectView,effectCount> effects;
};
class EffectEngine {
    std::optional<Frame> previous_;
    std::array<float,effectCount> targets_{}, levels_{};
    std::array<uint64_t,effectCount> pendingAt_{}, consumedAt_{}, beganAt_{}, lastEventAt_{}, offAt_{};
    std::array<float,effectCount> pendingStrength_{}, eventStrength_{};
    std::array<bool,effectCount> available_{};
    uint64_t lastTick_ = 0, freshAt_ = 0;
    int held_ = -1;
    uint64_t holdUntil_ = 0;
    bool afterburnerOn_ = false;
    bool gearMoving_ = false;
    int gearDirection_ = 0;
    uint64_t gearBegan_ = 0, gearChanged_ = 0, gearLockedAt_ = 0;
    uint64_t airborneAt_ = 0;
    std::optional<double> taxiBaseline_;
public:
    void reset();
    void ingest(const Frame& frame, uint64_t now, int maxGapMs = 300);
    void trigger(size_t effect, uint64_t now, float strength = 0.7f);
    Mix tick(uint64_t now, const Settings& settings, bool demo = false);
};
int mapMotor(float intensity, int minimum, int maximum, int cap, float curve = 1.0f);
void fitEffectRanges(Settings& settings, int minimum, int maximum);
void applyGearPreset(Settings& settings);
void applyHeadsetMix(Settings& settings);
Frame effectDemoFrame(size_t effect, double time, uint64_t now);
double effectDemoDuration(size_t effect, const Settings& settings);
enum class FlightDemoStep { Roll, GearUp, Gunfire, Buffet, Airbrake, Afterburner, Stores, Countermeasures, GearDown, Landing, Rest };
struct FlightDemoStage {
    FlightDemoStep step;
    double start, duration;
    const char* title;
    const char* description;
};
std::vector<FlightDemoStage> flightDemoStages(const Settings& settings);
int flightDemoStageAt(const std::vector<FlightDemoStage>& stages,double time);
Frame flightDemoFrame(double time,uint64_t now,const std::vector<FlightDemoStage>& stages,double gearTravelSeconds);
double gearDemoRestSeconds(const Settings& settings);
Frame gearDemoFrame(double time, uint64_t now, double travelSeconds = 8.0, double restSeconds = 1.1);
double gearDemoDuration(double travelSeconds, double restSeconds = 1.1);
}
