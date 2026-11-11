#!/usr/bin/env python3
"""Authenticated multi-provider AI gateway for Quake III agent tooling.

Provider API keys stay on this server in environment variables. Authenticated loopback UDP requests carry bounded player telemetry and control-session commands; allowlisted input is executed by the local desktop input bridge.
"""
import hmac
import json
import math
import os
import re
import socketserver
import threading
import time
import uuid

from input_bridge import backend_name, execute_action
from asset_catalog import build_catalog
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = os.environ.get("AGENT_GATEWAY_HOST", "127.0.0.1")
PORT = int(os.environ.get("AGENT_GATEWAY_PORT", "8765"))
EXPECTED_KEY = os.environ.get("AGENT_API_KEY", "")
ALLOWED_ORIGINS = {"http://127.0.0.1:8000", "http://localhost:8000", "null"}
MODERATION_UDP_HOST = "127.0.0.1"
MODERATION_UDP_PORT = 27991
_moderation_lock = threading.Lock()
_moderation_latest = {"id": "", "timestamp": 0, "reason": ""}


def record_moderation_event(datagram):
    """Accept only the fixed game-server event marker; discard all other data."""
    if datagram != b"Q3MOD1":
        return False
    event = {"id": uuid.uuid4().hex, "timestamp": int(time.time()),
             "reason": "chat_policy_violation"}
    with _moderation_lock:
        _moderation_latest.update(event)
    return True


class ModerationUDPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        record_moderation_event(self.request[0])


class ModerationUDPServer(socketserver.ThreadingUDPServer):
    allow_reuse_address = True
    daemon_threads = True


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
    "nvidia": {"name": "NVIDIA NIM", "env": "NVIDIA_API_KEY", "model_env": "NVIDIA_MODEL", "model": "nvidia/llama-3.3-nemotron-super-49b-v1.5", "kind": "openai", "url": "https://integrate.api.nvidia.com/v1/chat/completions"},
    "nvidia-nim-local": {"name": "NVIDIA NIM (self-hosted)", "env": None, "model_env": "NVIDIA_NIM_MODEL", "model": "meta/llama-3.1-8b-instruct", "kind": "openai", "url_env": "NVIDIA_NIM_BASE_URL", "url": "http://127.0.0.1:8000/v1/chat/completions", "local": True},
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



AGENT_UDP_HOST = "127.0.0.1"
AGENT_UDP_PORT = int(os.environ.get("Q3_AGENT_UDP_PORT", "27992"))
AGENT_UDP_PROVIDERS = set(PROVIDERS.keys())



_agent_state_lock = threading.Lock()
_agent_state = {"received_at": 0.0, "data": None}


def record_agent_state(datagram):
    if len(datagram) > 512:
        return False
    try:
        item = json.loads(datagram.decode("utf-8"))
        if not isinstance(item, dict) or item.get("type") != "Q3STATE1":
            return False
        key = item.get("key")
        if not isinstance(key, str) or not EXPECTED_KEY or not hmac.compare_digest(key, EXPECTED_KEY):
            return False
        origin, velocity, angles = item.get("origin"), item.get("velocity"), item.get("angles")
        if not all(isinstance(v, list) and len(v) == 3 for v in (origin, velocity, angles)):
            return False
        values = origin + velocity + angles
        if any(type(v) not in (int, float) or not math.isfinite(v) or abs(v) > 100000 for v in values):
            return False
        health, weapon, server_time = item.get("health"), item.get("weapon"), item.get("server_time")
        if type(health) is not int or not -1000 <= health <= 10000:
            return False
        if type(weapon) is not int or not 0 <= weapon <= 64:
            return False
        if type(server_time) is not int or server_time < 0:
            return False
        safe = {"origin": origin, "velocity": velocity, "angles": angles,
                "health": health, "weapon": weapon, "server_time": server_time}
        with _agent_state_lock:
            _agent_state["received_at"] = time.monotonic()
            _agent_state["data"] = safe
        return True
    except (ValueError, UnicodeDecodeError, TypeError):
        return False


def current_agent_state():
    with _agent_state_lock:
        data = _agent_state["data"]
        age = time.monotonic() - _agent_state["received_at"]
        if data is None or age > 2.5:
            return None
        return {"age_seconds": round(age, 2), **data}


