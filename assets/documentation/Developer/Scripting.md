# Scripting Guide

This guide covers advanced scripting concepts in Orion.

## Lua Environment
Orion uses Lua 5.4.
The environment is sandboxed but has access to standard Lua libraries:
- `math`
- `string`
- `table`
- `os` (restricted)

## Handling Events
Extensions can respond to events (coming soon). Currently, scripts run immediately when loaded or when triggered manually.

## UI Customization
(Future Feature) Extensions will be able to create custom windows and widgets using the `orion.ui` module.

## Audio Processing
Direct audio processing via Lua is not supported for performance reasons. Use VST3 plugins for DSP. Extensions are intended for workflow automation and project management.
