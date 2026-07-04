import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';

import '../../data/esp32_repository.dart';
import '../../domain/smart_home_state.dart';

const _actionCooldown = Duration(seconds: 1);

typedef CommandRunner = Future<void> Function(
  ControllerCommand command, {
  Map<String, dynamic>? params,
});

class ModuleSectionData {
  const ModuleSectionData({
    required this.id,
    required this.indexLabel,
    required this.label,
    required this.title,
    required this.subtitle,
    required this.accent,
    required this.accentStrong,
    required this.icon,
  });

  final String id;
  final String indexLabel;
  final String label;
  final String title;
  final String subtitle;
  final Color accent;
  final Color accentStrong;
  final IconData icon;
}

class ModuleWorkspace extends StatelessWidget {
  const ModuleWorkspace({
    super.key,
    required this.section,
    required this.state,
    required this.onCommand,
  });

  final ModuleSectionData section;
  final SmartHomeState state;
  final CommandRunner onCommand;

  @override
  Widget build(BuildContext context) {
    final metrics = _metricsForSection();
    final actions = _actionsForSection();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _SectionHeader(section: section),
        const SizedBox(height: 16),
        _MetricGrid(metrics: metrics, accent: section.accentStrong),
        const SizedBox(height: 20),
        if (section.id == 'climate') ...[
          _BuzzerControlPanel(
              onCommand: onCommand, accent: section.accentStrong),
          const SizedBox(height: 20),
        ],
        _ActionList(actions: actions, accent: section.accentStrong),
        const SizedBox(height: 20),
        _ActivityLog(events: state.events, accent: section.accentStrong),
      ],
    );
  }

  List<_MetricData> _metricsForSection() {
    switch (section.id) {
      case 'access':
        return [
          _MetricData(
            label: 'Porta Interna',
            value:
                _stateLabel(state.internalDoorUnlocked, 'SBLOCCATA', 'CHIUSA'),
            icon: Icons.door_front_door_outlined,
          ),
          _MetricData(
            label: 'Validazione RFID',
            value: _stateLabel(state.rfidReaderEnabled, 'ATTIVO', 'OFF'),
            icon: Icons.nfc_outlined,
          ),
          _MetricData(
            label: 'Sistema Allarme',
            value: _stateLabel(state.intrusionAlarmArmed, 'ARMATO', 'OFF'),
            icon: Icons.security_rounded,
          ),
        ];
      case 'parking':
        return [
          _MetricData(
            label: 'Posti Liberi',
            value: '${state.availableSpots}',
            icon: Icons.local_parking_rounded,
          ),
          _MetricData(
            label: 'Sbarra Accesso',
            value: _stateLabel(state.parkingBarrierOpen, 'APERTA', 'CHIUSA'),
            icon: Icons.sensor_door_outlined,
          ),
          _MetricData(
            label: 'Rilevamento',
            value: _stateLabel(state.vehicleDetected, 'RILEVATO', 'LIBERO'),
            icon: Icons.sensors_outlined,
          ),
        ];
      case 'exterior':
        return [
          _MetricData(
            label: 'Stato Luci',
            value: _stateLabel(state.exteriorLightsOn, 'ACCESE', 'SPENTE'),
            icon: Icons.lightbulb_rounded,
          ),
          _MetricData(
            label: 'Posizione Tenda',
            value: _stateLabel(state.awningOpen, 'APERTA', 'CHIUSA'),
            icon: Icons.blinds_rounded,
          ),
          _MetricData(
            label: 'Luce Ambiente',
            value: _stateLabel(state.twilightDetected, 'NOTTE', 'GIORNO'),
            icon: Icons.wb_sunny_rounded,
          ),
        ];
      case 'climate':
        return [
          _MetricData(
            label: 'Temperatura',
            value: '${state.temperature.toStringAsFixed(1)}°C',
            icon: Icons.thermostat_rounded,
          ),
          _MetricData(
            label: 'Umidità Rel.',
            value: '${state.humidity.toStringAsFixed(0)}%',
            icon: Icons.water_drop_rounded,
          ),
          _MetricData(
            label: 'Stato Ventola',
            value: _stateLabel(state.fanOn, 'ACCESA', 'SPENTA'),
            icon: Icons.air_outlined,
          ),
        ];
      case 'interior':
        return [
          _MetricData(
            label: 'Sensori Moto',
            value: _stateLabel(state.motionDetected, 'RILEVATO', 'LIBERO'),
            icon: Icons.radar_rounded,
          ),
          _MetricData(
            label: 'Luci Presenza',
            value: _stateLabel(state.indoorLightsOn, 'ACCESE', 'SPENTE'),
            icon: Icons.lightbulb_circle_outlined,
          ),
        ];
      default:
        return [];
    }
  }

  List<_ActionData> _actionsForSection() {
    switch (section.id) {
      case 'access':
        return [
          _ActionData(
            label: 'Sblocco Manuale Porta',
            icon: Icons.lock_open_rounded,
            onTap: () => onCommand(ControllerCommand.openInternalDoor),
          ),
          _ActionData(
            label: state.intrusionAlarmArmed
                ? 'Disattiva Sistema'
                : 'Attiva Sistema',
            icon: Icons.security_rounded,
            onTap: () => onCommand(ControllerCommand.toggleAlarm),
          ),
          _ActionData(
            label: 'Attiva/Disattiva RFID',
            icon: Icons.badge_rounded,
            onTap: () => onCommand(ControllerCommand.toggleRfid),
          ),
        ];
      case 'parking':
        return [
          _ActionData(
            label: 'Apri Sbarra Gate',
            icon: Icons.input_rounded,
            onTap: () => onCommand(ControllerCommand.openParkingBarrier),
          ),
          _ActionData(
            label: 'Registra Entrata',
            icon: Icons.login_rounded,
            onTap: () => onCommand(ControllerCommand.vehicleEntered),
          ),
          _ActionData(
            label: 'Registra Uscita',
            icon: Icons.logout_rounded,
            onTap: () => onCommand(ControllerCommand.vehicleExited),
          ),
        ];
      case 'exterior':
        return [
          _ActionData(
            label: 'Illuminazione Esterna',
            icon: Icons.light_mode_rounded,
            onTap: () => onCommand(ControllerCommand.toggleExteriorLights),
          ),
          _ActionData(
            label: 'Muovi Tenda',
            icon: Icons.curtains_closed_rounded,
            onTap: () => onCommand(ControllerCommand.toggleAwning),
          ),
        ];
      case 'climate':
        return [
          _ActionData(
            label: 'Controllo Ventola',
            icon: Icons.mode_fan_off_rounded,
            onTap: () => onCommand(ControllerCommand.toggleFan),
          ),
          _ActionData(
            label: 'Gestione Finestre',
            icon: Icons.window_rounded,
            onTap: () => onCommand(ControllerCommand.toggleWindows),
          ),
        ];
      case 'interior':
        return [
          _ActionData(
            label: 'Illuminazione Interna',
            icon: Icons.highlight_rounded,
            onTap: () => onCommand(ControllerCommand.toggleIndoorLights),
          ),
        ];
      default:
        return [];
    }
  }

  String _stateLabel(bool active, String activeLabel, String inactiveLabel) {
    if (!state.hasLiveData) return 'N/D';
    return active ? activeLabel : inactiveLabel;
  }
}

