# Quake III ↔ OpenSpy integration work

## Important protocol boundary

Quake III Arena uses the native id Tech 3 master protocol (UDP 27950). OpenSpy's
core QR service accepts GameSpy Query & Reporting (QR/QR2) registration, not
Quake III's native heartbeat. OpenSpy documents QR as a separate service and
requires a game-specific registration identity. A Quake III heartbeat cannot
be forwarded unchanged to OpenSpy.

This directory implements the **Quake III-facing master-directory component**.
It is a working native-protocol directory for Quake III clients and a clean
boundary for a future OpenSpy QR2 adapter. It does not claim that servers are
already visible in OpenSpy's public GameSpy browser.

## Run the native directory

Python 3.8+; no third-party packages:

```sh
python3 openspy-bridge/q3_master.py --host 0.0.0.0 --port 27950
```

The HTTP status endpoint binds to loopback by default:

- `http://127.0.0.1:8780/health`
- `http://127.0.0.1:8780/servers`

Open UDP 27950 on the host firewall. For a public deployment, use a stable
public hostname and configure the Quake III dedicated server to publish to it:

```cfg
set dedicated "2"
set sv_master1 "your-master-host.example:27950"
```

Clients must also be built/configured to query that master. A server-side
`sv_master1` setting alone does not rewrite the stock 1.32 client's browser
master address. Direct IP joins continue to work independently.

The directory records IPv4 heartbeat source addresses, removes servers after
the configured TTL (default 180 seconds), supports the native
`getservers <game> <protocol>` request, and returns the standard
`getserversResponse\\` address list. It caps response datagrams below a
conservative MTU and applies a small per-source query throttle.

## OpenSpy QR2 uplink: blocked pending OpenSpy-side enablement

An actual OpenSpy uplink needs all of the following before it can be enabled:

1. OpenSpy maintainers must approve/enable Quake III as a game on their backend.
2. They must provide the registered GameSpy `gamename` and QR2 secret key (or
   another supported server-registration credential).
3. A Quake III-specific query-key schema must be agreed so OpenSpy's browser
   can query server status, map, player counts, and player names.
4. The QR2 registration/query adapter must be tested against an OpenSpy
   instance, including server verification and listing.

The QR2 protocol expects a server to answer GameSpy QR2 status queries. A
native Quake III server answers id Tech 3 `getinfo` / `getstatus` queries
instead. A complete adapter therefore needs to translate both registration
and status-query traffic; merely sending a QR2 heartbeat would not be enough.

No OpenSpy credentials are embedded in this repository. Do not invent a
`gamename` or secret key, and do not point a Quake III server directly at
`master.openspy.net:27900`.

## Tests

Run from the repository root:

```sh
python3 -m unittest discover -s openspy-bridge -v
```

These tests cover native heartbeat registration, server-list serialization,
expiry, shutdown, and invalid inputs. They do not test a live OpenSpy backend.
