# Quake III AI Agent Control Panel

This directory adds an initial, separate graphical credential panel and a small Python standard-library gateway. It intentionally leaves the original Quake III engine and game modules unchanged.

## What works now

- Generate a 256-bit random agent key in the browser using the Web Crypto API.
- Copy the generated key; the page does not save it to local storage.
- Verify a bearer key against the gateway's authenticated status endpoint.
- Reject missing or incorrect credentials with HTTP 401.
- Compare credentials using Python's constant-time `hmac.compare_digest`.

## Run locally

Use Python 3.8+:

```sh
cd agent-control
export AGENT_API_KEY='paste-the-generated-key-here'
python3 server.py
```

Open `http://127.0.0.1:8765` only after serving the static files from a local HTTP server, for example in a second terminal:

```sh
python3 -m http.server 8000
```

Then open `http://127.0.0.1:8000` from the `agent-control` directory. Set the gateway URL in the panel to `http://127.0.0.1:8765`.

For a remote deployment, use HTTPS, keep the key in a protected environment variable or secret manager, restrict network access, and do not expose the development server directly to the public internet. The sample server binds to loopback by default.

## Live multiplayer input bridge

The gateway sends short, allowlisted desktop input pulses to the foreground Quake III Arena client. It does not patch the engine or inject code into the process: it uses the normal keyboard/mouse input path, so the client sends ordinary player commands through its multiplayer network path.

Supported actions: forward/back, left/right strafe, jump, crouch, attack, and relative mouse look. Each pulse is limited to 40–250 ms and held keys/buttons are released afterward. The bridge never runs Quake console commands or arbitrary shell commands.

- Windows: native Windows user32 keyboard/mouse events; foreground game-window title is checked.
- Linux X11: requires `xdotool` and a focused Quake III window.
- Linux Wayland: requires `ydotool` plus the `ydotoold` daemon and uinput permissions. Wayland has no compositor-independent API for querying the focused application's title, so set `Q3_AGENT_ALLOW_UNVERIFIED_WAYLAND_FOCUS=1` only after focusing Quake III. This is an explicit opt-in; the bridge cannot verify focus on Wayland.
- macOS: uses built-in CoreGraphics events and checks the frontmost application with AppleScript. The terminal/Python process may need Accessibility permission in System Settings → Privacy & Security → Accessibility.

The `/api/ai/control` endpoint asks the selected model for one JSON control pulse, validates the schema, and sends it to the focused game. `/api/agent/control` accepts a precomputed action object from an authenticated external agent. Both require the agent bearer key. Repeated requests are needed for continuous movement; the bridge intentionally does not leave keys held between requests. Only use automated play on servers where it is permitted.

## Linux Wayland and macOS setup

### Linux Wayland

Install `ydotool` using your distribution's package manager and start its `ydotoold` daemon in the same logged-in session. Ensure the user can access the daemon socket and uinput device. Focus Quake III, then start the gateway with:

```sh
export Q3_AGENT_ALLOW_UNVERIFIED_WAYLAND_FOCUS=1
python3 server.py
```

The opt-in is necessary because Wayland intentionally prevents a generic application from inspecting another application's focus. Do not change focus while agent control is active.

### macOS

No third-party Python package is required. Start the gateway in the logged-in graphical session. If macOS prompts for permission, grant Accessibility access to Terminal or the Python process under **System Settings → Privacy & Security → Accessibility**. Keep Quake III as the frontmost application.

Both backends preserve the bounded action schema. Their behavior should be tested on the target compositor/macOS version before live multiplayer use.

## Key handling

The generated key is shown in the page and copied only on user action. It is not persisted by the page. Treat it like a password: do not commit it, paste it into issues, or include it in logs. Rotate it by generating a new value and restarting the gateway with the replacement.

## Multi-provider AI model gateway

The authenticated gateway includes a provider selector and prompt interface. It supports OpenAI, Anthropic, Google Gemini, Mistral, Groq, xAI, DeepSeek, OpenRouter, NVIDIA NIM (hosted and self-hosted), Ollama, and LM Studio. Provider API keys stay on the server. Only configured providers are selectable.

Set `AGENT_API_KEY` and whichever provider credentials you use. Model IDs and local endpoint URLs can be overridden with the environment variables documented in the gateway configuration. Defaults are examples and may need updating as providers retire or rename models.

The gateway endpoints `GET /api/models`, `POST /api/ai/chat`, `POST /api/ai/control`, and `POST /api/agent/control` require the agent bearer key. Prompts are limited to 12,000 characters and request bodies to 64 KiB. Provider calls have a 90-second timeout.

## Live control API example

```sh
curl -X POST http://127.0.0.1:8765/api/agent/control \
  -H "Authorization: Bearer $AGENT_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{"action":{"move":"forward","jump":true,"duration_ms":150}}'
```

## Input bridge validation tests

Run from this directory:

```sh
python3 -m unittest -v test_input_bridge.py
```

The tests validate action allowlisting and focus guards. They do not replace live desktop testing on each target OS/compositor.

## Quake III PK3 asset discovery and AI selection

The gateway includes a read-only PK3 catalog for the original game assets. It scans archive directory entries without extracting files or modifying the archives.

Set `Q3_ASSET_ROOT` to the directory containing your legally obtained `.pk3` files. For example:

```sh
cd agent-control
export Q3_ASSET_ROOT="$HOME/quake3/baseq3"
export AGENT_API_KEY='your-agent-key'
python3 server.py
```

On Windows PowerShell:

```powershell
$env:Q3_ASSET_ROOT = "C:\\Games\\Quake III Arena\\baseq3"
$env:AGENT_API_KEY = "your-agent-key"
python server.py
```

Copy your own `pak0.pk3` and any other game PK3 archives into that folder. Do not commit the archives to this repository. The scanner looks for up to 64 top-level PK3 files and indexes:

- Player model directories under `models/players/`, including MD3/MDC files, skin files, animation.cfg presence and MD3 attachment tag names.
- Weapon model directories under `models/weapons2/`.
- Map-object groups under `models/mapobjects/`.
- BSP maps under `maps/`, plus shader and texture counts.

After verifying gateway access, use **Scan local Quake III PK3 archives** in the control panel. The authenticated `GET /api/assets` endpoint returns the catalog. The **Ask AI to select assets** button calls `POST /api/ai/asset-plan`; the model receives a compact catalog and must return exact asset names. The gateway validates every selected name against the scanned catalog before returning it. Unknown or invented asset names are rejected.

The asset selector currently provides discovery and validated planning only. It does not yet change the running game's player configuration, spawn a map, or hot-load models in a live match; that requires an engine-side runtime integration. The catalog and selection API are read-only with respect to game archives.

Run the asset catalog unit tests from this directory:

```sh
python3 -m unittest -v test_asset_catalog.py
```