class _SectionHeader extends StatelessWidget {
  const _SectionHeader({required this.section});
  final ModuleSectionData section;

  @override
  Widget build(BuildContext context) {
    final text = Theme.of(context).textTheme;
    final scheme = Theme.of(context).colorScheme;
    final accent = section.accentStrong;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // Index + accent pip row
        Row(
          children: [
            Container(
              width: 3,
              height: 20,
              decoration: BoxDecoration(
                color: accent,
                borderRadius: BorderRadius.circular(2),
                boxShadow: [
                  BoxShadow(
                    color: accent.withValues(alpha: 0.4),
                    blurRadius: 8,
                    spreadRadius: 0,
                  ),
                ],
              ),
            ),
            const SizedBox(width: 10),
            Icon(section.icon, size: 14, color: accent),
            const SizedBox(width: 8),
            Text(
              section.title.toUpperCase(),
              style: text.labelLarge?.copyWith(
                color: scheme.onSurface,
                letterSpacing: 1.5,
                fontWeight: FontWeight.w900,
                fontSize: 11,
              ),
            ),
            const Spacer(),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
              decoration: BoxDecoration(
                color: accent.withValues(alpha: 0.1),
                borderRadius: BorderRadius.circular(6),
                border: Border.all(
                  color: accent.withValues(alpha: 0.2),
                  width: 0.5,
                ),
              ),
              child: Text(
                section.indexLabel,
                style: text.labelSmall?.copyWith(
                  color: accent,
                  fontWeight: FontWeight.w900,
                  fontSize: 9,
                  letterSpacing: 1,
                ),
              ),
            ),
          ],
        ),
        const SizedBox(height: 8),
        Padding(
          padding: const EdgeInsets.only(left: 13),
          child: Text(
            section.subtitle,
            style: text.bodyMedium?.copyWith(
              color: scheme.onSurface.withValues(alpha: 0.45),
              fontSize: 13,
            ),
          ),
        ),
      ],
    );
  }
}

