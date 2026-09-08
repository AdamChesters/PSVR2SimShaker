# Installation and first flight

## Requirements

- Windows x64 and DCS World with the F/A-18C Hornet.
- PSVR2 connected through its supported PC setup, with SteamVR working.
- A compatible experimental PSVR2Toolkit driver and its matching CAPI DLL.
- Headset rumble unlocked for the current power session. See the upstream [installation guide](https://github.com/BnuuySolutions/PSVR2Toolkit/wiki/Installation) and [headset-unlock guide](https://github.com/BnuuySolutions/PSVR2Toolkit/wiki/Jailbreaking-your-headset).

## Start a headset session

**Headset haptics WILL NOT WORK unless `vr2jb.exe` has successfully unlocked the headset for the current power session. Required order: `vr2jb.exe` → SteamVR → PSVR2SimShaker → DCS.**

For a headset already configured with compatible PSVR2Toolkit and firmware 6.00, use **`vr2jb.exe` from [vr2jb v1.0.1](https://github.com/BnuuySolutions/vr2jb/releases/tag/v1.0.1)**:

1. Exit **DCS, SteamVR, the PlayStation VR2 App and PSVR2SimShaker**. In SimShaker use **Settings → Exit application** or the tray's **Exit**. The window's X only hides it.
2. Turn on the headset and keep it awake. Leave SteamVR closed.
3. Open the extracted `vr2jb-windows-linux-builds-v1.0.1` folder and run **`vr2jb.exe` with no arguments**. Wait for success. The console waits about eight seconds before closing; a white LED blink every two seconds indicates the unlock.
4. Start **SteamVR** and wait until its headset indicator shows connected.
5. Open **PSVR2SimShaker**, then start a **DCS Hornet mission**. Flight effects run automatically; tests are optional.

Repeat this sequence after a red-LED headset shutdown. The helper and Toolkit test app do not need to remain open. These are per-session startup steps; first-time firmware setup is described in the [official headset guide](https://github.com/BnuuySolutions/PSVR2Toolkit/wiki/Jailbreaking-your-headset). SimShaker does not flash firmware or run the unlock automatically.

The same steps appear in **Headset → Connection help**, with **Copy startup steps** so you can keep them after exiting the app, plus download and setup links.

## First use

1. Run the setup executable. It installs for the current Windows user without administrator rights. A portable ZIP is also available.
2. Open **Settings → DCS integration** and install the export into your DCS Saved Games profile. Restart DCS if it was running. Installation keeps a backup of the previous `Export.lua` and appends an identified hook; existing exporters remain in place.
3. Follow the headset startup steps above. Start a Hornet mission. Output follows advancing simulator time and stops on stale telemetry; there is no live-output checkbox or test-confirmation requirement.
4. In **Effects**, use Master, Mute and each row's switch/peak strength. **Test** auditions a cue alone; **Tune** expands its options. **Headset → Test headset** and **Demo flight** are available whenever you want to explore the output.
5. Adjust your enabled cues and strengths in **Effects**, or choose a preset in **Settings → Profiles and presets**. See the [effect guide](EFFECTS.md).

The gear **Test** uses both directions and excludes gear drag. **Tune → Demo travel per direction (s)** controls audition length; live travel follows DCS. **Rhythm** contains the start/lock hits, coast gaps and travel ramp. **Apply gear preset** selects 4.8-second demo travel per direction (12.2 seconds total). Live gear movement always follows telemetry.

## Status lights and tests

The top bar remains visible on every page:

- **DCS hook** checks the marked export entry and both owned Lua/DLL files in detected/saved profiles. A partial installation is not marked installed. If only some profiles have the hook, the indicator reports the count.
- **DCS telemetry** lights for recent shared-memory packets with an advancing simulator clock. It returns to waiting when DCS pauses, stops or goes stale.
- **Aircraft** additionally requires a flying state, aircraft identity and numeric signals. Unsupported aircraft are labelled explicitly; only the Hornet drives effects. Individual cues require their own signals.

Tests never light the real DCS indicators. The direct headset test plays four 500 ms bursts with 250 ms gaps at the selected strength, independently of the flight ceiling. Effect tests and Demo flight respect Master and the ceiling. Pressing a test button unmutes; **STOP** mutes and cancels it. **Stop test** ends the test and lets available DCS effects resume.

**Headset → Demo flight** follows your enabled mix: ground roll → gear up → gun burst → airborne buffet → airbrake airflow → afterburner ignition and sustained rumble → store release → countermeasures → gear down → touchdown → finish. The timeline highlights the current stage and marks completed ones. Its timings come from the same schedule as the generated telemetry, using your saved gear travel time. It waits for the headset before starting. Optional engine/runway cues stay quiet unless enabled. No DCS mission is needed.

## Flight operation

Launch the app after completing the headset unlock and starting SteamVR. It opens the headset connection when DCS sends live Hornet telemetry. Windows startup is not offered because the unlock must come first. Closing the window leaves the app in the tray, with its graphics resources released. Exit completely using **Settings → Exit application** or the tray menu.

The headset must still be awake and unlocked, and SteamVR must be ready. This is a current PSVR2Toolkit hardware prerequisite; the installer cannot eliminate it. It is not necessary to start TelemFFB, SimHaptic, or a separate telemetry server.

## Stop and reconnect

Use **STOP**, the tray's stop menu, or **Ctrl+Alt+Space**. The app reports if another program already owns the shortcut. Clear Mute explicitly to resume. A telemetry cutoff defaults to 300 ms. A separate output helper has a 300 ms command lease and sends zero when the app stops refreshing it.

If the toolkit times out, output is faulted until reconnect. A driver hung inside a call can prevent a physical stop; neither this app nor a successful API response measures the motor. Recover the VR runtime before reconnecting.

## Profiles and diagnostics

Settings save under `%LOCALAPPDATA%\PSVR2SimShaker`. Exported tuning profiles omit machine paths and mute state. Recording and replay are not included.

Diagnostic reports include toolkit hash, application status and current effect signals. Review a report before sharing it. No telemetry or tuning is uploaded. The app contacts GitHub to check public releases at launch; an installer is downloaded only when you choose to update.

## Updating

The **Version** status light and text are green when no newer release is published. When an update is available, its light flashes yellow and its text stays yellow. It sits beside the three connection lights in a compact four-column row, which collapses into a list in narrower windows. Alpha releases are included. Click the indicator to review the release, then choose **Download and install**. Close DCS before installing so it can release the export bridge DLL. The app verifies the downloaded installer's size and SHA-256 against GitHub release metadata, then opens the installer and exits. Your saved tuning and installation folder are retained; select **Open PSVR2SimShaker** at the end to restart the app. Updating does not replace the headset unlock or SteamVR startup steps.

If the check fails, the indicator stays grey, with **Check again** and **Open Releases** available when clicked. A release without a verifiable installer can be downloaded manually from its page. Portable users can install into the current folder or update manually from the ZIP. Checks use GitHub's public API without an account or token; network errors and rate limits do not interrupt flight effects.

## Removal

Use **Settings → DCS integration → Remove our export** for a selected DCS profile, or uninstall the application. Only the identified PSVR2SimShaker hook and its owned files are removed. The original export backup and your tuning files are retained. Close DCS before uninstalling so Windows can release the loaded bridge DLL.

## Current limitations

This is an early preview. Individual Hornet signals need in-flight validation across missions and multiplayer export policies. Runway texture and touchdown severity are approximations; carrier-relative velocity is not available. Stores-count changes cannot distinguish release from jettison. Toolkit DLL builds require an entry in the compatibility list because earlier versions exported identical names with incompatible return types. Do not simply bypass this check for an untested binary.
