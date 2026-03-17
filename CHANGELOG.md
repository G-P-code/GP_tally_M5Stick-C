# Changelog

## [4.5.2] — 2026-03-17

### Fixed
- **Reassign tally state** — tally state (`actualType`, `actualColor`, LEDs) is now cleared immediately in `socket_Reassign()` before the new device name is set. Previously the stale colour from the previous device persisted until the next `device_states` event arrived.
- **Race condition on reassign** — `device_states` from TA can arrive before `devices` event completes device name update. Tally state is now reset at the correct point in the reassign sequence so stale state never bleeds into the new device display.
- **`bus_options` race on connect** — TA sends `device_states` before `bus_options` on initial connect. Added `busOptionsReady` flag so `processTallyData()` is deferred until bus colour and priority data is available. Buffered `DeviceStates` are processed immediately when `bus_options` arrives.
- **Device name update** — screen repaints immediately when TallyArbiter assigns a new device name.
- 
