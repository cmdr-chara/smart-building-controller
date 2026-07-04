const Object _unset = Object();

class SystemEvent {
  const SystemEvent({required this.title, required this.createdAt});

  final String title;
  final DateTime createdAt;

  factory SystemEvent.record(String title) {
    return SystemEvent(title: title, createdAt: DateTime.now());
  }
}

class SmartHomeState {
  SmartHomeState({
    required this.internalDoorUnlocked,
    required this.intrusionAlarmArmed,
    required this.rfidReaderEnabled,
    required this.parkingBarrierOpen,
    required this.vehicleDetected,
    required this.parkingCapacity,
    required this.occupiedSpots,
    required this.twilightDetected,
    required this.exteriorLightsOn,
    required this.awningOpen,
    required this.temperature,
    required this.humidity,
    required this.fanOn,
    required this.windowsOpen,
    required this.buzzerMelody,
    required this.doomBuzzerEnabled,
    required this.motionDetected,
    required this.indoorLightsOn,
    required this.lcdEnabled,
    required this.lastUpdated,
    required this.events,
    required this.isLoading,
    required this.hasLiveData,
    this.errorMessage,
  });

  final bool internalDoorUnlocked;
  final bool intrusionAlarmArmed;
  final bool rfidReaderEnabled;
  final bool parkingBarrierOpen;
  final bool vehicleDetected;
  final int parkingCapacity;
  final int occupiedSpots;
  final bool twilightDetected;
  final bool exteriorLightsOn;
  final bool awningOpen;
  final double temperature;
  final double humidity;
  final bool fanOn;
  final bool windowsOpen;
  final String buzzerMelody;
  final bool doomBuzzerEnabled;
  final bool motionDetected;
  final bool indoorLightsOn;
  final bool lcdEnabled;
  final DateTime lastUpdated;
  final List<SystemEvent> events;
  final bool isLoading;
  final bool hasLiveData;
  final String? errorMessage;

  int get availableSpots => parkingCapacity - occupiedSpots;

  double get parkingUsage {
    if (parkingCapacity == 0) {
      return 0;
    }
    return occupiedSpots / parkingCapacity;
  }

  factory SmartHomeState.seed() {
    return SmartHomeState(
      internalDoorUnlocked: false,
      intrusionAlarmArmed: false,
      rfidReaderEnabled: false,
      parkingBarrierOpen: false,
      vehicleDetected: false,
      parkingCapacity: 0,
      occupiedSpots: 0,
      twilightDetected: false,
      exteriorLightsOn: false,
      awningOpen: false,
      temperature: 0,
      humidity: 0,
      fanOn: false,
      windowsOpen: false,
      buzzerMelody: 'musicBox',
      doomBuzzerEnabled: false,
      motionDetected: false,
      indoorLightsOn: false,
      lcdEnabled: false,
      lastUpdated: DateTime.now(),
      events: <SystemEvent>[SystemEvent.record('In attesa del server remoto')],
      isLoading: false,
      hasLiveData: false,
    );
  }

  factory SmartHomeState.fromJson(
    Map<String, dynamic> json, {
    SmartHomeState? fallback,
  }) {
    final base = fallback ?? SmartHomeState.seed();

    return base.copyWith(
      internalDoorUnlocked: _readBool(
        json['internalDoorUnlocked'],
        base.internalDoorUnlocked,
      ),
      intrusionAlarmArmed: _readBool(
        json['intrusionAlarmArmed'],
        base.intrusionAlarmArmed,
      ),
      rfidReaderEnabled: _readBool(
        json['rfidReaderEnabled'],
        base.rfidReaderEnabled,
      ),
      parkingBarrierOpen: _readBool(
        json['parkingBarrierOpen'],
        base.parkingBarrierOpen,
      ),
      vehicleDetected: _readBool(json['vehicleDetected'], base.vehicleDetected),
      parkingCapacity: _readInt(json['parkingCapacity'], base.parkingCapacity),
      occupiedSpots: _readInt(json['occupiedSpots'], base.occupiedSpots),
      twilightDetected: _readBool(
        json['twilightDetected'],
        base.twilightDetected,
      ),
      exteriorLightsOn: _readBool(
        json['exteriorLightsOn'],
        base.exteriorLightsOn,
      ),
      awningOpen: _readBool(json['awningOpen'], base.awningOpen),
      temperature: _readDouble(json['temperature'], base.temperature),
      humidity: _readDouble(json['humidity'], base.humidity),
      fanOn: _readBool(json['fanOn'], base.fanOn),
      windowsOpen: _readBool(json['windowsOpen'], base.windowsOpen),
      buzzerMelody: _readString(json['buzzerMelody'], base.buzzerMelody),
      doomBuzzerEnabled: _readBool(
        json['doomBuzzerEnabled'],
        base.doomBuzzerEnabled,
      ),
      motionDetected: _readBool(json['motionDetected'], base.motionDetected),
      indoorLightsOn: _readBool(json['indoorLightsOn'], base.indoorLightsOn),
      lcdEnabled: _readBool(json['lcdEnabled'], base.lcdEnabled),
      lastUpdated: DateTime.tryParse(json['lastUpdated'] as String? ?? '') ??
          DateTime.now(),
      isLoading: false,
      hasLiveData: true,
      errorMessage: null,
    );
  }

