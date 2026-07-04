import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:liquid_glass_widgets/liquid_glass_widgets.dart';

import '../application/controller_notifier.dart';
import '../application/settings_controller.dart';
import '../domain/esp32_settings.dart';
import '../domain/smart_home_state.dart';
import 'widgets/module_sections.dart';

class ControllerHomePage extends ConsumerStatefulWidget {
  const ControllerHomePage({super.key});

  @override
  ConsumerState<ControllerHomePage> createState() => _ControllerHomePageState();
}

class _ControllerHomePageState extends ConsumerState<ControllerHomePage> {
  int _selectedIndex = 0;

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(controllerProvider);
    final settings = ref.watch(settingsControllerProvider);
    final size = MediaQuery.sizeOf(context);
    final wide = size.width >= 1000;

    if (wide) {
      return Scaffold(
        backgroundColor: Theme.of(context).colorScheme.surface,
        body: Row(
          children: [
            _DesktopNav(
              selectedIndex: _selectedIndex,
              onSelected: (index) => setState(() => _selectedIndex = index),
            ),
            VerticalDivider(
                width: 1,
                color: Theme.of(context).colorScheme.outlineVariant),
            Expanded(
              child: _MainWorkspace(
                state: state,
                settings: settings,
                selectedIndex: _selectedIndex,
                onSettings: () => _showSettings(context, settings),
              ),
            ),
          ],
        ),
      );
    }

