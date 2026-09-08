# PSVR2SimShaker

<img src="assets/PSVR2SimShaker.png" alt="SimShaker S and vibration ripples" width="96" height="96">

Independent DCS World headset haptics for PlayStation VR2, by Adam Chesters.

**0.1.1 — public preview.** Windows x64; F/A-18C Hornet. PSVR2SimShaker reads DCS through its own export hook and shared-memory bridge. It works alongside seat and joystick haptics without requiring SimHaptic or TelemFFB.

## Current features

- Gear movement, gun bursts, touchdown, afterburner ignition, store release, countermeasures, airborne buffet and subtle gear/speedbrake airflow.
- Optional engine and runway ambience, disabled by default.
- Per-effect switches, strength controls, isolated tests and expandable timing/response controls.
- Master, Mute, headset strength ceiling and an emergency-stop shortcut.
- A direct four-burst headset test and a demo flight with a highlighted timeline, using your enabled mix and saved gear travel time.
- Automatic effect output when valid Hornet telemetry and the headset are ready.
- Persistent indicators for the DCS hook, advancing simulator telemetry and aircraft data.
- Importable/exportable tuning profiles, connection diagnostics and a step-by-step `vr2jb.exe` guide.
- Per-user installer and portable ZIP. Closing the window keeps the app in the tray.

## Getting started

Download the installer or portable ZIP from [Releases](https://github.com/AdamChesters/PSVR2SimShaker/releases). Install the DCS hook from **Settings → DCS integration**.

The headset requires compatible experimental [PSVR2Toolkit](https://github.com/BnuuySolutions/PSVR2Toolkit) and a per-power-session rumble unlock. Follow the [setup guide](docs/SETUP.md): close DCS/SteamVR/SimShaker, run `vr2jb.exe` with the headset awake, start SteamVR, then launch SimShaker and a Hornet mission. Master and Mute control output; tests are optional.

The Toolkit exposes a single motor command from 10–25, plus zero for stop. This app shapes those commands into cues; it does not send audio samples to the headset. Damage/ejection effects are unavailable. Hornet signal quality, multiplayer restrictions and carrier behaviour need live-flight validation.

## Documentation

- [Setup and connection guide](docs/SETUP.md)
- [Effects and tuning](docs/EFFECTS.md)
- [Headset capabilities](docs/HARDWARE.md)
- [DCS signal reference](docs/TELEMETRY.md)
- [Shared-memory protocol](docs/PROTOCOL.md)
- [Testing and known limits](docs/TESTING.md)
- [Changelog](CHANGELOG.md)
- [Deferred features](docs/FUTURE_WORK.md)

## Build

Install Visual Studio 2026 C++ tools, CMake and vcpkg. Set `VCPKG_ROOT` to your vcpkg checkout:

```powershell
cmake --preset windows
cmake --build --preset release
ctest --preset release
```

Dependencies are pinned in `vcpkg.json`. For the installer and portable ZIP, install Inno Setup 6 and run `scripts/package.ps1`; use `-InnoCompiler` to select ISCC.exe if needed. No Sony firmware or Toolkit driver binaries are bundled.

## License and credits

GPL-3.0. See [third-party notices](THIRD_PARTY_NOTICES.md) and `licenses/` for upstream credits and licenses.