class _MetricGrid extends StatelessWidget {
  const _MetricGrid({required this.metrics, required this.accent});
  final List<_MetricData> metrics;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        if (constraints.maxWidth > 600) {
          return GridView.builder(
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 3,
              crossAxisSpacing: 8,
              mainAxisSpacing: 8,
              childAspectRatio: 1.8,
            ),
            itemCount: metrics.length,
            itemBuilder: (context, index) {
              return _MetricTile(
                  metric: metrics[index], index: index, accent: accent);
            },
          );
        }

        // Mobile: use IntrinsicHeight so tiles match the taller sibling
        return Column(
          children: [
            if (metrics.isNotEmpty)
              IntrinsicHeight(
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Expanded(
                      child: _MetricTile(
                          metric: metrics[0], index: 0, accent: accent),
                    ),
                    if (metrics.length > 1) ...[
                      const SizedBox(width: 8),
                      Expanded(
                        child: _MetricTile(
                            metric: metrics[1], index: 1, accent: accent),
                      ),
                    ],
                  ],
                ),
              ),
            if (metrics.length > 2) ...[
              const SizedBox(height: 8),
              _MetricTile(metric: metrics[2], index: 2, accent: accent),
            ],
            for (int i = 3; i < metrics.length; i += 2) ...[
              const SizedBox(height: 8),
              IntrinsicHeight(
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Expanded(
                      child: _MetricTile(
                          metric: metrics[i], index: i, accent: accent),
                    ),
                    if (i + 1 < metrics.length) ...[
                      const SizedBox(width: 8),
                      Expanded(
                        child: _MetricTile(
                            metric: metrics[i + 1],
                            index: i + 1,
                            accent: accent),
                      ),
                    ] else ...[
                      const SizedBox(width: 8),
                      const Spacer(),
                    ],
                  ],
                ),
              ),
            ],
          ],
        );
      },
    );
  }
}

class _MetricTile extends StatelessWidget {
  const _MetricTile({
    required this.metric,
    required this.index,
    required this.accent,
  });
  final _MetricData metric;
  final int index;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    final text = Theme.of(context).textTheme;
    final scheme = Theme.of(context).colorScheme;

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            scheme.surfaceContainerLow,
            Color.lerp(scheme.surfaceContainerLow, accent, 0.06)!,
          ],
        ),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(
          color: accent.withValues(alpha: 0.12),
          width: 0.5,
        ),
        boxShadow: [
          BoxShadow(
            color: accent.withValues(alpha: 0.06),
            blurRadius: 12,
            offset: const Offset(0, 3),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(4),
                decoration: BoxDecoration(
                  color: accent.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(6),
                ),
                child: Icon(metric.icon, size: 11, color: accent),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  metric.label.toUpperCase(),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: text.labelSmall?.copyWith(
                    fontWeight: FontWeight.w800,
                    letterSpacing: 0.8,
                    color: scheme.onSurface.withValues(alpha: 0.45),
                    fontSize: 9,
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 10),
          Text(
            metric.value,
            style: text.headlineSmall?.copyWith(
              fontWeight: FontWeight.w900,
              letterSpacing: -0.5,
              fontSize: 20,
              color: metric.value == 'N/D'
                  ? scheme.onSurface.withValues(alpha: 0.25)
                  : scheme.onSurface,
            ),
          ),
        ],
      ),
    )
        .animate()
        .fadeIn(delay: (index * 60).ms)
        .slideY(begin: 0.08, duration: 350.ms, curve: Curves.easeOutCubic);
  }
}

