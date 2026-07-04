import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:smart_controller_app/src/features/controller/data/http_esp32_repository.dart';

void main() {
  test('normalizes endpoint when user pastes state.php URL', () async {
    Uri? requestedUri;
    final repository = HttpEsp32Repository(
      baseUrl: 'https://example.org/smart-controller/api/state.php',
      client: MockClient((request) async {
        requestedUri = request.url;
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
    expect(state.availableSpots, 28);
  });
}
