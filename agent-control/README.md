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