class _BuzzerMelody {
  const _BuzzerMelody({
    required this.label,
    required this.value,
    required this.icon,
  });

  final String label;
  final String value;
  final IconData icon;
}

const _buzzerMelodies = <_BuzzerMelody>[
  _BuzzerMelody(
    label: 'Doom Riff',
    value: 'doom',
    icon: Icons.whatshot_rounded,
  ),
  _BuzzerMelody(
    label: 'Grandfather Clock',
    value: 'musicBox',
    icon: Icons.graphic_eq_rounded,
  ),
  _BuzzerMelody(
    label: 'Toreador March',
    value: 'toreador',
    icon: Icons.music_note_rounded,
  ),
  _BuzzerMelody(
    label: 'Megalovania breve',
    value: 'mega',
    icon: Icons.bolt_rounded,
  ),
  _BuzzerMelody(
    label: 'Sirena 118',
    value: 'siren118',
    icon: Icons.emergency_rounded,
  ),
];

class _BuzzerControlPanel extends StatefulWidget {
  const _BuzzerControlPanel({required this.onCommand, required this.accent});

  final CommandRunner onCommand;
  final Color accent;

  @override
  State<_BuzzerControlPanel> createState() => _BuzzerControlPanelState();
}

class _BuzzerControlPanelState extends State<_BuzzerControlPanel> {
  Timer? _cooldownTimer;
  String _melody = 'musicBox';
  double _speed = 1.0;
  bool _coolingDown = false;

  @override
  void dispose() {
    _cooldownTimer?.cancel();
    super.dispose();
  }

  void _play() {
    if (_coolingDown) return;

    widget.onCommand(
      ControllerCommand.playSelectedBuzzer,
      params: <String, dynamic>{
        'melody': _melody,
        'speed': (_speed * 100).round(),
      },
    );

    setState(() => _coolingDown = true);
    _cooldownTimer?.cancel();
    _cooldownTimer = Timer(_actionCooldown, () {
      if (!mounted) return;
      setState(() => _coolingDown = false);
    });
  }

