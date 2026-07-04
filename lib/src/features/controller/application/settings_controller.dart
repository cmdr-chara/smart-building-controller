import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:http/http.dart' as http;

import '../data/settings_storage.dart';
import '../domain/esp32_settings.dart';

final settingsStorageProvider = Provider<SettingsStorage>(
  (ref) => throw UnimplementedError('SettingsStorage non inizializzato'),
);

final initialSettingsProvider = Provider<Esp32Settings>(
  (ref) => const Esp32Settings(),
);

final httpClientProvider = Provider<http.Client>((ref) {
  final client = http.Client();
  ref.onDispose(client.close);
  return client;
});

final settingsControllerProvider =
    NotifierProvider<SettingsController, Esp32Settings>(SettingsController.new);

class SettingsController extends Notifier<Esp32Settings> {
  @override
  Esp32Settings build() {
    return ref.watch(initialSettingsProvider);
  }

  Future<void> updateConnection({required String baseUrl}) async {
    final normalized = _normalizeBaseUrl(baseUrl);
    state = state.copyWith(baseUrl: normalized);

    await ref.read(settingsStorageProvider).save(state);
  }

  String _normalizeBaseUrl(String input) {
    var value = input.trim();
    if (value.isEmpty) {
      return '';
    }
    if (!value.contains('://')) {
      value = 'http://$value';
    }
    if (value.endsWith('/')) {
      value = value.substring(0, value.length - 1);
    }
    return value;
  }
}
