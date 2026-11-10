#!/usr/bin/env python3
"""Authenticated multi-provider AI gateway for Quake III agent tooling.

Provider API keys stay on this server in environment variables. This gateway
does not execute game commands or connect to the Quake III engine yet.
"""
import hmac
import json
import os
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = os.environ.get("AGENT_GATEWAY_HOST", "127.0.0.1")
PORT = int(os.environ.get("AGENT_GATEWAY_PORT", "8765"))
EXPECTED_KEY = os.environ.get("AGENT_API_KEY", "")
ALLOWED_ORIGINS = {"http://127.0.0.1:8000", "http://localhost:8000"}

# Model IDs can be changed without editing code. Secrets are never sent to the browser.
PROVIDERS = {
    "openai": {"name": "OpenAI", "env": "OPENAI_API_KEY", "model_env": "OPENAI_MODEL", "model": "gpt-4.1-mini", "kind": "openai", "url": "https://api.openai.com/v1/chat/completions"},
    "anthropic": {"name": "Anthropic", "env": "ANTHROPIC_API_KEY", "model_env": "ANTHROPIC_MODEL", "model": "claude-3-7-sonnet-latest", "kind": "anthropic", "url": "https://api.anthropic.com/v1/messages"},
    "google": {"name": "Google Gemini", "env": "GOOGLE_API_KEY", "model_env": "GOOGLE_MODEL", "model": "gemini-2.5-flash", "kind": "gemini"},
    "mistral": {"name": "Mistral", "env": "MISTRAL_API_KEY", "model_env": "MISTRAL_MODEL", "model": "mistral-small-latest", "kind": "openai", "url": "https://api.mistral.ai/v1/chat/completions"},
    "groq": {"name": "Groq", "env": "GROQ_API_KEY", "model_env": "GROQ_MODEL", "model": "llama-3.3-70b-versatile", "kind": "openai", "url": "https://api.groq.com/openai/v1/chat/completions"},
    "xai": {"name": "xAI", "env": "XAI_API_KEY", "model_env": "XAI_MODEL", "model": "grok-3-mini", "kind": "openai", "url": "https://api.x.ai/v1/chat/completions"},
    "deepseek": {"name": "DeepSeek", "env": "DEEPSEEK_API_KEY", "model_env": "DEEPSEEK_MODEL", "model": "deepseek-chat", "kind": "openai", "url": "https://api.deepseek.com/chat/completions"},
    "openrouter": {"name": "OpenRouter", "env": "OPENROUTER_API_KEY", "model_env": "OPENROUTER_MODEL", "model": "openai/gpt-4.1-mini", "kind": "openai", "url": "https://openrouter.ai/api/v1/chat/completions"},
    "ollama": {"name": "Ollama (local)", "env": None, "model_env": "OLLAMA_MODEL", "model": "qwen2.5:7b", "kind": "openai", "url_env": "OLLAMA_BASE_URL", "url": "http://127.0.0.1:11434/v1/chat/completions", "local": True},
    "lmstudio": {"name": "LM Studio (local)", "env": None, "model_env": "LM_STUDIO_MODEL", "model": "local-model", "kind": "openai", "url_env": "LM_STUDIO_BASE_URL", "url": "http://127.0.0.1:1234/v1/chat/completions", "local": True},
}


def provider_model(provider_id, config):
    return os.environ.get(config["model_env"], config["model"])


def provider_available(config):
    if config.get("local"):
        return True
    return bool(os.environ.get(config["env"], ""))


def request_json(url, payload, headers):
    data = json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(url, data=data, headers=headers, method="POST")
    try:
        with urllib.request.urlopen(request, timeout=90) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        detail = error.read(4096).decode("utf-8", "replace")
        raise RuntimeError("Provider returned HTTP %d: %s" % (error.code, detail[:1000]))


