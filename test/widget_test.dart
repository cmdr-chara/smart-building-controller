import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:smart_controller_app/src/app/app.dart';

void main() {
  testWidgets('renders dashboard shell', (WidgetTester tester) async {
    final binding = tester.binding;
    await binding.setSurfaceSize(const Size(390, 844));

    await tester.pumpWidget(const ProviderScope(child: SmartControllerApp()));
    await tester.pump(const Duration(milliseconds: 320));
    await tester.pump(const Duration(milliseconds: 320));

    expect(tester.takeException(), isNull);
    expect(find.byType(MaterialApp), findsOneWidget);
    expect(find.byType(Scaffold), findsOneWidget);

    await binding.setSurfaceSize(null);
  });
}
