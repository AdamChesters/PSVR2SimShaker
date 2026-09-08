# Testing and known limits

## Automated checks

The Windows x64 Release build passes three CTest suites:

- **shaker_tests:** profile and packet validation; real-feed freshness and aircraft/session state; event baselines; cue availability; effect envelopes and priorities; motor bounds; gear start/travel/lock and quiet recovery; demo timeline alignment; export install/repair/remove preservation.
- **bridge_tests:** shared-memory identity and bounds, writer exclusion, session restart, contention, close/reopen and native module exports.
- **host_tests:** output helper with a test-only Toolkit substitute, command lease expiry, explicit zero, inactive runtime, fault handling and timestamp boundaries.

Full demo sequences are checked with 4.8, 12.1 and 15 seconds of gear travel per direction. All eight enabled cues, both gear strokes, disabled cues and quiet phases are exercised. Test binaries are not packaged.

The Lua exporter has also been exercised against the DCS Lua runtime using controlled API fixtures for callback chaining, throttling, missing values and payload station validation.

## App and hardware checks

The per-user installer and portable ZIP build successfully. Native UI checks cover the large repository link, top connection indicators, effect controls, demo timeline and completion, connection-guide wrapping and Settings. The full demo reaches completion without changing real DCS connection indicators. Helper logs confirm the final zero command was acknowledged.

Headset vibration has been confirmed through the app and the Toolkit test application. Gear sequencing has been physically auditioned. Motor command acknowledgements do not measure vibration frequency, force or braking.

## Live-flight validation

The public preview still needs Hornet flight testing for signal quality, cue timing and strength. Check ground roll, takeoff/gear, guns, buffet, airbrake, afterburner, store release, countermeasures and landing. Also check pause/menu/mission transitions, reconnect, multiplayer export restrictions and carrier operations.

Touchdown severity uses vertical speed without carrier deck-relative correction. Store count reductions include jettison. Unlimited-ammunition settings may prevent gun detection. Damage and ejection are unavailable.

The headset must be unlocked for its current power session and connected through compatible PSVR2Toolkit. A stalled driver can prevent a physical stop even when the app requests zero; the app does not claim a hardware watchdog or measured motor braking.
