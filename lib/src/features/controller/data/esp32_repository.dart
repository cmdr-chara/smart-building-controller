import '../domain/smart_home_state.dart';

enum ControllerCommand {
  openInternalDoor('openInternalDoor'),
  toggleAlarm('toggleAlarm'),
  toggleRfid('toggleRfid'),
  openParkingBarrier('openParkingBarrier'),
  vehicleEntered('vehicleEntered'),
  vehicleExited('vehicleExited'),
  toggleExteriorLights('toggleExteriorLights'),
  toggleAwning('toggleAwning'),
  syncExteriorAutomation('syncExteriorAutomation'),
  toggleFan('toggleFan'),
  toggleWindows('toggleWindows'),
  playBuzzer('playBuzzer'),
  playDoomBuzzer('playDoomBuzzer'),
  playFnafMusicBox('playFnafMusicBox'),
  playToreadorMarch('playToreadorMarch'),
  playMegaBoss('playMegaBoss'),
  playAmbulanceSiren('playAmbulanceSiren'),
  playSelectedBuzzer('playSelectedBuzzer'),
  stopBuzzer('stopBuzzer'),
  syncClimateAutomation('syncClimateAutomation'),
  toggleIndoorLights('toggleIndoorLights'),
  syncPresenceLighting('syncPresenceLighting');

  const ControllerCommand(this.apiValue);

  final String apiValue;
}

abstract class Esp32Repository {
  Future<SmartHomeState> fetchState({SmartHomeState? fallback});

  Future<SmartHomeState> sendCommand(
    ControllerCommand command,
    SmartHomeState current, {
    Map<String, dynamic>? params,
  });
}
