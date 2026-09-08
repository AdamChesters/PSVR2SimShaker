# Hornet headset effects

Effects use bounded motor commands and envelopes suited to a single headset motor. Strength settings are not measured hertz or acceleration.

## Everyday controls

**Effects** lists the flight cues. Each row has a enable switch, peak strength and **Test**. Test plays the selected cue alone, even if its switch is off, without changing that switch. Master response and the headset ceiling still apply. **STOP** mutes all output and cancels tests. **Stop test** ends an effect audition; available DCS flight output resumes unless muted.

**Tune** expands a cue's description and optional timing, response/mixing, and command-preview panels. These panels start closed. **Optional ambience** holds the engine and runway effects. The preview plots the actual engine's synthetic motor commands, not a recording of physical motor movement.

**Headset** contains connection, the direct four-burst test, Demo flight with a highlighted timeline, and the flight ceiling. **Settings** contains profiles and DCS integration, with diagnostics inside an expandable section. Effects run automatically with valid Hornet telemetry; there is no live-output toggle or required test.

## Default cues

Numbers below assume master 100%, curve 1 and ceiling 25. A range is the available command span; telemetry severity and gain determine the actual value.

| Cue | Default | Shape and strength | Detection / reason for inclusion |
|---|---|---|---|
| Gear up/down | On | 200 ms hit at 20 → 300 ms quiet → rising travel at 14–16 → 460 ms quiet → 250 ms hit at 19 | Position changes determine travel. Animation endpoints approximate completion. |
| Gun burst | On | 20–25 while ammunition falls; 180 ms hold after the last observed decrease, 60 ms settle, 300 ms quiet recovery | A sustained firing cue, since individual rounds arrive too quickly for separate motor pulses. |
| Touchdown | On | 15–25, 220 ms hit, 140 ms settle, 350 ms quiet recovery | Descending contact after at least 500 ms airborne, with a 1.1 s cooldown. Severity uses pre-contact vertical speed. Carrier-relative velocity is unavailable. |
| Afterburner onset | On | Kick at 21 for 180 ms, then roughly 14–16 rumble fading over 700 ms; 350 ms quiet | Engagement of either engine gives one combined cue. Hysteresis plus 1.4 s cooldown prevents repeated throttle chatter. Joining a flight already in AB gives no invented ignition. |
| Store release | On | Sharp 160 ms pulse at 20, then 280 ms quiet | Falling airborne store count. Includes jettison; does not identify a weapon or prove a successful launch. |
| Countermeasures | On | Sharp 140 ms pulse at 15, then 260 ms quiet | Falling flare or chaff count. Rapid releases coalesce into bounded cues rather than queue. |
| Airborne buffet | On | 12–18, 80 ms attack / 120 ms release; stop fade capped at 260 ms | General DCS shake while airborne. Useful airframe feedback, not an isolated stall or G-force warning. |
| Gear/brake airflow | On | Subtle 11–13, 180 ms attack / 120 ms release; stop fade capped at 320 ms | Airspeed and deployment while airborne. Kept separate from gear movement; yields during the mechanism's quiet phases. |
| Engine ambience | Off, available | Low 10–12, 400 ms attack / 160 ms release; stop fade capped at 450 ms | Optional RPM feedback for users without seat/stick haptics. Does not reproduce engine harmonics. |
| Runway bumps | Off, available | Low 10–12, 50 ms attack / 80 ms release; stop fade capped at 200 ms | Changing vertical acceleration while rolling. Steady acceleration produces no invented continuous buzz. |
| Damage / ejection | Unavailable | No output | Damage animation is not a reliable individual hit event. Ejection requires a validated own-aircraft event source. |

Store and countermeasure events arriving during their hit/recovery are consumed, never queued for a delayed pulse. A later event after recovery can produce another cue. Gunfire intentionally extends a burst while new rounds are observed. Missing required signals immediately stop their cue and establish fresh baselines when restored.

## Inertia and mixing

Impact and ignition cues have explicit finite envelopes followed by zero-command recovery. Software smoothing cannot fill these zero intervals. Lower-priority continuous cues also cannot fill a higher-priority cue's recovery; an urgent cue can still interrupt it. Priorities are touchdown 100, gun 90, stores 80, buffet 65, AB 60, countermeasures 50, gear 45, airflow 35, runway 15 and engine 5.

Continuous effects receive smoothing and bounded stop fades, rather than repeated gear-like impacts. The engine selects one dominant cue; it does not add motor frequencies or sum every effect into saturation. These defaults accommodate perceived inertia without claiming measured spin-up, braking or inverse physical compensation.

## Profiles and gear preset

Settings save automatically. **Settings → Profiles and presets → Apply default headset mix** applies the flight-cue defaults while keeping your gear tuning, demo travel, Master, ceiling and response curve. Profile import fits effect ranges to your local headset range.

**Apply gear preset** updates the gear cue with a start hit, quiet gap, rising travel rumble, coast and endpoint hit. It selects 4.8 seconds of demo travel per direction; you can set this from three to fifteen seconds. This controls the audition only. Live gear timing follows DCS telemetry.
