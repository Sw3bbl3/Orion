# UI Design for Extensions

Orion's extension surface is automation-first. Current UI APIs are under `orion.ui`.

## Available UI Helpers

- `orion.ui.confirm(title, message, callback)`
- `orion.ui.input(title, message, callback)`

Both APIs are callback-based and run asynchronously.

## Pattern

```lua
orion.ui.input("Rename Clip", "Enter new name:", function(value)
    if value ~= nil and value ~= "" then
        orion.clip.setClipName(1, 1, value)
        orion.log("Clip renamed to " .. value, 3)
    end
end)
```

## UX Guidance

- Keep prompts short and specific.
- Use `orion.log` to confirm outcomes after user interaction.
- Avoid long chains of blocking prompts.
