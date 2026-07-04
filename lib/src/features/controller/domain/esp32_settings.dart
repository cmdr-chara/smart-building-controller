class Esp32Settings {
  const Esp32Settings({this.baseUrl = ''});

  final String baseUrl;

  bool get isConfigured => baseUrl.isNotEmpty;

  String get connectionLabel {
    if (isConfigured) {
      return baseUrl;
    }
    return 'Server non configurato';
  }

  Esp32Settings copyWith({String? baseUrl}) {
    return Esp32Settings(baseUrl: baseUrl ?? this.baseUrl);
  }
}
