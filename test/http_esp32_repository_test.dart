import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:smart_controller_app/src/features/controller/data/http_esp32_repository.dart';

void main() {
  const apiToken = '0123456789abcdef0123456789abcdef';

  test('normalizes endpoint and authenticates state requests', () async {
    Uri? requestedUri;
    String? authorization;
    final repository = HttpEsp32Repository(
      baseUrl: 'https://example.org/smart-controller/api/state.php',
      apiToken: apiToken,
      client: MockClient((request) async {
        requestedUri = request.url;
        authorization = request.headers['authorization'];
        return http.Response(
          '''
          {
            "ok": true,
            "state": {
              "parkingCapacity": 32,
              "occupiedSpots": 4,
              "lastUpdated": "2026-04-22T10:00:00+00:00"
            }
          }
          ''',
          200,
          headers: const {'content-type': 'application/json'},
        );
      }),
    );

    final state = await repository.fetchState();

    expect(
      requestedUri.toString(),
      'https://example.org/smart-controller/api/state.php',
    );
    expect(authorization, 'Bearer $apiToken');
    expect(state.availableSpots, 28);
  });

  test('rejects cleartext non-loopback endpoints', () async {
    final repository = HttpEsp32Repository(
      baseUrl: 'http://example.org/smart-controller',
      apiToken: apiToken,
      client: MockClient((request) async => http.Response('{}', 200)),
    );

    await expectLater(repository.fetchState(), throwsA(isA<StateError>()));
  });

  test('rejects missing API credentials before network access', () async {
    var called = false;
    final repository = HttpEsp32Repository(
      baseUrl: 'https://example.org/smart-controller',
      apiToken: '',
      client: MockClient((request) async {
        called = true;
        return http.Response('{}', 200);
      }),
    );

    await expectLater(repository.fetchState(), throwsA(isA<StateError>()));
    expect(called, isFalse);
  });
}
