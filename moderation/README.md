# Multiplayer chat terminology moderation

## Server enforcement

The game server's `G_Say` hook checks chat before logging or delivery. It
covers global chat, team chat, and private tells. The filter uses a narrow
phrase list and a fixed kick reason; rejected message text and matched terms
are not logged or displayed.

## Main-menu notice

When the client receives the fixed disconnect reason `Chat policy violation`,
it raises the existing disconnect error flow with a dedicated message. The UI
selects `moderation_popmenu` and displays the Chris Hansen parody headline
plus the policy explanation. This is fictional copy, not a statement or
message from Chris Hansen.

## Live OBS overlay

1. Run `agent-control/server.py` on the same machine as the game server and
   OBS. Keep the gateway bound to loopback (the default).
2. Configure `AGENT_API_KEY` with a random value of at least 32 characters.
3. Add `moderation/obs-overlay.html` to OBS as a Browser Source (local file).
4. In the overlay setup panel, enter the gateway URL and the same bearer key.
5. Trigger a test kick to verify the event appears in the stream.

For a matching kick, the native server sends only the fixed UDP marker
`Q3MOD1` to loopback UDP port 27991. The gateway stores an opaque event ID,
timestamp, and fixed reason code. The OBS overlay polls the authenticated
`/api/moderation/latest` endpoint and displays fixed copy. No player name,
chat text, or matched term is sent to the overlay.

**Deployment boundary:** the game server, gateway, and OBS must share a host
for this loopback event path. Remote or containerized deployments need an
explicitly secured event relay. The gateway listener is loopback-only.
