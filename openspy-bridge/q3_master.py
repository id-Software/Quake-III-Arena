#!/usr/bin/env python3
"""Small IPv4 Quake III master directory for native id Tech 3 clients.

This is the Quake III-facing side of a future OpenSpy integration. It speaks
Quake III's UDP master protocol; it does NOT speak GameSpy QR2 and therefore
does not publish servers into OpenSpy by itself.
"""
import argparse
import asyncio
import ipaddress
import json
import os
import struct
import time
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread

CONNECTIONLESS = b"\xff\xff\xff\xff"
RESPONSE_PREFIX = CONNECTIONLESS + b"getserversResponse\\"
MAX_PACKET = 1200
DEFAULT_TTL = 180
MAX_SERVERS = 4096


@dataclass
class Server:
    host: str
    port: int
    game: str
    last_seen: float


class Directory:
    def __init__(self, ttl=DEFAULT_TTL, games=None, protocols=None):
        self.ttl = ttl
        self.games = set(games or ("Quake3", "quake3", "QuakeArena-1", "ioquake3"))
        self.protocols = set(protocols or (68,))
        self.servers = {}

    def expire(self, now=None):
        now = time.monotonic() if now is None else now
        self.servers = {key: item for key, item in self.servers.items()
                        if now - item.last_seen <= self.ttl}

    def heartbeat(self, address, game, now=None):
        if game not in self.games:
            return False
        host, port = address[0], int(address[1])
        try:
            ipaddress.IPv4Address(host)
        except ipaddress.AddressValueError:
            return False
        if not 1 <= port <= 65535:
            return False
        now = time.monotonic() if now is None else now
        key = (host, port)
        if key not in self.servers and len(self.servers) >= MAX_SERVERS:
            self.expire(now)
            if len(self.servers) >= MAX_SERVERS:
                return False
        self.servers[key] = Server(host, port, game, now)
        return True

    def shutdown(self, address):
        self.servers.pop((address[0], int(address[1])), None)

    def list_packet(self, game, protocol, now=None):
        if game not in self.games or protocol not in self.protocols:
            return [RESPONSE_PREFIX + b"\\EOT"]
        self.expire(now)
        entries = []
        for server in sorted(self.servers.values(), key=lambda item: (item.host, item.port)):
            if server.game != game:
                continue
            entries.append(ipaddress.IPv4Address(server.host).packed + struct.pack("!H", server.port))
        # Keep packets below a conservative MTU. Each response fragment ends
        # with a continuation marker; the final fragment ends with \\EOT.
        packets = []
        current = bytearray(RESPONSE_PREFIX)
        for entry in entries:
            if len(current) + len(entry) + 4 > MAX_PACKET:
                current.extend(b"\\")
                packets.append(bytes(current))
                current = bytearray(RESPONSE_PREFIX)
            current.extend(entry)
        current.extend(b"\\EOT")
        packets.append(bytes(current))
        return packets


class MasterProtocol(asyncio.DatagramProtocol):
    def __init__(self, directory):
        self.directory = directory
        self.transport = None
        self.last_query = {}

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, address):
        if not data.startswith(CONNECTIONLESS) or len(data) > 4096:
            return
        try:
            command = data[4:].split(b"\x00", 1)[0].decode("ascii", "strict").strip()
        except UnicodeDecodeError:
            return
        if command.startswith("heartbeat "):
            game = command[len("heartbeat "):].split()[0]
            self.directory.heartbeat(address, game)
            return
        if command == "shutdown":
            self.directory.shutdown(address)
            return
        if command.startswith("getservers "):
            # Avoid turning this into an unrestricted UDP reflector.
            now = time.monotonic()
            host = address[0]
            if now - self.last_query.get(host, 0.0) < 0.15:
                return
            self.last_query[host] = now
            parts = command.split()
            if len(parts) < 3:
                return
            game = parts[1]
            try:
                protocol = int(parts[2], 10)
            except ValueError:
                return
            result = self.directory.list_packet(game, protocol)
            for packet in result:
                self.transport.sendto(packet, address)


class HealthHandler(BaseHTTPRequestHandler):
    directory = None

    def do_GET(self):
        if self.path not in ("/health", "/servers"):
            self.send_error(404)
            return
        self.directory.expire()
        if self.path == "/health":
            payload = {"service": "quake3-native-master", "ok": True,
                       "server_count": len(self.directory.servers),
                       "protocol": "id-tech-3", "openspy_connected": False}
        else:
            payload = {"servers": [vars(item) for item in self.directory.servers.values()]}
        body = json.dumps(payload, indent=2).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        print("http: " + fmt % args)


async def run(args):
    directory = Directory(ttl=args.ttl)
    loop = asyncio.get_running_loop()
    transport, _ = await loop.create_datagram_endpoint(
        lambda: MasterProtocol(directory),
        local_addr=(args.host, args.port))
    HealthHandler.directory = directory
    http = ThreadingHTTPServer((args.http_host, args.http_port), HealthHandler)
    Thread(target=http.serve_forever, daemon=True).start()
    print("Quake III native master listening on UDP %d; health HTTP on %d" %
          (args.port, args.http_port))
    try:
        await asyncio.Event().wait()
    finally:
        transport.close()
        http.shutdown()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=os.environ.get("Q3_MASTER_HOST", "0.0.0.0"))
    parser.add_argument("--port", type=int, default=int(os.environ.get("Q3_MASTER_PORT", "27950")))
    parser.add_argument("--http-host", default=os.environ.get("Q3_MASTER_HTTP_HOST", "127.0.0.1"))
    parser.add_argument("--http-port", type=int, default=int(os.environ.get("Q3_MASTER_HTTP_PORT", "8780")))
    parser.add_argument("--ttl", type=int, default=int(os.environ.get("Q3_MASTER_TTL", str(DEFAULT_TTL))))
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not 1 <= args.http_port <= 65535 or args.ttl < 30:
        parser.error("ports must be 1..65535 and TTL must be at least 30 seconds")
    asyncio.run(run(args))


if __name__ == "__main__":
    main()
