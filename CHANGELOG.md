# Changelog

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
