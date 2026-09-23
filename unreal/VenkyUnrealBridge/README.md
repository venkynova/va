# Venky Unreal Bridge

This plugin connects the local **Venky Agent** to the **Unreal Editor** through a localhost HTTP bridge.

## What it does

Venky Agent talks to:

`http://127.0.0.1:8000/api/unreal/...`

The Python server forwards Unreal commands to:

`http://127.0.0.1:8765`

The Unreal Editor plugin listens on that port and executes a small allowlisted set of editor actions.

Supported commands:

- `health`
- `create_actor` / `create_primitive` — Cube, Sphere, Cylinder, Plane
- `move_actor`
- `delete_actor`
- `list_actors`
- `play`
- `stop`

## Install into an Unreal project

1. Copy the entire `VenkyUnrealBridge` folder into your Unreal project's:
   `Plugins/`
2. Open the `.uproject` file.
3. Enable **Venky Unreal Bridge** in **Edit → Plugins**.
4. Restart Unreal Editor if it asks.
5. If Unreal asks to build the C++ plugin, install the C++ build tools required by your Unreal Engine installation and let Unreal compile it.

The plugin is an **Editor** module; it is intended to run inside the Unreal Editor, not in a packaged game.

## First test

With Unreal Editor open and the plugin enabled, run Venky Agent:

`python server.py`

Then open:

`http://127.0.0.1:8000`

Try:

`/unrealengine health`

A connected response should come back from Unreal.

Then:

`/unrealengine create cube`

A cube should appear in the current editor level.

## Command examples

### Create a cube

```json
{
  "action": "create_actor",
  "primitive": "Cube",
  "name": "VenkyCube",
  "location": [0, 0, 100]
}
```

### Move an actor

```json
{
  "action": "move_actor",
  "name": "VenkyCube",
  "location": [300, 0, 100]
}
```

### Delete an actor

```json
{
  "action": "delete_actor",
  "name": "VenkyCube"
}
```

## Local-only design

The intended traffic path is:

`Browser → Venky Agent → 127.0.0.1:8765 → Unreal Editor`

Do not expose the bridge port to the public internet.
