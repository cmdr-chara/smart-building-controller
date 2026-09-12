# Firmware agent instructions

- Preserve module identity, target board, pin mapping, voltage/interface assumptions, and the API/serial contract consumed by PHP and Flutter. Check the wiring source before changing a pin or peripheral.
- Keep requested actuator state separate from actual feedback. Missing, stale, or failed sensor readings must not be reported as valid observations.
- Preserve existing timing, command bounds, and fail-safe behavior; a convenience change must not silently energize a motor, barrier, window, buzzer, or alarm path.
- `accessi_allarme` is a direct Wi-Fi/API client; other modules may use the ESP32 serial bridge. Validate the actual producer/consumer path for the changed sketch.
- Do not commit Wi-Fi/API secrets. Use inert credentials and simulated inputs for host checks.

Use [the module setup notes](../README.md) and [wiring tables](../docs/schema_elettrico.md) for the matching board and libraries. Compile only for the intended target and report any unavailable toolchain. Uploading, resetting boards, or exercising live actuators requires separately scoped authorization. A successful compile is not evidence that wiring, sensors, or actuation is safe or functional.
