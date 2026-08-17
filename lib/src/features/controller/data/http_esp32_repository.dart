import 'dart:convert';

import 'package:http/http.dart' as http;

import '../domain/smart_home_state.dart';
import 'esp32_repository.dart';

class HttpEsp32Repository implements Esp32Repository {
  HttpEsp32Repository({
    required this.baseUrl,
    required this.client,
    this.apiToken = const String.fromEnvironment('SMART_CONTROLLER_API_TOKEN'),
  });

  final String baseUrl;
  final http.Client client;
  final String apiToken;

  Uri _uri(String fileName) {
    final normalized = _normalizedBaseUrl();
    final uri = Uri.parse('$normalized/api/$fileName');
    _validateEndpoint(uri);
    return uri;
  }

  String _normalizedBaseUrl() {
    var value = baseUrl.trim();

    while (value.endsWith('/')) {
      value = value.substring(0, value.length - 1);
    }

    const endpointSuffixes = <String>[
      '/api/state.php',
      '/api/command.php',
      '/api/device_state.php',
      '/api',
    ];

    for (final suffix in endpointSuffixes) {
      if (value.toLowerCase().endsWith(suffix)) {
        value = value.substring(0, value.length - suffix.length);
        break;
      }
    }

    return value;
  }

  void _validateEndpoint(Uri uri) {
    final loopback = uri.host == 'localhost' ||
        uri.host == '127.0.0.1' ||
        uri.host == '::1';
    if (uri.scheme == 'https' || (uri.scheme == 'http' && loopback)) {
      return;
    }
    throw StateError(
      'Gli endpoint remoti devono usare HTTPS; HTTP e consentito solo su loopback.',
    );
  }

  Map<String, String> _headers({bool json = false}) {
    final token = apiToken.trim();
    if (token.length < 32) {
      throw StateError(
        'SMART_CONTROLLER_API_TOKEN non configurato o troppo corto.',
      );
    }

    return <String, String>{
      'Accept': 'application/json',
      'Authorization': 'Bearer $token',
      if (json) 'Content-Type': 'application/json',
    };
  }

  @override
  Future<SmartHomeState> fetchState({SmartHomeState? fallback}) async {
    final uri = _uri('state.php');
    final response = await client.get(
      uri,
      headers: _headers(),
    );

    if (response.statusCode != 200) {
      throw Exception(
        'GET state.php fallita: ${response.statusCode} ${_bodyPreview(response.body)}',
      );
    }

    final decoded = _decodeMap(response.body, uri);
    final payload = decoded['state'] is Map<String, dynamic>
        ? decoded['state'] as Map<String, dynamic>
        : decoded;

    return SmartHomeState.fromJson(
      payload,
      fallback: fallback,
    );
  }

  @override
  Future<SmartHomeState> sendCommand(
    ControllerCommand command,
    SmartHomeState current, {
    Map<String, dynamic>? params,
  }) async {
    final uri = _uri('command.php');
    final body = <String, dynamic>{
      'action': command.apiValue,
      'state': current.toJson(),
    };
    if (params != null && params.isNotEmpty) {
      body['params'] = params;
    }

    final response = await client.post(
      uri,
      headers: _headers(json: true),
      body: jsonEncode(body),
    );

    if (response.statusCode != 200) {
      throw Exception(
        'POST command.php fallita: ${response.statusCode} ${_bodyPreview(response.body)}',
      );
    }

    final decoded = _decodeMap(response.body, uri);
    final payload = decoded['state'] is Map<String, dynamic>
        ? decoded['state'] as Map<String, dynamic>
        : decoded;

    return SmartHomeState.fromJson(
      payload,
      fallback: current,
    ).withEvent('Comando remoto inviato: ${command.apiValue}');
  }

  Map<String, dynamic> _decodeMap(String body, Uri uri) {
    try {
      final decoded = jsonDecode(body);
      if (decoded is Map<String, dynamic>) {
        return decoded;
      }
    } on FormatException catch (error) {
      throw FormatException(
        'Risposta non JSON da $uri: ${_bodyPreview(body)}',
        error.source,
        error.offset,
      );
    }
    throw FormatException('Risposta JSON non valida da $uri');
  }

  String _bodyPreview(String body) {
    final compact = body.replaceAll(RegExp(r'\s+'), ' ').trim();
    if (compact.isEmpty) {
      return '(risposta vuota)';
    }
    return compact.length <= 140 ? compact : '${compact.substring(0, 140)}...';
  }
}
