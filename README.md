# Venky Agent v1

Personal AI agent by Venky.

## Current architecture

User → Agent Brain → Tasks → Tools + Research → Action Router → **Unreal Control Bridge** → Unreal Engine

The project still contains the earlier GitHub read/action code for compatibility, but the main physical-computer control target is now **Unreal Engine**.

## Free-first AI

- Ollama + Gemma 3 4B for local AI
- No cloud API bill required for the local brain
- Optional cloud provider support can be added later

## Run the agent

1. Install/run Ollama and make sure \`gemma3:4b\` is available.
2. In PowerShell, start Ollama:
   \`ollama run gemma3:4b\`
3. In another PowerShell, from the repo folder:
   \`python server.py\`
4. Open:
   \`http://127.0.0.1:8000\`

## Unreal Engine control

The Unreal bridge lives in:

\`unreal/VenkyUnrealBridge/\`

Copy the plugin into your Unreal project's \`Plugins/\` folder and enable **Venky Unreal Bridge** in Unreal Editor.

The bridge listens on:

\`http://127.0.0.1:8765\`

The Venky Agent proxies commands through:

\`http://127.0.0.1:8000/api/unreal/\`

### Example commands in the Venky Agent UI

\`/unrealengine health\`

\`/unrealengine create cube\`

\`/unrealengine create sphere\`

\`/unrealengine move VenkyCube to 300 0 100\`

\`/unrealengine delete VenkyCube\`

\`/unrealengine list\`

\`/unrealengine play\`

\`/unrealengine stop\`

Mutating Unreal commands are shown for confirmation before they are sent.

## Supported Unreal actions

- Health / connection check
- Create Cube, Sphere, Cylinder, or Plane
- Move an actor
- Delete an actor
- List actors in the current level
- Start Play in Editor
- Stop Play in Editor

## End-to-end test

After Unreal Editor and the plugin are running, start Venky Agent and run:

\`python tools/test_unreal_bridge.py\`

The script creates a temporary cube, lists actors, moves the cube, and deletes it.

## Security

- Keep the Unreal bridge on localhost.
- Do not expose port 8765 to the public internet.
- Do not commit API keys or other secrets.
- The Unreal bridge uses an allowlist of explicit editor actions rather than arbitrary code execution.

## Project structure

\`\`\`
va/
├─ index.html
├─ app.js
├─ server.py
├─ config.example.js
├─ tools/
│  └─ test_unreal_bridge.py
└─ unreal/
   └─ VenkyUnrealBridge/
      ├─ VenkyUnrealBridge.uplugin
      ├─ README.md
      └─ Source/
         └─ VenkyUnrealBridge/
            ├─ VenkyUnrealBridge.Build.cs
            └─ Private/
               └─ VenkyUnrealBridgeModule.cpp
\`\`\`
