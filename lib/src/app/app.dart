import 'package:flutter/material.dart';
import 'package:liquid_glass_widgets/liquid_glass_widgets.dart';

import '../features/controller/presentation/controller_home_page.dart';
import 'theme/app_theme.dart';

class SmartControllerApp extends StatelessWidget {
  const SmartControllerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return LiquidGlassWidgets.wrap(MaterialApp(
      title: 'ESP32 Smart Controller',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.light(),
      darkTheme: AppTheme.dark(),
      themeMode: ThemeMode.system,
      home: const ControllerHomePage(),
    ));
  }
}
