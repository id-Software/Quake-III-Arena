# Compiler and dependency modernization

## QVM compiler: LCC 4.2

The legacy top-level `lcc/` tree is LCC 4.1 (2001). The maintained Q3-specific
LCC 4.2 fork is tracked as the `qvmtools/q3lcc` submodule at:

- Upstream: https://github.com/ec-/q3lcc
- Pinned commit: `814e3808346aba20f994fa83fca4f9d37124c67c`
- Version: LCC 4.2
- Tool outputs: `q3lcc`, `q3cpp`, and `q3rcc`

This revision includes a fix for building with GCC 14 and newer. The old
`lcc/` directory is retained as a historical fallback; it is not overwritten.
The upstream LCC license is in the submodule's `CPYRIGHT` file and must
accompany any redistribution of its source or binaries. Review that license
before bundling LCC into a commercial distribution.

Initialize and build on Linux or macOS:

```sh
git submodule update --init --recursive -- qvmtools/q3lcc
sh ./scripts/build-qvm-tools.sh
```

The output is kept in the submodule build directory. The existing QVM assembler
is not replaced. The official Quake III 1.32/1.32c bytecode target remains the
compatibility target; the toolchain change is not considered validated until
the QVMs are rebuilt and compared/run against known 1.32/1.32c test cases.

## Third-party libraries

The active ReForged engine dependency pins are managed separately from the
historical Radiant/tool sources:

- `stb`: pinned revision `2b2e64bc4778df6a06fbea3a5dbe61f7a76ecf19`
- `TinyJPEG`: pinned revision `e978b746714abad76e0f00264d2a154b52de8fc1`

Use `scripts/update-dependencies.ps1` on Windows to initialize and verify
these exact revisions. The engine submodule owns its own dependency graph;
do not replace these with floating branch heads.

The repository also contains legacy JPEG-6 code under `code/jpeg-6` and
`libs/jpeg6` for older tools. Those copies are deliberately not deleted by
this change: the available source tree does not establish that every consumer
has migrated to the active engine's TinyJPEG path. Removing or swapping them
without a consumer/build audit could break Radiant or archive utilities.

## Validation status

This change pins and provides a build path for LCC 4.2. It does not claim
that the QVM tools or full game have compiled successfully in this session.
Before declaring the migration complete, run the compiler build, regenerate
the baseq3 and missionpack QVMs, compare representative bytecode, and test
single-player and 1.32/1.32c multiplayer loading.
