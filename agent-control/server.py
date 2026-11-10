#!/usr/bin/env python3
"""Small authenticated gateway for the Quake III AI-agent control panel.

Set AGENT_API_KEY in the process environment before starting this server.
This is an authentication scaffold; it does not yet issue game commands.
"""
import hmac
import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = os.environ.get("AGENT_GATEWAY_HOST", "127.0.0.1")
PORT = int(os.environ.get("AGENT_GATEWAY_PORT", "8765"))
EXPECTED_KEY = os.environ.get("AGENT_API_KEY", "")


class Handler(BaseHTTPRequestHandler):
    server_version = "Q3AgentGateway/0.1"

    def send_json(self, status, payload):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path != "/api/agent/status":
            self.send_json(404, {"error": "not_found"})
            return
        supplied = self.headers.get("Authorization", "")
        prefix = "Bearer "
        token = supplied[len(prefix):] if supplied.startswith(prefix) else ""
        if not EXPECTED_KEY or not hmac.compare_digest(token, EXPECTED_KEY):
            self.send_json(401, {"error": "unauthorized"})
            return
        self.send_json(200, {
            "name": "Quake III Agent Gateway",
            "authenticated": True,
            "engine_connected": False
        })

    def log_message(self, fmt, *args):
        # Avoid logging request headers, which may contain credentials.
        super().log_message(fmt, *args)


if __name__ == "__main__":
    if len(EXPECTED_KEY) < 32:
        raise SystemExit("Set AGENT_API_KEY to a random value of at least 32 characters.")
    print("Quake III Agent Gateway listening on http://%s:%d" % (HOST, PORT))
    ThreadingHTTPServer((HOST, PORT), Handler).serve_forever()