_agent_session_lock = threading.Lock()
_agent_session = None
_AGENT_MAX_SESSION_SECONDS = 30 * 60
_AGENT_ACTION_INTERVAL_SECONDS = 0.35

def _agent_session_worker(stop_event, provider_id, prompt):
    config = PROVIDERS[provider_id]
    model = provider_model(provider_id, config)
    deadline = time.monotonic() + _AGENT_MAX_SESSION_SECONDS
    step = 0
    while not stop_event.is_set() and time.monotonic() < deadline:
        step += 1
        telemetry = current_agent_state()
        if telemetry is None and step == 1:
            stop_event.wait(0.8)
            telemetry = current_agent_state()
        if telemetry is None:
            print("AI continuous session stopped: player telemetry is missing or stale.")
            break
        state_text = json.dumps(telemetry, separators=(",", ":"))
        control_prompt = (
            "Continuous Quake III control session. Choose the next short movement pulse "
            "for this task. Return ONLY JSON: move (none|forward|back), "
            "strafe (none|left|right), jump/crouch/attack (booleans), "
            "look_x/look_y (integers -100..100), duration_ms (integer 40..250). "
            "No extra keys or markdown. Release all keys after every pulse. "
            "You receive limited player telemetry, but no screenshots, enemy positions, or map "
            "geometry. Use the supplied telemetry as the only observation and do not "
            "invent unseen events. If task complete or unable to progress safely, "
            "return neutral controls. Latest player telemetry: " + state_text + ". Task: " + prompt
        )
        try:
            answer = call_provider(provider_id, model, control_prompt).strip()
            tick = chr(96)
            if answer.startswith(tick * 3):
                answer = answer.strip().strip(tick).strip()
                if answer[:4].lower() == "json":
                    answer = answer[4:].strip()
            action = json.loads(answer)
            if stop_event.is_set():
                break
            if current_agent_state() is None:
                print("AI continuous session stopped: player telemetry became stale.")
                break
            result = execute_action(action)
            if step == 1 or step % 20 == 0:
                print("AI continuous session: pulse %d (%dms)" % (step, result["duration_ms"]))
        except Exception as error:
            print("AI continuous session stopped after error: %s" % str(error)[:240])
            break
        stop_event.wait(_AGENT_ACTION_INTERVAL_SECONDS)
    if time.monotonic() >= deadline:
        print("AI continuous session reached its 30-minute safety limit.")
    stop_event.set()

def start_agent_session(provider_id, prompt):
    global _agent_session
    with _agent_session_lock:
        if _agent_session is not None:
            _agent_session["stop"].set()
        stop_event = threading.Event()
        worker = threading.Thread(target=_agent_session_worker,
            args=(stop_event, provider_id, prompt),
            name="q3-agent-control-session", daemon=True)
        _agent_session = {"stop": stop_event, "thread": worker}
        worker.start()

def stop_agent_session():
    with _agent_session_lock:
        if _agent_session is None:
            return False
        _agent_session["stop"].set()
        return True




def parse_agent_datagram(datagram):
    if len(datagram) > 1400:
        raise ValueError("agent_datagram_too_large")
    request = json.loads(datagram.decode("utf-8"))
    if not isinstance(request, dict) or request.get("type") != "Q3AI1":
        raise ValueError("invalid_agent_datagram")
    key, provider, prompt = request.get("key"), request.get("provider"), request.get("prompt")
    if not isinstance(key, str) or len(key) < 32 or len(key) > 127:
        raise ValueError("invalid_agent_key")
    if not isinstance(provider, str) or provider not in AGENT_UDP_PROVIDERS:
        raise ValueError("unknown_provider")
    if not isinstance(prompt, str) or not prompt.strip() or len(prompt) > 240:
        raise ValueError("invalid_agent_prompt")
    if not key.startswith("q3a_") or any(ch not in "0123456789abcdefABCDEF" for ch in key[4:]):
        raise ValueError("invalid_agent_key_format")
    continuous = request.get("continuous", False)
    if not isinstance(continuous, bool):
        raise ValueError("continuous_must_be_boolean")
    return key, provider, prompt.strip(), continuous


