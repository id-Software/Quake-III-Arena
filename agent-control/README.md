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

The gateway can now send short, allowlisted desktop input pulses to the foreground Quake III Arena client. It does not patch the engine or inject code into the process: it uses the same keyboard/mouse input path as a human player, so the normal client user-command and multiplayer network path remains in use.

Supported actions: forward/back, left/right strafe, jump, crouch, attack, and relative mouse look. Each pulse is limited to 40–250 ms and all held keys/buttons are released afterward. The foreground window title must identify Quake III Arena. The bridge never runs console commands or arbitrary shell commands.

- Windows: uses the native Windows keyboard/mouse input API.
- Linux X11: requires `xdotool` and a Quake III window with focus.
- Linux Wayland-only sessions and macOS are not supported by this input backend yet.

The `/api/ai/control` endpoint asks the selected model for one JSON control pulse, validates the schema, and sends it to the focused game. `/api/agent/control` accepts a precomputed action object from an authenticated external agent. Both require the agent bearer key. Repeated requests are needed for continuous movement; the bridge intentionally does not leave keys held between requests. The game window must be focused, and the user should only use this against servers where automated play is permitted.

## Key handling

The generated key is shown in the page and copied only on user action. It is not persisted by the page. Treat it like a password: do not commit it, paste it into issues, or include it in logs. Rotate it by generating a new value and restarting the gateway with the replacement.

## Multi-provider AI model gateway

The authenticated gateway now includes a provider selector and prompt interface. It supports:

- OpenAI
- Anthropic
- Google Gemini
- Mistral
- Groq
- xAI
- DeepSeek
- OpenRouter
- NVIDIA NIM (hosted NVIDIA API Catalog)
- NVIDIA NIM (self-hosted)
- Ollama (local)
- LM Studio (local)

The hosted NVIDIA NIM option uses NVIDIA's OpenAI-compatible API Catalog endpoint. The self-hosted NVIDIA NIM option uses a local OpenAI-compatible NIM endpoint. The other hosted providers use their respective APIs. Ollama and LM Studio use local OpenAI-compatible endpoints. Only providers with server-side credentials (or local providers) are selectable. The browser receives provider names and model IDs, never provider API keys.

### Configure providers

Set AGENT_API_KEY as before, then add whichever provider credentials you use:

```sh
export OPENAI_API_KEY='...'
export ANTHROPIC_API_KEY='...'
export GOOGLE_API_KEY='...'
export MISTRAL_API_KEY='...'
export GROQ_API_KEY='...'
export XAI_API_KEY='...'
export DEEPSEEK_API_KEY='...'
export OPENROUTER_API_KEY='...'
export NVIDIA_API_KEY='...'
# Optional: choose a different NVIDIA-hosted model
export NVIDIA_MODEL='nvidia/llama-3.3-nemotron-super-49b-v1.5'
python3 server.py
```

You do not need to configure every provider. Model IDs can be overridden with OPENAI_MODEL, ANTHROPIC_MODEL, GOOGLE_MODEL, MISTRAL_MODEL, GROQ_MODEL, XAI_MODEL, DEEPSEEK_MODEL, and OPENROUTER_MODEL. Defaults are examples and may need updating as providers retire or rename models.

For NVIDIA-hosted inference, create an API key in the NVIDIA API Catalog and set NVIDIA_API_KEY. NVIDIA model IDs can be selected in the panel's editable Model ID field; the default is nvidia/llama-3.3-nemotron-super-49b-v1.5. For self-hosted NVIDIA NIM, start a NIM container and set NVIDIA_NIM_BASE_URL (default http://127.0.0.1:8000/v1/chat/completions) and optionally NVIDIA_NIM_MODEL (default meta/llama-3.1-8b-instruct). The self-hosted endpoint does not require an API key by default; configure your deployment's authentication separately if exposed beyond localhost.

For local inference, start Ollama or LM Studio on the same host as the gateway. Optional settings: OLLAMA_BASE_URL (default http://127.0.0.1:11434/v1/chat/completions), OLLAMA_MODEL, LM_STUDIO_BASE_URL (default http://127.0.0.1:1234/v1/chat/completions), and LM_STUDIO_MODEL.

The gateway endpoints GET /api/models and POST /api/ai/chat both require the agent bearer key. Prompts are limited to 12,000 characters and request bodies to 64 KiB. Provider calls have a 90-second timeout.

**Scope:** This adds model-provider connectivity for agent tooling. It does not yet translate model output into Quake III game actions or connect to the engine. Review model output before using it to drive any future game-control bridge.


## Live control API example

Send a short forward-and-jump pulse to the focused game window:

```sh
curl -X POST http://127.0.0.1:8765/api/agent/control \
  -H "Authorization: Bearer $AGENT_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{"action":{"move":"forward","jump":true,"duration_ms":150}}'
```

Ask the selected AI provider to choose and execute a pulse through
`POST /api/ai/control` using the same provider/model/prompt fields as the chat
endpoint. The response includes the validated action and foreground window.

Linux X11 setup: install `xdotool` from your distribution packages. Start the
gateway in the same desktop session as Quake III. Keep the game window focused.


## Input bridge validation tests

Run the standard-library unit tests from this directory:

```sh
python3 -m unittest -v test_input_bridge.py
```

These tests cover action allowlisting, direction and duration bounds, mouse
limits, button types, and the foreground-window guard. They do not replace a
live Windows/Linux desktop test with a running Quake III multiplayer client.