    return Scaffold(
      backgroundColor: Theme.of(context).colorScheme.surface,
      extendBody: true,
      body: _MainWorkspace(
        state: state,
        settings: settings,
        selectedIndex: _selectedIndex,
        onSettings: () => _showSettings(context, settings),
        hasFloatingNavigation: true,
      ),
      bottomNavigationBar: Padding(
        padding: EdgeInsets.fromLTRB(
          16,
          0,
          16,
          MediaQuery.of(context).viewPadding.bottom + 4,
        ),
        child: _MobileNav(
          selectedIndex: _selectedIndex,
          onSelected: (index) => setState(() => _selectedIndex = index),
        ),
      ),
    );
  }

  Future<void> _showSettings(
      BuildContext context, Esp32Settings settings) async {
    final controller = TextEditingController(text: settings.baseUrl);
    final scheme = Theme.of(context).colorScheme;
    final text = Theme.of(context).textTheme;

    await showModalBottomSheet<void>(
      context: context,
      isScrollControlled: true,
      backgroundColor: Colors.transparent,
      builder: (context) {
        return Padding(
          padding: EdgeInsets.only(
            bottom: MediaQuery.of(context).viewInsets.bottom,
          ),
          child: Container(
            margin: const EdgeInsets.fromLTRB(12, 0, 12, 12),
            padding: const EdgeInsets.fromLTRB(20, 16, 20, 16),
            decoration: BoxDecoration(
              color: scheme.surfaceContainerLow,
              borderRadius: BorderRadius.circular(20),
              border: Border.all(
                color: scheme.outlineVariant.withValues(alpha: 0.4),
                width: 0.5,
              ),
              boxShadow: [
                BoxShadow(
                  color: Colors.black.withValues(alpha: 0.3),
                  blurRadius: 24,
                  offset: const Offset(0, -4),
                ),
              ],
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Grab handle
                Center(
                  child: Container(
                    width: 32,
                    height: 3,
                    decoration: BoxDecoration(
                      color: scheme.onSurface.withValues(alpha: 0.12),
                      borderRadius: BorderRadius.circular(2),
                    ),
                  ),
                ),
                const SizedBox(height: 14),
                // Title row
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.all(6),
                      decoration: BoxDecoration(
                        color: scheme.secondary.withValues(alpha: 0.1),
                        borderRadius: BorderRadius.circular(8),
                      ),
                      child: Icon(
                        Icons.settings_outlined,
                        size: 14,
                        color: scheme.secondary,
                      ),
                    ),
                    const SizedBox(width: 10),
                    Text(
                      'Impostazioni',
                      style: text.titleMedium?.copyWith(
                        fontWeight: FontWeight.w800,
                        fontSize: 14,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                // Section label
                Text(
                  'CONFIGURAZIONE BRIDGE REMOTO',
                  style: text.labelSmall?.copyWith(
                    fontWeight: FontWeight.w900,
                    letterSpacing: 1.2,
                    color: scheme.onSurface.withValues(alpha: 0.35),
                    fontSize: 9,
                  ),
                ),
                const SizedBox(height: 10),
                // Input field
                TextField(
                  controller: controller,
                  autofocus: true,
                  style: text.bodyMedium?.copyWith(
                    fontSize: 13,
                    color: scheme.onSurface,
                  ),
                  decoration: InputDecoration(
                    labelText: 'URL Base del Server',
                    labelStyle: text.labelMedium?.copyWith(
                      color: scheme.onSurface.withValues(alpha: 0.5),
                      fontSize: 11,
                    ),
                    hintText: 'https://esempio.it/api',
                    hintStyle: text.bodySmall?.copyWith(
                      color: scheme.onSurface.withValues(alpha: 0.25),
                    ),
                    filled: true,
                    fillColor: scheme.surface,
                    contentPadding: const EdgeInsets.symmetric(
                      horizontal: 14,
                      vertical: 12,
                    ),
                    border: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: BorderSide(
                        color: scheme.outlineVariant.withValues(alpha: 0.3),
                      ),
                    ),
                    enabledBorder: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: BorderSide(
                        color: scheme.outlineVariant.withValues(alpha: 0.3),
                      ),
                    ),
                    focusedBorder: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: BorderSide(
                        color: scheme.secondary.withValues(alpha: 0.6),
                        width: 1.5,
                      ),
                    ),
                  ),
                ),
                const SizedBox(height: 8),
                Text(
                  'Inserisci l\'endpoint del tuo bridge PHP remoto.',
                  style: text.labelSmall?.copyWith(
                    color: scheme.onSurface.withValues(alpha: 0.3),
                    fontSize: 10,
                  ),
                ),
                const SizedBox(height: 16),
                Divider(
                  height: 1,
                  color: scheme.outlineVariant.withValues(alpha: 0.2),
                ),
                const SizedBox(height: 14),
                // Actions
                Row(
                  children: [
                    Expanded(
                      child: OutlinedButton(
                        onPressed: () => Navigator.of(context).pop(),
                        style: OutlinedButton.styleFrom(
                          minimumSize: const Size.fromHeight(40),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(10),
                          ),
                          side: BorderSide(
                            color: scheme.outlineVariant.withValues(alpha: 0.4),
                          ),
                        ),
                        child: Text(
                          'Annulla',
                          style: text.labelMedium?.copyWith(
                            fontWeight: FontWeight.w700,
                            fontSize: 12,
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: FilledButton(
                        onPressed: () async {
                          await ref
                              .read(settingsControllerProvider.notifier)
                              .updateConnection(baseUrl: controller.text);
                          if (context.mounted) {
                            Navigator.of(context).pop();
                          }
                          await ref
                              .read(controllerProvider.notifier)
                              .refresh();
                        },
                        style: FilledButton.styleFrom(
                          minimumSize: const Size.fromHeight(40),
                          backgroundColor: scheme.primary,
                          foregroundColor: scheme.surface,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(10),
                          ),
                        ),
                        child: Text(
                          'Salva',
                          style: text.labelMedium?.copyWith(
                            fontWeight: FontWeight.w700,
                            fontSize: 12,
                            color: scheme.surface,
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}

class _MainWorkspace extends ConsumerWidget {
  const _MainWorkspace({
    required this.state,
    required this.settings,
    required this.selectedIndex,
    required this.onSettings,
    this.hasFloatingNavigation = false,
  });

  final SmartHomeState state;
  final Esp32Settings settings;
  final int selectedIndex;
  final VoidCallback onSettings;
  final bool hasFloatingNavigation;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return SafeArea(
      bottom: false,
      child: Column(
        children: [
          _Header(
            state: state,
            onSettings: onSettings,
            onRefresh: () => ref.read(controllerProvider.notifier).refresh(),
          ),
          _GlobalStatus(state: state, settings: settings),
          Expanded(
            child: SingleChildScrollView(
              padding: EdgeInsets.fromLTRB(
                16,
                16,
                16,
                hasFloatingNavigation ? 100 : 56,
              ),
              physics: const ClampingScrollPhysics(),
              child: AnimatedSwitcher(
                duration: const Duration(milliseconds: 180),
                layoutBuilder: (currentChild, previousChildren) {
                  return currentChild ?? const SizedBox.shrink();
                },
                transitionBuilder: (child, animation) {
                  return FadeTransition(opacity: animation, child: child);
                },
                child: ModuleWorkspace(
                  key: ValueKey<int>(selectedIndex),
                  section: _sectionData[selectedIndex],
                  state: state,
                  onCommand: ref.read(controllerProvider.notifier).runCommand,
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _Header extends StatelessWidget {
  const _Header({
    required this.state,
    required this.onSettings,
    required this.onRefresh,
  });

  final SmartHomeState state;
  final VoidCallback onSettings;
  final VoidCallback onRefresh;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 8, 10, 6),
      child: Row(
        children: [
          Container(
            width: 6,
            height: 6,
            decoration: BoxDecoration(
              color: const Color(0xFF0E6F69),
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(
                  color: const Color(0xFF0E6F69).withValues(alpha: 0.5),
                  blurRadius: 8,
                ),
              ],
            ),
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              'SISTEMA OPERATIVO ESP32',
              style: Theme.of(context).textTheme.labelLarge?.copyWith(
                    letterSpacing: 1.5,
                    fontWeight: FontWeight.w900,
                    fontSize: 11,
                  ),
            ),
          ),
          if (state.isLoading)
            const Padding(
              padding: EdgeInsets.symmetric(horizontal: 12),
              child: SizedBox(
                width: 16,
                height: 16,
                child: CircularProgressIndicator(strokeWidth: 2),
              ),
            )
          else
            IconButton(
              onPressed: onRefresh,
              padding: EdgeInsets.zero,
              constraints: const BoxConstraints(minWidth: 36, minHeight: 36),
              style: IconButton.styleFrom(
                backgroundColor: scheme.surfaceContainerLow,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(10),
                ),
              ),
              icon: const Icon(Icons.refresh_rounded, size: 16),
              tooltip: 'Aggiorna stato',
            ),
          const SizedBox(width: 6),
          IconButton(
            onPressed: onSettings,
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 36, minHeight: 36),
            style: IconButton.styleFrom(
              backgroundColor: scheme.surfaceContainerLow,
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(10),
              ),
            ),
            icon: const Icon(Icons.settings_outlined, size: 16),
            tooltip: 'Impostazioni',
          ),
        ],
      ),
    );
  }
}

class _GlobalStatus extends StatelessWidget {
  const _GlobalStatus({required this.state, required this.settings});

  final SmartHomeState state;
  final Esp32Settings settings;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final text = Theme.of(context).textTheme;

    return Container(
      padding: const EdgeInsets.symmetric(vertical: 6, horizontal: 12),
      decoration: BoxDecoration(
        border: Border(bottom: BorderSide(color: scheme.outlineVariant)),
      ),
      child: Wrap(
        alignment: WrapAlignment.center,
        crossAxisAlignment: WrapCrossAlignment.center,
        spacing: 16,
        runSpacing: 8,
        children: [
          _StatusIndicator(
            active: settings.isConfigured,
            label: settings.isConfigured ? 'BRIDGE OK' : 'OFFLINE',
            icon: Icons.link_rounded,
          ),
          _StatusIndicator(
            active: state.hasLiveData,
            label: state.hasLiveData ? 'CONNESSO' : 'IN ATTESA',
            icon: Icons.sensors_rounded,
          ),
          if (state.hasLiveData)
            Text(
              'SYNC ${_formatTime(state.lastUpdated)}',
              style: text.labelSmall?.copyWith(
                color: scheme.onSurface.withValues(alpha: 0.4),
                letterSpacing: 0.5,
                fontWeight: FontWeight.w800,
                fontSize: 9,
              ),
            ),
        ],
      ),
    );
  }
}

class _StatusIndicator extends StatelessWidget {
  const _StatusIndicator({
    required this.active,
    required this.label,
    required this.icon,
  });

  final bool active;
  final String label;
  final IconData icon;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: 5,
          height: 5,
          decoration: BoxDecoration(
            color: active ? const Color(0xFF10B981) : scheme.error,
            shape: BoxShape.circle,
          ),
        )
            .animate(onPlay: (c) => c.repeat(reverse: true))
            .fade(duration: 800.ms, begin: 0.3),
        const SizedBox(width: 6),
        Icon(icon, size: 10, color: scheme.onSurface.withValues(alpha: 0.4)),
        const SizedBox(width: 4),
        Text(
          label,
          style: Theme.of(context).textTheme.labelMedium?.copyWith(
                fontWeight: FontWeight.w900,
                color: scheme.onSurface.withValues(alpha: 0.7),
                fontSize: 9,
                letterSpacing: 0.5,
              ),
        ),
      ],
    );
  }
}

class _DesktopNav extends StatelessWidget {
  const _DesktopNav({required this.selectedIndex, required this.onSelected});

  final int selectedIndex;
  final ValueChanged<int> onSelected;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return NavigationRail(
      selectedIndex: selectedIndex,
      onDestinationSelected: onSelected,
      labelType: NavigationRailLabelType.none,
      backgroundColor: scheme.surface,
      minWidth: 64,
      leading: const Padding(
        padding: EdgeInsets.symmetric(vertical: 24),
        child: Icon(Icons.terminal_rounded, size: 24),
      ),
      destinations: [
        for (final item in _sectionData)
          NavigationRailDestination(
            icon: Icon(item.icon, size: 20),
            selectedIcon: Icon(item.icon, size: 20, color: scheme.primary),
            label: Text(item.label),
          ),
      ],
    );
  }
}

class _MobileNav extends StatelessWidget {
  const _MobileNav({required this.selectedIndex, required this.onSelected});

  final int selectedIndex;
  final ValueChanged<int> onSelected;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    return ClipRRect(
      borderRadius: BorderRadius.circular(28),
      clipBehavior: Clip.antiAlias,
      child: GlassBottomBar(
      selectedIndex: selectedIndex,
      onTabSelected: onSelected,
      barHeight: 64,
      horizontalPadding: 8,
      verticalPadding: 10,
      blendAmount: 22,
      iconSize: 22,
      selectedIconColor: scheme.onSurface,
      unselectedIconColor: scheme.onSurface,
      glowBlurRadius: 14,
      glowSpreadRadius: 0,
      glowOpacity: 0.42,
      maskingQuality: MaskingQuality.high,
      magnification: 1.08,
      quality: GlassQuality.premium,
      // Bar background glass — visible refraction + blur
      glassSettings: LiquidGlassSettings(
        glassColor: scheme.surfaceContainerHighest.withValues(alpha: 0.28),
        thickness: 30,
        blur: 3,
        chromaticAberration: 0.3,
        refractiveIndex: 1.59,
        lightIntensity: 0.6,
        ambientStrength: 1,
        saturation: 0.7,
      ),
      // Indicator glass — stronger refraction for "water droplet" lens
      indicatorSettings: const LiquidGlassSettings(
        glassColor: Color.fromRGBO(255, 255, 255, 0.12),
        thickness: 22,
        blur: 0,
        chromaticAberration: 0.5,
        refractiveIndex: 1.2,
        lightIntensity: 1.6,
        saturation: 1.2,
      ),
        tabs: [
          for (final section in _sectionData)
            GlassBottomBarTab(
              icon: Icon(section.icon),
              glowColor: section.accentStrong,
            ),
        ],
      ),
    );
  }
}

const _sectionData = <ModuleSectionData>[
  ModuleSectionData(
    id: 'access',
    indexLabel: '01',
    label: 'Accessi',
    title: 'Protocollo Accessi',
    subtitle: 'Autenticazione e sicurezza perimetrale.',
    accent: Color(0xFF0E6F69),
    accentStrong: Color(0xFF0E6F69),
    icon: Icons.security_rounded,
  ),
  ModuleSectionData(
    id: 'parking',
    indexLabel: '02',
    label: 'Parcheggio',
    title: 'Flusso Logistico',
    subtitle: 'Tracciamento veicoli e posti.',
    accent: Color(0xFFB86A2F),
    accentStrong: Color(0xFFB86A2F),
    icon: Icons.local_parking_rounded,
  ),
  ModuleSectionData(
    id: 'exterior',
    indexLabel: '03',
    label: 'Esterne',
    title: 'Sistemi Atmosferici',
    subtitle: 'Illuminazione e protezione.',
    accent: Color(0xFF4D7A5A),
    accentStrong: Color(0xFF4D7A5A),
    icon: Icons.wb_sunny_rounded,
  ),
  ModuleSectionData(
    id: 'climate',
    indexLabel: '04',
    label: 'Clima',
    title: 'Controllo Ambientale',
    subtitle: 'Termodinamica e flussi d\'aria.',
    accent: Color(0xFF4A6C8A),
    accentStrong: Color(0xFF4A6C8A),
    icon: Icons.thermostat_rounded,
  ),
  ModuleSectionData(
    id: 'interior',
    indexLabel: '05',
    label: 'Interni',
    title: 'Logica Abitativa',
    subtitle: 'Presenza e luci automatiche.',
    accent: Color(0xFF745091),
    accentStrong: Color(0xFF745091),
    icon: Icons.home_max_rounded,
  ),
];

String _formatTime(DateTime value) {
  final hour = value.hour.toString().padLeft(2, '0');
  final minute = value.minute.toString().padLeft(2, '0');
  final second = value.second.toString().padLeft(2, '0');
  return '$hour:$minute:$second';
}
