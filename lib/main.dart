import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:liquid_glass_widgets/liquid_glass_widgets.dart';

import 'src/app/app.dart';
import 'src/features/controller/application/settings_controller.dart';
import 'src/features/controller/data/settings_storage.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await LiquidGlassWidgets.initialize();

  final storage = SettingsStorage();
  final initialSettings = await storage.load();

  runApp(
    ProviderScope(
      overrides: [
        settingsStorageProvider.overrideWithValue(storage),
        initialSettingsProvider.overrideWithValue(initialSettings),
      ],
      child: const SmartControllerApp(),
    ),
  );
}
