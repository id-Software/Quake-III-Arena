# SHA-256 build checksums

The RTX and legacy OpenGL build scripts generate a SHA256SUMS.txt manifest
after each successful build. The manifest records SHA-256 digests for files
inside that architecture-specific build directory, using relative paths. The
manifest itself is excluded.

## Windows

Verify a completed RTX build:

```powershell
.\scripts\sha256.ps1 -Mode Verify -Path .\build-rtx-x64
```

Verify a legacy OpenGL build:

```powershell
.\scripts\sha256.ps1 -Mode Verify -Path .\build-opengl-x86
```

Generate or refresh a manifest manually:

```powershell
.\scripts\sha256.ps1 -Mode Generate -Path .\build-rtx-x64
```

Verification does not regenerate the manifest. It reports missing or modified
files and exits with an error if any mismatch or malformed entry is found.

## Linux

The Linux legacy OpenGL build helper creates a GNU sha256sum-compatible
manifest. Verify it with:

```sh
cd build-opengl-linux
sha256sum --check SHA256SUMS.txt
```

SHA-256 detects changes and corruption but does not authenticate who produced
the files. Use a trusted detached signature or signed release for publisher
authentication.

## In-game multiplayer chat

The game-side command `/sha256 <message>` calculates the SHA-256 digest of
the supplied text and posts the 64-character hexadecimal result to global
multiplayer chat. For example:

```text
/sha256 hello world
```

The command hashes the exact argument text assembled by the game command
parser. It is a convenience for comparing message text; it does not sign,
authenticate, or otherwise prove who sent a chat message. It is implemented
in the legacy game module at `code/game/g_cmds.c`.
