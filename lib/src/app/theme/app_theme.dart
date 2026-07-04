import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class AppTheme {
  static ThemeData light() {
    final scheme = ColorScheme.fromSeed(
      seedColor: const Color(0xFF1A1F1E),
      primary: const Color(0xFF1A1F1E),
      secondary: const Color(0xFF0E6F69),
      surface: const Color(0xFFFFFFFF),
      onSurface: const Color(0xFF1A1F1E),
      surfaceContainerLow: const Color(0xFFF7F8F8),
      outlineVariant: const Color(0xFFE5E7EB),
    );
    return _buildTheme(scheme, Brightness.light);
  }

  static ThemeData dark() {
    final scheme = ColorScheme.fromSeed(
      seedColor: const Color(0xFFFFFFFF),
      primary: const Color(0xFFFFFFFF),
      secondary: const Color(0xFF0E6F69),
      surface: const Color(0xFF111312),
      onSurface: const Color(0xFFF9FAFB),
      surfaceContainerLow: const Color(0xFF1A1D1C),
      outlineVariant: const Color(0xFF2D3331),
      brightness: Brightness.dark,
    );
    return _buildTheme(scheme, Brightness.dark);
  }

  static ThemeData _buildTheme(ColorScheme scheme, Brightness brightness) {
    final bodyText = GoogleFonts.plusJakartaSansTextTheme(
      ThemeData(brightness: brightness).textTheme,
    );
    final displayText = GoogleFonts.soraTextTheme(bodyText);

    return ThemeData(
      useMaterial3: true,
      colorScheme: scheme,
      scaffoldBackgroundColor: scheme.surface,
      textTheme: displayText.copyWith(
        headlineLarge: displayText.headlineLarge?.copyWith(
          fontWeight: FontWeight.w800,
          height: 1.1,
          letterSpacing: -1.5,
          color: scheme.onSurface,
        ),
        headlineMedium: displayText.headlineMedium?.copyWith(
          fontWeight: FontWeight.w800,
          letterSpacing: -1.0,
          color: scheme.onSurface,
        ),
        headlineSmall: displayText.headlineSmall?.copyWith(
          fontWeight: FontWeight.w800,
          letterSpacing: -0.5,
          color: scheme.onSurface,
        ),
        titleLarge: bodyText.titleLarge?.copyWith(
          fontWeight: FontWeight.w700,
          fontSize: 18,
        ),
        titleMedium: bodyText.titleMedium?.copyWith(
          fontWeight: FontWeight.w600,
          fontSize: 15,
        ),
        bodyLarge: bodyText.bodyLarge?.copyWith(
          height: 1.6,
          fontWeight: FontWeight.w400,
          color: scheme.onSurface.withValues(alpha: 0.8),
        ),
        bodyMedium: bodyText.bodyMedium?.copyWith(
          height: 1.6,
          color: scheme.onSurface.withValues(alpha: 0.6),
        ),
        labelLarge: bodyText.labelLarge?.copyWith(
          fontWeight: FontWeight.w700,
          letterSpacing: 0.5,
          fontSize: 12,
        ),
        labelMedium: bodyText.labelMedium?.copyWith(
          fontWeight: FontWeight.w700,
          letterSpacing: 0.5,
          fontSize: 10,
        ),
      ),
      appBarTheme: AppBarTheme(
        backgroundColor: scheme.surface,
        elevation: 0,
        scrolledUnderElevation: 0,
        centerTitle: false,
        titleTextStyle: displayText.titleLarge?.copyWith(
          fontWeight: FontWeight.w800,
          color: scheme.onSurface,
        ),
        iconTheme: IconThemeData(color: scheme.onSurface),
      ),
      navigationBarTheme: NavigationBarThemeData(
        height: 64,
        backgroundColor: scheme.surface,
        indicatorColor: Colors.transparent,
        labelTextStyle: WidgetStateProperty.resolveWith((states) {
          final selected = states.contains(WidgetState.selected);
          return bodyText.labelMedium?.copyWith(
            fontWeight: selected ? FontWeight.w800 : FontWeight.w500,
            color: selected ? scheme.primary : scheme.onSurface.withValues(alpha: 0.5),
          );
        }),
        iconTheme: WidgetStateProperty.resolveWith((states) {
          final selected = states.contains(WidgetState.selected);
          return IconThemeData(
            color: selected ? scheme.primary : scheme.onSurface.withValues(alpha: 0.5),
            size: 24,
          );
        }),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: FilledButton.styleFrom(
          minimumSize: const Size.fromHeight(48),
          backgroundColor: scheme.primary,
          foregroundColor: scheme.surface,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
          textStyle: bodyText.labelLarge?.copyWith(fontWeight: FontWeight.w700),
        ),
      ),
      outlinedButtonTheme: OutlinedButtonThemeData(
        style: OutlinedButton.styleFrom(
          minimumSize: const Size.fromHeight(48),
          side: BorderSide(color: scheme.outlineVariant, width: 1.0),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
          textStyle: bodyText.labelLarge?.copyWith(
            fontWeight: FontWeight.w700,
            color: scheme.onSurface,
          ),
        ),
      ),
      dividerTheme: DividerThemeData(
        color: scheme.outlineVariant,
        thickness: 1,
        space: 1,
      ),
      dialogTheme: DialogThemeData(
        backgroundColor: scheme.surface,
        surfaceTintColor: Colors.transparent,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      ),
    );
  }
}