def run_agent_datagram(datagram):
    key, provider_id, prompt, continuous = parse_agent_datagram(datagram)
    if not EXPECTED_KEY or not hmac.compare_digest(key, EXPECTED_KEY):
        raise PermissionError("unauthorized")
    config = PROVIDERS[provider_id]
    if not provider_available(config):
        raise RuntimeError("provider_not_configured")
    model = provider_model(provider_id, config)
    if continuous:
        start_agent_session(provider_id, prompt)
        return {"provider": provider_id, "session": "started", "continuous": True}
    control_prompt = (
        "Control a Quake III player with one short input pulse. Return ONLY JSON: "
        "move (none|forward|back), strafe (none|left|right), jump (boolean), "
        "crouch (boolean), attack (boolean), look_x/look_y (integers -100..100), "
        "duration_ms (integer 40..250). No extra fields or markdown. Use 120ms. "
        "No screenshots or game telemetry are available; do not claim to see the game. "
        "User task: " + prompt
    )
    answer = call_provider(provider_id, model, control_prompt).strip()
    if answer.startswith("\`\`\`"):
        answer = re.sub(r"^\`\`\`(?:json)?\s*|\s*\`\`\`$", "", answer, flags=re.IGNORECASE)
    result = execute_action(json.loads(answer))
    return {"provider": provider_id, "model": model, "control": result}


class AgentUDPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        try:
            if self.request[0].startswith(b'{"type":"Q3STATE1"'):
                record_agent_state(self.request[0])
                return
            if self.request[0].startswith(b"Q3STOP1|"):
                key = self.request[0][8:].decode("ascii")
                if not EXPECTED_KEY or not hmac.compare_digest(key, EXPECTED_KEY):
                    raise PermissionError("unauthorized")
                stopped = stop_agent_session()
                print("AI continuous session stop requested." if stopped else "No AI session was running.")
                return
            result = run_agent_datagram(self.request[0])
            if result.get("continuous"):
                print("AI continuous session started (%s)." % result["provider"])
            else:
                print("AI control pulse executed (%s, %dms)" % (result["provider"], result["control"]["duration_ms"]))
        except PermissionError:
            print("Rejected unauthenticated local AI control request.")
        except Exception as error:
            print("AI control request failed: %s" % str(error)[:240])


