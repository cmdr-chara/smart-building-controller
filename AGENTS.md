# Smart Building Controller agent instructions

## System contracts

- Coordinate Flutter, PHP, ESP32, and Arduino changes across their producers and consumers. API fields and serial `CMD;...` / `STATE;...` messages are compatibility contracts, not independently editable UI details.
- Keep desired commands distinct from reported device state. Sending a request or receiving HTTP success does not prove that an actuator moved or a sensor produced a valid reading.
- The `accessi_allarme` ESP32 module talks to PHP directly; do not assume every module uses the serial bridge.
- Preserve bearer authentication for reads, commands, and device-state writes, remote HTTPS enforcement, request bounds, and storage protection. The compiled client token is prototype access control, not production per-user authentication.
- Keep tokens, Wi-Fi credentials, device identifiers, and real operational state out of examples and tests. Changes must not weaken existing access, storage, or transport protections.

## Guidance and completion

Use [README.md](README.md) for setup/protocol behavior and [docs/schema_elettrico.md](docs/schema_elettrico.md) for wiring changes. Keep board pin tables and affected firmware consumers synchronized. Use `flutter analyze` and `flutter test` for app behavior changes; server and firmware require their own relevant checks.

Automated tests use fake endpoints, temporary state, and simulated hardware. Firmware upload, commands to real actuators, alarm/access changes, and hosted deployment need explicit scope; repository editing does not authorize them.

Finish with affected cross-component contracts and failure paths checked, documentation updated, and actual host/build/device limitations reported. A Flutter test or firmware compile is not a physical-system safety or hardware-functionality certification.
