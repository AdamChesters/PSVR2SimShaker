# Testing and known limits

## Automated checks

The Windows x64 Release build has four CTest suites:

- **shaker_tests:** profile and packet validation; real-feed freshness and aircraft/session state; event baselines; cue availability; effect envelopes and priorities; motor bounds; gear start/travel/lock and quiet recovery; demo timeline alignment; export install/repair/remove preservation.
- **bridge_tests:** shared-memory identity and bounds, writer exclusion, session restart, contention, close/reopen and native module exports.
- **host_tests:** output helper with a test-only Toolkit substitute, command lease expiry, explicit zero, inactive runtime, fault handling and timestamp boundaries.
- **update_tests:** Alpha/SemVer ordering, published-release selection, download host restrictions, missing assets/checksums, corrupt or truncated installers and cancellation cleanup.

The optional `update_tests --live <download-folder>` check retrieves the newest public release and downloads/verifies its installer without launching it. This is separate from offline CTest runs and does not access the headset.

For 0.2 Alpha, the public GitHub lookup and installer checksum check passed. Current, available, downloading and offline UI states were rendered offscreen at the minimum window size to check colours and text wrapping, with hardware and settings writes disabled. The installer handoff has not been directly observed against a running user's installation. A manual 0.2 Alpha installation with the app closed preserved the saved settings; a later installed binary was verified as 0.2.3 Alpha. That version check alone does not validate the in-app updater handoff.

Full demo sequences are checked with 4.8, 12.1 and 15 seconds of gear travel per direction. All nine enabled cues, both gear strokes, disabled cues and quiet phases are exercised. Test binaries are not packaged.

The Lua exporter has also been exercised against the DCS Lua runtime using controlled API fixtures for callback chaining, throttling, missing values and payload station validation.

Sustained-cue tests include 30 seconds in afterburner, a single ignition across both engines, temporary takeover by gunfire, resuming the rumble, independent enable/strength controls, profile migration, missing signals, a new session already in afterburner, stale telemetry and leaving afterburner. Gear and speedbrake airflow are each exercised beyond deployment with ground contact and priority suppression.

Priority regression tests overlay strong buffet on every event and configuration cue, including deliberate quiet recovery. Migration tests preserve other tuning, custom priorities and subsequent user edits.

The 0.2.3 Alpha release passed all four CTest suites and Windows CI. Subsequent UI checks covered the embedded application logo, baseline-aligned GitHub link, Version-first status order, wide four-column and narrow list layouts, flashing/dim update indicator states, and update-dialog wrapping. These visual checks used offscreen rendering; they do not establish live-flight behaviour.

## App and hardware checks

The per-user installer and portable ZIP build successfully. Native UI checks cover the large repository link, top connection indicators, effect controls, demo timeline and completion, connection-guide wrapping and Settings. The full demo reaches completion without changing real DCS connection indicators. Helper logs confirm the final zero command was acknowledged.

Headset vibration has been confirmed through the app and the Toolkit test application. Gear sequencing has been physically auditioned. Motor command acknowledgements do not measure vibration frequency, force or braking.

## Live-flight validation

Hornet flight testing has begun: gunfire, the afterburner onset kick and some gear movement have been felt. Gear-down strength, configuration airflow, countermeasures and the new sustained afterburner effect need further in-flight tuning and validation. Check ground roll, takeoff/gear, guns, buffet, airbrake, afterburner, store release, countermeasures and landing. Also check pause/menu/mission transitions, reconnect, multiplayer export restrictions and carrier operations.

Touchdown severity uses vertical speed without carrier deck-relative correction. Store count reductions include jettison. Unlimited-ammunition settings may prevent gun detection. Damage and ejection are unavailable.

The headset must be unlocked for its current power session and connected through compatible PSVR2Toolkit. A stalled driver can prevent a physical stop even when the app requests zero; the app does not claim a hardware watchdog or measured motor braking.