  SmartHomeState copyWith({
    bool? internalDoorUnlocked,
    bool? intrusionAlarmArmed,
    bool? rfidReaderEnabled,
    bool? parkingBarrierOpen,
    bool? vehicleDetected,
    int? parkingCapacity,
    int? occupiedSpots,
    bool? twilightDetected,
    bool? exteriorLightsOn,
    bool? awningOpen,
    double? temperature,
    double? humidity,
    bool? fanOn,
    bool? windowsOpen,
    String? buzzerMelody,
    bool? doomBuzzerEnabled,
    bool? motionDetected,
    bool? indoorLightsOn,
    bool? lcdEnabled,
    DateTime? lastUpdated,
    List<SystemEvent>? events,
    bool? isLoading,
    bool? hasLiveData,
    Object? errorMessage = _unset,
  }) {
    return SmartHomeState(
      internalDoorUnlocked: internalDoorUnlocked ?? this.internalDoorUnlocked,
      intrusionAlarmArmed: intrusionAlarmArmed ?? this.intrusionAlarmArmed,
      rfidReaderEnabled: rfidReaderEnabled ?? this.rfidReaderEnabled,
      parkingBarrierOpen: parkingBarrierOpen ?? this.parkingBarrierOpen,
      vehicleDetected: vehicleDetected ?? this.vehicleDetected,
      parkingCapacity: parkingCapacity ?? this.parkingCapacity,
      occupiedSpots: occupiedSpots ?? this.occupiedSpots,
      twilightDetected: twilightDetected ?? this.twilightDetected,
      exteriorLightsOn: exteriorLightsOn ?? this.exteriorLightsOn,
      awningOpen: awningOpen ?? this.awningOpen,
      temperature: temperature ?? this.temperature,
      humidity: humidity ?? this.humidity,
      fanOn: fanOn ?? this.fanOn,
      windowsOpen: windowsOpen ?? this.windowsOpen,
      buzzerMelody: buzzerMelody ?? this.buzzerMelody,
      doomBuzzerEnabled: doomBuzzerEnabled ?? this.doomBuzzerEnabled,
      motionDetected: motionDetected ?? this.motionDetected,
      indoorLightsOn: indoorLightsOn ?? this.indoorLightsOn,
      lcdEnabled: lcdEnabled ?? this.lcdEnabled,
      lastUpdated: lastUpdated ?? this.lastUpdated,
      events: events ?? this.events,
      isLoading: isLoading ?? this.isLoading,
      hasLiveData: hasLiveData ?? this.hasLiveData,
      errorMessage:
          errorMessage == _unset ? this.errorMessage : errorMessage as String?,
    );
  }

  SmartHomeState withEvent(String message) {
    return copyWith(
      lastUpdated: DateTime.now(),
      events: <SystemEvent>[SystemEvent.record(message), ...events.take(9)],
    );
  }

  Map<String, dynamic> toJson() {
    return <String, dynamic>{
      'internalDoorUnlocked': internalDoorUnlocked,
      'intrusionAlarmArmed': intrusionAlarmArmed,
      'rfidReaderEnabled': rfidReaderEnabled,
      'parkingBarrierOpen': parkingBarrierOpen,
      'vehicleDetected': vehicleDetected,
      'parkingCapacity': parkingCapacity,
      'occupiedSpots': occupiedSpots,
      'twilightDetected': twilightDetected,
      'exteriorLightsOn': exteriorLightsOn,
      'awningOpen': awningOpen,
      'temperature': temperature,
      'humidity': humidity,
      'fanOn': fanOn,
      'windowsOpen': windowsOpen,
      'buzzerMelody': buzzerMelody,
      'doomBuzzerEnabled': doomBuzzerEnabled,
      'motionDetected': motionDetected,
      'indoorLightsOn': indoorLightsOn,
      'lcdEnabled': lcdEnabled,
      'lastUpdated': lastUpdated.toIso8601String(),
    };
  }
}

bool _readBool(Object? raw, bool fallback) {
  if (raw is bool) {
    return raw;
  }
  if (raw is num) {
    return raw != 0;
  }
  if (raw is String) {
    return raw.toLowerCase() == 'true' || raw == '1';
  }
  return fallback;
}

String _readString(Object? raw, String fallback) {
  if (raw is String && raw.isNotEmpty) {
    return raw;
  }
  return fallback;
}

int _readInt(Object? raw, int fallback) {
  if (raw is int) {
    return raw;
  }
  if (raw is num) {
    return raw.toInt();
  }
  if (raw is String) {
    return int.tryParse(raw) ?? fallback;
  }
  return fallback;
}

double _readDouble(Object? raw, double fallback) {
  if (raw is double) {
    return raw;
  }
  if (raw is num) {
    return raw.toDouble();
  }
  if (raw is String) {
    return double.tryParse(raw) ?? fallback;
  }
  return fallback;
}
