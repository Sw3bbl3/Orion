# Extensions Development

Orion supports Lua scripting for extending functionality. Extensions can automate tasks, create custom UI elements, and interact with the engine.

## Directory Structure
Extensions live in your User Data Folder under `Extensions/`.
Each extension is a folder containing a `main.lua` file.

Example:
```
Documents/Orion/Extensions/MyExtension/
  - main.lua
  - resource.png
```

## The 'orion' Global
All Orion API functions are exposed through the global `orion` table.

### API Reference

#### `orion.log(message, [level])`
Logs a message to the Orion console and the log file.
- **message**: The string to log.
- **level**: (Optional) The log level (0: Info, 1: Warning, 2: Error, 3: Success). Defaults to 0.

Example:
```lua
orion.log("Operation successful", 3)
```

#### `orion.alert(title, message)`
Displays a native alert dialog box.
- **title**: The title of the window.
- **message**: The content of the message.

Example:
```lua
orion.alert("Success", "The operation completed successfully.")
```

#### `orion.getVersion()`
Returns the current version of Orion as a string.

Example:
```lua
local version = orion.getVersion()
orion.log("Running on Orion " .. version)
```

## Creating a New Extension
1. Go to **Dashboard > Extensions**.
2. Click the **Plus (+)** icon.
3. Enter a name for your extension.
4. Orion will create the folder and a basic `main.lua` for you.
5. Click the extension in the list to edit the script.

## Packaging and Sharing
Extensions can be exported as `.oex` files (Orion Extension) for easy sharing.
- **Export**: Select an extension and click the **Export** icon.
- **Import**: Click the **Import** icon and select an `.oex` file.