class AgentUDPServer(socketserver.ThreadingUDPServer):
    allow_reuse_address = True
    daemon_threads = True


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
        if self.path == "/api/moderation/latest":
            with _moderation_lock:
                event = dict(_moderation_latest)
            self.send_json(200, {"event": event})
            return
        if self.path == "/api/agent/status":
            self.send_json(200, {"name": "Quake III Agent Gateway", "authenticated": True, "engine_connected": current_agent_state() is not None, "input_backend": backend_name()})
            return
        if self.path == "/api/models":
            models = [{"id": key, "name": value["name"], "model": provider_model(key, value),
                       "configured": provider_available(value), "local": bool(value.get("local"))}
                      for key, value in PROVIDERS.items()]
            self.send_json(200, {"providers": models})
            return
        if self.path == "/api/assets":
            catalog = build_catalog()
            self.send_json(200, catalog)
            return
        self.send_json(404, {"error": "not_found"})

    def do_POST(self):
        if not self.authorized():
            self.send_json(401, {"error": "unauthorized"})
            return
        if self.path not in ("/api/ai/chat", "/api/ai/control", "/api/agent/control", "/api/ai/asset-plan"):
            self.send_json(404, {"error": "not_found"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length < 1 or length > 65536:
                self.send_json(413, {"error": "request_size_invalid"})
                return
            body = json.loads(self.rfile.read(length).decode("utf-8"))
            if self.path == "/api/agent/control":
                result = execute_action(body.get("action"))
                self.send_json(200, result)
                return
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
            if self.path == "/api/ai/asset-plan":
                if len(prompt) > 2000:
                    self.send_json(400, {"error": "asset_request_too_long"})
                    return
                catalog = build_catalog()
                if not catalog["archives"]:
                    self.send_json(409, {"error": "no_pk3_archives_found",
                                         "detail": "Place your legally obtained .pk3 files in Q3_ASSET_ROOT."})
                    return
                compact = {
                    "players": [{"name": item["name"], "skins": item["skins"][:12],
                                 "tags": item["tags"][:20],
                                 "has_animation_cfg": item["has_animation_cfg"],
                                 "md3_count": len(item["md3"])} for item in catalog["players"][:300]],
                    "weapons": [{"name": item["name"], "tags": item["tags"][:20],
                                 "md3_count": len(item["md3"])} for item in catalog["weapons"][:200]],
                    "mapobjects": [{"name": item["name"], "tags": item["tags"][:20],
                                   "md3_count": len(item["md3"])} for item in catalog["mapobjects"][:300]],
                    "maps": catalog["maps"][:500],
                }
                plan_prompt = (
                    "Choose assets ONLY from this Quake III asset catalog. Return only one JSON object "
                    "with keys player_model, weapon_model, map, mapobject, reason. Use exact catalog "
                    "names; use null for any category with no suitable match. Keep reason under 400 chars. "
                    "Do not invent assets. Catalog: " + json.dumps(compact, separators=(",", ":")) +
                    "\nUser's desired setup: " + prompt.strip()
                )
                answer = call_provider(provider_id, model, plan_prompt)
                cleaned = answer.strip()
                if cleaned.startswith("```"):
                    cleaned = re.sub(r"^```(?:json)?\s*|\s*```$", "", cleaned, flags=re.IGNORECASE)
                plan = json.loads(cleaned)
                allowed = {
                    "player_model": {item["name"] for item in catalog["players"]},
                    "weapon_model": {item["name"] for item in catalog["weapons"]},
                    "map": set(catalog["maps"]),
                    "mapobject": {item["name"] for item in catalog["mapobjects"]},
                }
                for field, choices in allowed.items():
                    value = plan.get(field)
                    if value is not None and value not in choices:
                        self.send_json(502, {"error": "model_selected_unknown_asset",
                                             "field": field, "value": value})
                        return
                result = {field: plan.get(field) for field in allowed}
                result["reason"] = str(plan.get("reason", ""))[:400]
                result["catalog_counts"] = catalog["counts"]
                self.send_json(200, {"provider": provider_id, "model": model, "plan": result})
                return
            if self.path == "/api/ai/control":
                control_prompt = ("You are controlling a Quake III Arena player in a live multiplayer match. " +
                    "Translate the request into exactly one short control pulse. Return ONLY a JSON object with fields " +
                    "move (none|forward|back), strafe (none|left|right), jump (boolean), crouch (boolean), " +
                    "attack (boolean), look_x (integer -100..100), look_y (integer -100..100), " +
                    "duration_ms (integer 40..250). No markdown or explanation. Do not invent fields. " +
                    "Use 120ms by default and release all keys after each pulse. User request: " + prompt.strip())
                answer = call_provider(provider_id, model, control_prompt)
                cleaned = answer.strip()
                if cleaned.startswith("```"):
                    cleaned = re.sub(r"^```(?:json)?\s*|\s*```$", "", cleaned, flags=re.IGNORECASE)
                action = json.loads(cleaned)
                result = execute_action(action)
                self.send_json(200, {"provider": provider_id, "model": model, "response": answer, "control": result})
            else:
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
    agent_server = AgentUDPServer((AGENT_UDP_HOST, AGENT_UDP_PORT), AgentUDPHandler)
    threading.Thread(target=agent_server.serve_forever, name="q3-agent-udp", daemon=True).start()
    moderation_server = ModerationUDPServer(
        (MODERATION_UDP_HOST, MODERATION_UDP_PORT), ModerationUDPHandler)
    threading.Thread(target=moderation_server.serve_forever,
                     name="q3-moderation-udp", daemon=True).start()
    print("Quake III Agent Gateway listening on http://%s:%d" % (HOST, PORT))
    print("Quake III AI control listening on UDP %s:%d" %
          (AGENT_UDP_HOST, AGENT_UDP_PORT))
    print("Quake III moderation events listening on UDP %s:%d" %
          (MODERATION_UDP_HOST, MODERATION_UDP_PORT))
    ThreadingHTTPServer((HOST, PORT), Handler).serve_forever()
