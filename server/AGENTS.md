# PHP server agent instructions

- Treat `api/state.php`, `api/command.php`, and `api/device_state.php` as a shared contract with Flutter and both ESP32 client paths. Preserve field types, command meaning, error responses, and desired-versus-reported state semantics.
- Preserve the configured bearer-token checks on every API route, bounded request parsing, restrictive storage permissions, and `storage/.htaccess`. Do not disable authorization or open CORS to work around a client failure.
- Keep concurrent JSON-storage updates coordinated and failures explicit. Never replace a failed persistence operation with an apparently successful response.
- Tests must use disposable storage and fake clients, not the live `storage/state.json` or connected hardware. Do not expose tokens or operational data in responses, logs, or fixtures.

Use [README.md](../README.md) for supported hosting and endpoint conventions. Lint changed PHP files with `php -l` and exercise affected request/authentication/persistence paths in an isolated instance. PHP syntax checks do not establish API behavior, web-server access rules, or physical actuation. Hosted deployment and real commands are separate authorized operations.
