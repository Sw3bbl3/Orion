# Lua Deep Dive

Orion extensions run Lua 5.4 scripts with host bindings exposed through `orion.*`.

## Runtime Model

- Scripts are executed on-demand from the Extensions IDE.
- Host calls are bridged from Lua to C++ through the `orion` table.
- UI helpers (`orion.ui.confirm`, `orion.ui.input`) are asynchronous callbacks.

## Practical Rules

- Keep scripts deterministic and quick when changing project state.
- Prefer host APIs (`orion.track`, `orion.clip`, `orion.mixer`) over ad-hoc data files.
- Use `orion.log` for traceability and debugging output.

## Diagnostics and IDE Support

The Extensions IDE includes:

- Generated API dictionary from bound `orion.*` symbols
- Hover docs and symbol lookup
- Problems panel fed by the bundled Lua language server

## Example

```lua
local bpm = orion.project.getBpm()
if bpm < 120 then
    orion.project.setBpm(120)
    orion.log("Raised BPM to 120", 1)
end

orion.transport.play()
```
