import 'package:flutter/material.dart';

class AppSurface extends StatelessWidget {
  const AppSurface({
    super.key,
    required this.child,
    this.padding = const EdgeInsets.all(24),
    this.color,
    this.radius = 12,
    this.bordered = true,
  });

  final Widget child;
  final EdgeInsetsGeometry padding;
  final Color? color;
  final double radius;
  final bool bordered;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    return Container(
      decoration: BoxDecoration(
        color: color ?? scheme.surfaceContainerLow,
        borderRadius: BorderRadius.circular(radius),
        border: bordered
            ? Border.all(color: scheme.outlineVariant, width: 1.0)
            : null,
      ),
      child: Padding(
        padding: padding,
        child: child,
      ),
    );
  }
}
