import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../data/esp32_repository.dart';
import '../data/http_esp32_repository.dart';
import '../domain/smart_home_state.dart';
import 'settings_controller.dart';

final esp32RepositoryProvider = Provider<Esp32Repository>((ref) {
  final settings = ref.watch(settingsControllerProvider);
  return HttpEsp32Repository(
    baseUrl: settings.baseUrl,
    client: ref.watch(httpClientProvider),
  );
});

final controllerProvider = NotifierProvider<ControllerNotifier, SmartHomeState>(
  ControllerNotifier.new,
);

class ControllerNotifier extends Notifier<SmartHomeState> {
  static const Duration _autoRefreshInterval = Duration(seconds: 1);

  Timer? _autoRefreshTimer;
  bool _refreshInFlight = false;
  bool _commandInFlight = false;

  @override
  SmartHomeState build() {
    final settings = ref.watch(settingsControllerProvider);
    final initial = SmartHomeState.seed();

    _autoRefreshTimer?.cancel();
    ref.onDispose(() => _autoRefreshTimer?.cancel());

    if (settings.isConfigured) {
      Future<void>.microtask(() => refresh(showBusy: false));
      _autoRefreshTimer = Timer.periodic(
        _autoRefreshInterval,
        (_) => refresh(showBusy: false),
      );
    }
    return initial;
  }

  Future<void> refresh({bool showBusy = true}) async {
    if (_refreshInFlight || _commandInFlight) {
      return;
    }

    if (!_ensureConfigured(
      'Configura prima il server remoto nelle impostazioni Endpoint.',
    )) {
      return;
    }

    _refreshInFlight = true;
    if (showBusy) {
      state = state.copyWith(isLoading: true, errorMessage: null);
    }

    try {
      final next =
          await ref.read(esp32RepositoryProvider).fetchState(fallback: state);
      state = next.copyWith(isLoading: false, errorMessage: null);
    } catch (error) {
      final message = 'Connessione fallita: $error';
      final shouldShowError = showBusy || !state.hasLiveData;
      final shouldAddEvent = shouldShowError && state.errorMessage != message;
      final next = state.copyWith(
        isLoading: false,
        errorMessage: shouldShowError ? message : null,
      );
      state = shouldAddEvent
          ? next.withEvent('Errore di sincronizzazione con il server')
          : next;
    } finally {
      _refreshInFlight = false;
    }
  }

  Future<void> runCommand(
    ControllerCommand command, {
    Map<String, dynamic>? params,
  }) async {
    if (!_ensureConfigured(
      'Configura il server prima di inviare comandi all ESP32.',
    )) {
      return;
    }

    _commandInFlight = true;
    state = state.copyWith(isLoading: true, errorMessage: null);

    try {
      final next = await ref
          .read(esp32RepositoryProvider)
          .sendCommand(command, state, params: params);
      state = next.copyWith(isLoading: false, errorMessage: null);
    } catch (error) {
      state = state
          .copyWith(
            isLoading: false,
            errorMessage: 'Comando non inviato: $error',
          )
          .withEvent('Invio comando fallito verso il server remoto');
    } finally {
      _commandInFlight = false;
    }
  }

  bool _ensureConfigured(String message) {
    final settings = ref.read(settingsControllerProvider);
    if (settings.isConfigured) {
      return true;
    }

    state = state.copyWith(isLoading: false, errorMessage: message);
    return false;
  }
}
