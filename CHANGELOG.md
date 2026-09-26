# Changelog

## 0.3.0 Alpha 3

- Add a Tests pane with steady rumble, sweep, single pulse, double knock, pulsing rumble and pulse-over-rumble controls.
- Compare all tests on the same six-second command graph. Adjust strength, length and gaps in 20 ms steps. Tests respect the ceiling and pause DCS output while the pane is open.
- Resume active background rumble as soon as a foreground event finishes. Gear's deliberate internal gaps remain quiet.
- Show this changelog once on the first launch after an update. Reopen it from the Version dialog.
- Aircraft profiles and effects still need detailed live-flight testing. Command graphs do not measure physical vibration.

## 0.3.0 Alpha 2

- Recognize the official PSVR2Toolkit 1.0.0 Experimental 2 Windows CAPI DLL, fixing the unvalidated-toolkit output fault for that release.
- Aircraft effects are unchanged; new profiles and carrier cues still require live-flight/headset testing.

## 0.3.0 Alpha 1

- Add Viper, A-10C/II, Tomcat, Phantom and Apache telemetry profiles with automatically selected, independently saved family tuning.
- Handle Viper single-engine afterburner, Tomcat speedbrake mapping and Apache fixed-gear/non-afterburning capabilities.
- Add an inferred Hornet/Tomcat catapult launch cue with baseline, acceleration and timeout guards.
- Scale touchdown strength with descent rate to full at 500 ft/min, capped above that; below 60 ft/min stays quiet.
- Add aircraft, persistence, landing, catapult and exporter regression tests.
- New mappings and effects require Windows live-flight/headset QA; see docs/AIRCRAFT.md.

## 0.2.3 Alpha

- Show the application logo at the top left beside the title. The logo is embedded in the executable and restored when the graphics device is recreated.

## 0.2.2 Alpha

- Align the small GitHub link to the title baseline.
- Put version status first in both the four-column row and narrow list.

## 0.2.1 Alpha

- Compact four-column status strip, collapsing into a list in narrower windows.
- Version status joins the connection lights: green when current, flashing yellow with yellow text when an update is available.
- Small GitHub label beside the linked application title.

## 0.2 Alpha

- Check published GitHub releases at launch, including Alpha releases, independently of haptic output.
- Green current-version and yellow update-available status in the title area, with click-to-confirm installation and download progress.
- Verify installer size and SHA-256 against GitHub release metadata; preserve saved tuning and the current installation folder.
- Prominent Alpha/tuning guidance, prerequisites, required `vr2jb.exe` → SteamVR → PSVR2SimShaker launch order and potential features in the README.
- Separate sustained afterburner rumble with independent strength, response and priority controls, plus an extended afterburner demo stage.
- Airborne buffet stays enabled at a lower priority so flight events, afterburner and configuration airflow can take over.
- Older profiles using buffet's original default priority are migrated once; other tuning and custom priorities are retained.

## 0.1.1

- Original SimShaker icon for the app, taskbar, system tray and installer.
- PNG master and multi-size Windows ICO included with the source and downloads.

## 0.1.0 — Initial public preview

- Independent DCS export and shared-memory telemetry for the F/A-18C Hornet.
- Eight enabled flight cues, with optional engine and runway ambience.
- Per-effect tuning, master/mute controls and a headset strength ceiling.
- Individual cue tests, four-burst motor test and a demo flight with a live timeline.
- DCS connection indicators, profile import/export and diagnostics.
- Guided headset startup, per-user installer and portable distribution.

Hornet flight testing is ongoing. See [testing and known limits](docs/TESTING.md).
