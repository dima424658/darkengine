# Retired Vulkan shadow-map renderer

The shadow-map implementation has been removed: PCSS/PCF filters, point-light
cube maps, sun cascades, camera-selected light budgets, distant pools and stable
depth caching are no longer part of the modern renderer. Their implementation
and historical development notes remain available in Git history.

The replacement uses actual RTXDI ReSTIR DI, RTXGI SHaRC and NRD RELAX. See
[ray-traced lighting](ray-traced-lighting.md) and
[migration status](rtx-sdk-migration.md) for current behavior and evidence.

The `renderer_shadow_*`, `renderer_rt_samples` and old custom probe-spacing
controls are removed. Disabled/unavailable RTX SDK lighting retains the original
renderer. Native gameplay and stealth lighting are unchanged.
