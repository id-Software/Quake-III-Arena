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

## Integration status

The gateway currently authenticates an agent and reports whether an engine connection exists; it does **not** yet control a running Quake III process. The next engineering step is a narrow, allowlisted command bridge inside the engine/server, with explicit validation and no arbitrary console-command execution. The original player client remains available for ordinary human play; agent authentication is a separate interface.

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
- Ollama (local)
- LM Studio (local)

The first eight providers use their hosted APIs. Ollama and LM Studio use local OpenAI-compatible endpoints. Only providers with server-side credentials (or local providers) are selectable. The browser receives provider names and model IDs, never provider API keys.

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
python3 server.py
```

You do not need to configure every provider. Model IDs can be overridden with OPENAI_MODEL, ANTHROPIC_MODEL, GOOGLE_MODEL, MISTRAL_MODEL, GROQ_MODEL, XAI_MODEL, DEEPSEEK_MODEL, and OPENROUTER_MODEL. Defaults are examples and may need updating as providers retire or rename models.

For local inference, start Ollama or LM Studio on the same host as the gateway. Optional settings: OLLAMA_BASE_URL (default http://127.0.0.1:11434/v1/chat/completions), OLLAMA_MODEL, LM_STUDIO_BASE_URL (default http://127.0.0.1:1234/v1/chat/completions), and LM_STUDIO_MODEL.

The gateway endpoints GET /api/models and POST /api/ai/chat both require the agent bearer key. Prompts are limited to 12,000 characters and request bodies to 64 KiB. Provider calls have a 90-second timeout.

**Scope:** This adds model-provider connectivity for agent tooling. It does not yet translate model output into Quake III game actions or connect to the engine. Review model output before using it to drive any future game-control bridge.