def call_provider(provider_id, model, prompt):
    config = PROVIDERS[provider_id]
    api_key = os.environ.get(config["env"], "") if config.get("env") else ""
    if not config.get("local") and not api_key:
        raise RuntimeError("Provider is not configured on the gateway.")
    if config["kind"] == "openai":
        base_url = os.environ.get(config.get("url_env", ""), config["url"])
        headers = {"Content-Type": "application/json"}
        if api_key:
            headers["Authorization"] = "Bearer " + api_key
        payload = {"model": model, "messages": [{"role": "user", "content": prompt}], "temperature": 0.7}
        data = request_json(base_url, payload, headers)
        return data["choices"][0]["message"]["content"]
    if config["kind"] == "anthropic":
        data = request_json(config["url"], {
            "model": model, "max_tokens": 2048,
            "messages": [{"role": "user", "content": prompt}]
        }, {"Content-Type": "application/json", "x-api-key": api_key, "anthropic-version": "2023-06-01"})
        return "".join(part.get("text", "") for part in data.get("content", []) if part.get("type") == "text")
    if config["kind"] == "gemini":
        url = "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s" % (model, api_key)
        data = request_json(url, {"contents": [{"parts": [{"text": prompt}]}]}, {"Content-Type": "application/json"})
        return "".join(part.get("text", "") for part in data["candidates"][0]["content"]["parts"])
    raise RuntimeError("Unsupported provider adapter.")


class Handler(BaseHTTPRequestHandler):
    server_version = "Q3AgentGateway/0.2"

    def add_cors_headers(self):
        origin = self.headers.get("Origin", "")
        if origin in ALLOWED_ORIGINS:
            self.send_header("Access-Control-Allow-Origin", origin)
            self.send_header("Vary", "Origin")
            self.send_header("Access-Control-Allow-Headers", "Authorization, Content-Type")
            self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")

    def send_json(self, status, payload):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.add_cors_headers()
        self.end_headers()
        self.wfile.write(body)

    def authorized(self):
        supplied = self.headers.get("Authorization", "")
        prefix = "Bearer "
        token = supplied[len(prefix):] if supplied.startswith(prefix) else ""
        return bool(EXPECTED_KEY) and hmac.compare_digest(token, EXPECTED_KEY)

    def do_OPTIONS(self):
        self.send_response(204)
        self.add_cors_headers()
        self.end_headers()

    def do_GET(self):
        if not self.authorized():
            self.send_json(401, {"error": "unauthorized"})
            return
        if self.path == "/api/agent/status":
            self.send_json(200, {"name": "Quake III Agent Gateway", "authenticated": True, "engine_connected": False})
            return
        if self.path == "/api/models":
            models = [{"id": key, "name": value["name"], "model": provider_model(key, value),
                       "configured": provider_available(value), "local": bool(value.get("local"))}
                      for key, value in PROVIDERS.items()]
            self.send_json(200, {"providers": models})
            return
        self.send_json(404, {"error": "not_found"})

    def do_POST(self):
        if not self.authorized():
            self.send_json(401, {"error": "unauthorized"})
            return
        if self.path != "/api/ai/chat":
            self.send_json(404, {"error": "not_found"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length < 1 or length > 65536:
                self.send_json(413, {"error": "request_size_invalid"})
                return
            body = json.loads(self.rfile.read(length).decode("utf-8"))
            provider_id = body.get("provider")
            prompt = body.get("prompt", "")
            if provider_id not in PROVIDERS:
                self.send_json(400, {"error": "unknown_provider"})
                return
            if not isinstance(prompt, str) or not prompt.strip() or len(prompt) > 12000:
                self.send_json(400, {"error": "prompt_must_be_1_to_12000_characters"})
                return
            if not provider_available(PROVIDERS[provider_id]):
                self.send_json(400, {"error": "provider_not_configured"})
                return
            model = body.get("model") or provider_model(provider_id, PROVIDERS[provider_id])
            if not isinstance(model, str) or not model.strip() or len(model) > 200:
                self.send_json(400, {"error": "invalid_model"})
                return
            answer = call_provider(provider_id, model, prompt.strip())
            self.send_json(200, {"provider": provider_id, "model": model, "response": answer})
        except (ValueError, KeyError, IndexError, TypeError) as error:
            self.send_json(502, {"error": "invalid_provider_response", "detail": str(error)[:500]})
        except Exception as error:
            self.send_json(502, {"error": "ai_provider_error", "detail": str(error)[:1000]})

    def log_message(self, fmt, *args):
        # Do not log request bodies or authorization headers.
        super().log_message(fmt, *args)


if __name__ == "__main__":
    if len(EXPECTED_KEY) < 32:
        raise SystemExit("Set AGENT_API_KEY to a random value of at least 32 characters.")
    print("Quake III Agent Gateway listening on http://%s:%d" % (HOST, PORT))
    ThreadingHTTPServer((HOST, PORT), Handler).serve_forever()
