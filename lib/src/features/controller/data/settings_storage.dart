import 'package:shared_preferences/shared_preferences.dart';

import '../domain/esp32_settings.dart';

class SettingsStorage {
  SettingsStorage() : _preferences = SharedPreferencesAsync();

  static const _baseUrlKey = 'base_url';

  final SharedPreferencesAsync _preferences;

  Future<Esp32Settings> load() async {
    final baseUrl = await _preferences.getString(_baseUrlKey) ?? '';

    return Esp32Settings(baseUrl: baseUrl);
  }

  Future<void> save(Esp32Settings settings) async {
    await _preferences.setString(_baseUrlKey, settings.baseUrl);
  }
}
