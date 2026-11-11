# Multiplayer chat terminology moderation

This branch adds a server-authoritative check in `G_Say` before messages are
logged, echoed, or delivered. It covers global chat, team chat, and private
tells. The existing `/sha256` command submits only the digest to chat, never
the original input, so it cannot expose the original text to other players.

The matcher folds ASCII case and removes punctuation/spacing before matching
a short list of unambiguous prohibited phrases. Rejected text and matched
phrases are not written to the game log. A match drops the sender with the
fixed reason `Chat policy violation`.

## Remaining integration work

The persistent main-menu notice and the live OBS event feed still need client
disconnect-state plumbing and an authenticated event endpoint in the gateway.
The included overlay is a presentation shell only; it is not connected to a
live event source yet. This commit does not claim those two features are
operational. Test the server change against the exact 1.32/1.32c build before
production use, and review false positives before expanding the phrase list.
