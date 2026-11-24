# Quake III Arena — 64-bit Linux / SDL2 port

A fork of [id-Software/Quake-III-Arena](https://github.com/id-Software/Quake-III-Arena)
(the 2005 GPL source release, version 1.32b) that builds and runs natively on a
current 64-bit Linux system. It was developed and tested on Fedora (x86_64,
gcc 16, Wayland, Mesa).

The original release only builds as 32-bit i386 and depends on things that no
longer exist on modern systems: `/dev/dsp` (OSS), XFree86 DGA/VidMode, nasm
assembly, and a JIT that assumes 32-bit pointers. This fork replaces those
with an SDL2 platform layer and fixes the 64-bit bugs. Gameplay code is
unchanged.

**No game data is included.** You need your own copy of Quake III Arena
(`pak0.pk3` and friends). The free demo works for the maps it contains.

## Status

Tested: client starts, menus, single player and bot matches on retail maps,
Team Arena (`fs_game missionpack`) loads, keyboard and mouse input, windowed
and fullscreen, dedicated server.

Not tested or not available:

- Sound opens a device through SDL, but has not been verified by ear.
- Hardware gamma is not available on Wayland (the game falls back to
  software overbright).
- No JIT: game logic runs in the QVM interpreter (fine for this game).
- No SMP renderer, no 3Dfx, no FreeType font generation (`ft2/` is not part of
  the release; fonts are prebuilt in the paks).
- Internet play and the original master server are not tested.

## Platforms

| Platform | Status |
|---|---|
| Linux x86_64 | built and played (gcc and clang) |
| Linux arm64 | should work (same code path); built by CI only |
| macOS (Apple Silicon / Intel) | prepared, **not yet run**: built by CI only |
| FreeBSD, others with SDL2 | untested |
| Windows | not supported yet |

The engine is C-only (no JIT, no assembly) and 64-bit clean, so there is
nothing CPU-specific. Everything platform-dependent lives in the SDL2 layer
(`code/unix/sdl_glimp.c`, `sdl_snd.c`).

## Building

Requirements: a C compiler (gcc or clang), make, SDL2 development files
(`SDL2-devel` on Fedora, `libsdl2-dev` on Debian/Ubuntu, `brew install sdl2`
on macOS), and python3 for the QVM script.

```sh
cd code/unix
make -f Makefile.sdl -j8                 # client + dedicated server
make -f Makefile.sdl CC=clang -j8        # another compiler
make -f Makefile.sdl CONFIG=debug -j8    # debug build
```

Output goes to `build/<os>-<arch>-<config>/` (`quake3` and `q3ded`). Delete
that directory to clean. Header dependencies are tracked, so rebuilds are
correct.

`Makefile.sdl` is the portable build. The original `Makefile` (i386 only,
X11/OSS) is kept as it was, apart from an x86_64 branch.

The GitHub Actions workflow (`.github/workflows/build.yml`) builds on Linux
x86_64, Linux arm64 and macOS arm64, and runs the QVM build.

## Game data

Put your paks in `run/` at the repository root:

```
run/baseq3/pak0.pk3 ... pak8.pk3
run/missionpack/pak0.pk3        (Team Arena, optional)
```

The `run/` directory is git-ignored.

### Using the demo instead of the retail game

The original Q3A demo's game modules are for an older engine API, so they must
be rebuilt from this source. After placing the demo's `pak0.pk3` in
`run/baseq3/`:

```sh
cd code/unix
./build_qvms.sh      # builds lcc + q3asm, compiles game/cgame/ui to QVMs
                     # and writes run/baseq3/zz_vm_from_source.pk3
```

With the retail paks you do not need this; the QVMs in the retail `pak8.pk3`
work as they are.

## Running

```sh
cd code/unix
./run_q3.sh +set r_fullscreen 0 +devmap q3dm1
./run_q3.sh +set fs_game missionpack          # Team Arena
```

The home directory (config, saves, screenshots) is `~/.q3a` on Linux and
`~/Library/Application Support/Quake3` on macOS.

`run_q3.sh` points the game at `run/` and sets `sv_pure 0`. Any extra
arguments are passed to the game. Console commands typed in-game need a
leading `/`, otherwise they are sent as chat.

## What changed from the original release

- **Portable build** `code/unix/Makefile.sdl`: C-only, interpreted QVMs,
  `-std=gnu89`, OS and architecture detection (Linux, macOS, x86_64, arm64).
  The legacy Makefile also got an x86_64 branch.
- **Portability groundwork**: the renderer takes its GL header from SDL (no
  GLX/X11 needed), macOS-specific context setup, home directory and
  `Sys_ProcessorCount`, arm64 CPU strings, and C versions of the float helpers
  on every non-i386 target.
- **SDL2 platform layer**: `code/unix/sdl_glimp.c` (window, GL context, input,
  gamma) and `code/unix/sdl_snd.c` (audio). `linux_qgl.c` loads GL functions
  through SDL when built with `-DUSE_SDL`. The old X11/GLX/OSS files are kept
  for the i386 build.
- **64-bit fixes**: heap pointer truncation in `Hunk_Alloc`; `VM_Call`
  collecting variadic arguments from a 32-bit stack layout; `qsort` element
  size for the pak list; renderer draw-surface swap assuming an 8-byte struct;
  MD4 using a 64-bit `unsigned long` (this broke BSP/AAS checksums, so bots
  would not load); 16-byte alignment for zone and botlib allocations
  (`long double` tokens crash at `-O2` otherwise).
- **Demo restrictions are opt-in** (`+set fs_restrict 1`). A source build has
  no retail product id, so the original code always dropped into demo mode.
- **`code/unix/build_qvms.sh`** rebuilds the game, cgame and ui QVMs with the
  tree's own `lcc` and `q3asm`.

See `git log` for the individual commits.

## License

Quake III Arena source code is released under the GNU General Public License
v2; see `COPYING.txt`. The game data (paks, maps, textures, sounds) is **not**
covered by that license and is not included here. The original readme is
`README.txt`.