  void _stop() {
    if (_coolingDown) return;

    widget.onCommand(ControllerCommand.stopBuzzer);

    setState(() => _coolingDown = true);
    _cooldownTimer?.cancel();
    _cooldownTimer = Timer(_actionCooldown, () {
      if (!mounted) return;
      setState(() => _coolingDown = false);
    });
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final selected = _buzzerMelodies.firstWhere(
      (melody) => melody.value == _melody,
      orElse: () => _buzzerMelodies.first,
    );
    final speedLabel = '${(_speed * 100).round()}%';

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Container(
              width: 3,
              height: 3,
              decoration: BoxDecoration(
                color: widget.accent.withValues(alpha: 0.5),
                shape: BoxShape.circle,
              ),
            ),
            const SizedBox(width: 8),
            Text(
              'BUZZER',
              style: Theme.of(context).textTheme.labelMedium?.copyWith(
                    fontWeight: FontWeight.w900,
                    letterSpacing: 1.2,
                    color: scheme.onSurface.withValues(alpha: 0.3),
                    fontSize: 9,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 8),
        Container(
          padding: const EdgeInsets.all(12),
          decoration: BoxDecoration(
            color: scheme.surfaceContainerLow,
            borderRadius: BorderRadius.circular(14),
            border: Border.all(
              color: scheme.outlineVariant.withValues(alpha: 0.3),
              width: 0.5,
            ),
          ),
          child: Column(
            children: [
              DropdownButtonFormField<String>(
                initialValue: _melody,
                decoration: InputDecoration(
                  labelText: 'Melodia',
                  prefixIcon: Icon(selected.icon, color: widget.accent),
                  isDense: true,
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(10),
                  ),
                ),
                items: [
                  for (final melody in _buzzerMelodies)
                    DropdownMenuItem<String>(
                      value: melody.value,
                      child: Text(melody.label),
                    ),
                ],
                onChanged: (value) {
                  if (value == null) return;
                  setState(() => _melody = value);
                },
              ),
              const SizedBox(height: 12),
              Row(
                children: [
                  Text(
                    'Velocita',
                    style: Theme.of(context).textTheme.labelMedium?.copyWith(
                          fontWeight: FontWeight.w700,
                        ),
                  ),
                  const Spacer(),
                  Text(
                    speedLabel,
                    style: Theme.of(context).textTheme.labelMedium?.copyWith(
                          fontWeight: FontWeight.w800,
                          color: widget.accent,
                        ),
                  ),
                ],
              ),
              Slider(
                value: _speed,
                min: 0.5,
                max: 2.0,
                divisions: 6,
                activeColor: widget.accent,
                label: speedLabel,
                onChanged: (value) => setState(() => _speed = value),
              ),
              const SizedBox(height: 4),
              Row(
                children: [
                  Expanded(
                    child: FilledButton.icon(
                      onPressed: _coolingDown ? null : _play,
                      icon: Icon(
                        _coolingDown
                            ? Icons.hourglass_bottom_rounded
                            : Icons.play_arrow_rounded,
                      ),
                      label: Text(_coolingDown ? 'Attendi' : 'Suona'),
                      style: FilledButton.styleFrom(
                        backgroundColor: widget.accent,
                        foregroundColor: scheme.onPrimary,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(10),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  IconButton.filledTonal(
                    onPressed: _coolingDown ? null : _stop,
                    icon: const Icon(Icons.stop_rounded),
                    tooltip: 'Ferma buzzer',
                    style: IconButton.styleFrom(
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(10),
                      ),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class _ActionList extends StatelessWidget {
  const _ActionList({required this.actions, required this.accent});
  final List<_ActionData> actions;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    if (actions.isEmpty) return const SizedBox.shrink();
    final scheme = Theme.of(context).colorScheme;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Container(
              width: 3,
              height: 3,
              decoration: BoxDecoration(
                color: accent.withValues(alpha: 0.5),
                shape: BoxShape.circle,
              ),
            ),
            const SizedBox(width: 8),
            Text(
              'CONTROLLI MODULO',
              style: Theme.of(context).textTheme.labelMedium?.copyWith(
                    fontWeight: FontWeight.w900,
                    letterSpacing: 1.2,
                    color: scheme.onSurface.withValues(alpha: 0.3),
                    fontSize: 9,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 8),
        Container(
          decoration: BoxDecoration(
            color: scheme.surfaceContainerLow,
            borderRadius: BorderRadius.circular(14),
            border: Border.all(
              color: scheme.outlineVariant.withValues(alpha: 0.3),
              width: 0.5,
            ),
          ),
          child: Column(
            children: [
              for (int i = 0; i < actions.length; i++) ...[
                _ActionRow(action: actions[i], accent: accent),
                if (i < actions.length - 1)
                  Divider(
                    indent: 48,
                    height: 1,
                    color: scheme.outlineVariant.withValues(alpha: 0.2),
                  ),
              ],
            ],
          ),
        ),
      ],
    );
  }
}

class _ActionRow extends StatefulWidget {
  const _ActionRow({required this.action, required this.accent});
  final _ActionData action;
  final Color accent;

  @override
  State<_ActionRow> createState() => _ActionRowState();
}

class _ActionRowState extends State<_ActionRow> {
  Timer? _cooldownTimer;
  bool _coolingDown = false;

  @override
  void dispose() {
    _cooldownTimer?.cancel();
    super.dispose();
  }

  void _handleTap() {
    if (_coolingDown) return;

    widget.action.onTap();

    setState(() => _coolingDown = true);
    _cooldownTimer?.cancel();
    _cooldownTimer = Timer(_actionCooldown, () {
      if (!mounted) return;
      setState(() => _coolingDown = false);
    });
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final action = widget.action;
    final accent = widget.accent;

    return Material(
      color: Colors.transparent,
      child: InkWell(
        onTap: _coolingDown ? null : _handleTap,
        borderRadius: BorderRadius.circular(16),
        splashColor: accent.withValues(alpha: 0.08),
        highlightColor: accent.withValues(alpha: 0.04),
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
          child: Row(
            children: [
              Container(
                padding: const EdgeInsets.all(7),
                decoration: BoxDecoration(
                  color: accent.withValues(alpha: 0.08),
                  borderRadius: BorderRadius.circular(9),
                  border: Border.all(
                    color: accent.withValues(alpha: 0.12),
                    width: 0.5,
                  ),
                ),
                child: Icon(action.icon, size: 15, color: accent),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Text(
                  action.label,
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.w600,
                        fontSize: 13,
                      ),
                ),
              ),
              Icon(
                _coolingDown
                    ? Icons.hourglass_bottom_rounded
                    : Icons.chevron_right_rounded,
                size: 16,
                color: scheme.onSurface.withValues(
                  alpha: _coolingDown ? 0.35 : 0.15,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _ActivityLog extends StatelessWidget {
  const _ActivityLog({required this.events, required this.accent});
  final List<SystemEvent> events;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    if (events.isEmpty) return const SizedBox.shrink();
    final scheme = Theme.of(context).colorScheme;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Container(
              width: 3,
              height: 3,
              decoration: BoxDecoration(
                color: accent.withValues(alpha: 0.5),
                shape: BoxShape.circle,
              ),
            ),
            const SizedBox(width: 8),
            Text(
              'LOG EVENTI',
              style: Theme.of(context).textTheme.labelMedium?.copyWith(
                    fontWeight: FontWeight.w900,
                    letterSpacing: 1.2,
                    color: scheme.onSurface.withValues(alpha: 0.3),
                    fontSize: 9,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 8),
        Container(
          decoration: BoxDecoration(
            color: scheme.surfaceContainerLow,
            borderRadius: BorderRadius.circular(14),
            border: Border.all(
              color: scheme.outlineVariant.withValues(alpha: 0.3),
              width: 0.5,
            ),
          ),
          child: ListView.separated(
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            padding: const EdgeInsets.symmetric(vertical: 10),
            itemCount: events.length > 5 ? 5 : events.length,
            separatorBuilder: (context, index) => Divider(
              indent: 32,
              height: 16,
              color: scheme.outlineVariant.withValues(alpha: 0.2),
            ),
            itemBuilder: (context, i) =>
                _EventRow(event: events[i], index: i, accent: accent),
          ),
        ),
      ],
    );
  }
}

class _EventRow extends StatelessWidget {
  const _EventRow({
    required this.event,
    required this.index,
    required this.accent,
  });
  final SystemEvent event;
  final int index;
  final Color accent;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final text = Theme.of(context).textTheme;
    final isLatest = index == 0;

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 14),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Padding(
            padding: const EdgeInsets.only(top: 5),
            child: Container(
              width: 7,
              height: 7,
              decoration: BoxDecoration(
                color: isLatest
                    ? accent
                    : scheme.onSurface.withValues(alpha: 0.15),
                shape: BoxShape.circle,
                boxShadow: isLatest
                    ? [
                        BoxShadow(
                          color: accent.withValues(alpha: 0.4),
                          blurRadius: 6,
                          spreadRadius: 0,
                        ),
                      ]
                    : null,
              ),
            ),
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  event.title,
                  style: text.bodyMedium?.copyWith(
                    fontWeight: isLatest ? FontWeight.w700 : FontWeight.w500,
                    color: isLatest
                        ? scheme.onSurface
                        : scheme.onSurface.withValues(alpha: 0.55),
                    fontSize: 13,
                  ),
                ),
                const SizedBox(height: 3),
                Text(
                  _formatTime(event.createdAt),
                  style: text.labelSmall?.copyWith(
                    color: scheme.onSurface.withValues(alpha: 0.3),
                    fontSize: 10,
                    letterSpacing: 0.3,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    ).animate().fadeIn(delay: (index * 50).ms).slideX(begin: 0.05);
  }
}

class _MetricData {
  const _MetricData({
    required this.label,
    required this.value,
    required this.icon,
  });
  final String label;
  final String value;
  final IconData icon;
}

class _ActionData {
  const _ActionData({
    required this.label,
    required this.icon,
    required this.onTap,
  });
  final String label;
  final IconData icon;
  final VoidCallback onTap;
}

String _formatTime(DateTime value) {
  final hour = value.hour.toString().padLeft(2, '0');
  final minute = value.minute.toString().padLeft(2, '0');
  final second = value.second.toString().padLeft(2, '0');
  return '$hour:$minute:$second';
}
