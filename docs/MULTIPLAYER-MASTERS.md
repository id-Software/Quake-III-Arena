# Quake III Arena multiplayer master services

## Native Quake III master protocol

This repository's Quake III multiplayer support uses the id Tech 3 / Quake III master-server protocol. Dedicated servers announce themselves using Quake III heartbeats, and master servers return Quake III server addresses. The standard master port is UDP 27950.

A ready-to-load configuration is provided at `config/quake3-masters.cfg`. Start a public dedicated server with the config loaded, for example:

```sh
ioq3ded +set dedicated 2 +exec quake3-masters.cfg +map q3dm1
```

Use the dedicated-server executable built by this repository in place of `ioq3ded` if it has a different name. The `dedicated 2` setting enables public Internet master-server registration. Ensure the game server's UDP port (normally 27960) is reachable from the Internet.

The configuration lists these Quake III master services:

- `master.quake3arena.com:27950`
- `master.ioquake3.org:27950`
- `master.maverickservers.com:27950`
- `master0.excessiveplus.net:27950`
- `dpmaster.deathmask.net:27950`

Master operators can change these entries or leave unused `sv_master` slots empty. Master-server availability can change; the list is configurable rather than hard-wired into the game.

## OpenSpy compatibility

OpenSpy is a GameSpy-compatible replacement service. Its normal master endpoint, `master.openspy.net:27900`, speaks the GameSpy master protocol. Quake III Arena does not use that protocol: it uses its own id Tech 3 master heartbeat and server-list protocol on UDP 27950.

Consequently, OpenSpy cannot currently be selected as a drop-in Quake III master service merely by changing `sv_master1`. Sending Quake III heartbeats to the OpenSpy GameSpy endpoint would not make Quake III servers appear there. This repository does not claim or enable that unsupported integration.

If OpenSpy adds explicit Quake III / id Tech 3 master-protocol support, or a compatible bridge is developed and tested, its endpoint can be added to this configuration. Until then, the native Quake III masters above are the supported public server-list services. Direct IP connection remains available independently of master-server listings.

## Self-hosted Quake III master bridge

A small native-protocol directory service is included at `openspy-bridge/q3_master.py`.
It accepts Quake III heartbeats and answers native `getservers` requests on UDP
27950. See [the bridge README](../openspy-bridge/README.md) for startup and
client configuration. This is a self-hosted Quake III directory, not an OpenSpy
backend integration.

## Client server browser

The server-side `sv_master1` through `sv_master5` settings control where a dedicated server publishes its heartbeat. They do not automatically rewrite every stock Quake III client's built-in Internet browser master address. Client browser behavior depends on the game executable and UI module; modern ioquake3-derived clients support configurable master lists more broadly than the original 1.32 client.

## References

- [OpenSpy FAQ](https://openspy.org/faq) — describes OpenSpy as a GameSpy-compatible multiplayer platform.
- [ioquake3 server administration guide](https://ioquake3.org/help/sys-admin-guide/) — dedicated server and public listing configuration.
- [Quake III master-server list](https://github.com/dgibbs64/Master-Server-List) — current public Quake III master endpoints.
