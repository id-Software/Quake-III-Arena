# Q3ASM modernization

Q3ASM converts LCC assembly text into Quake III VM bytecode (`.qvm`). This implementation is based on the maintained ioquake3 assembler, imported from ioquake/ioq3 `main` (retrieved 2026-11-11). It retains id Software's GPL source notices.

## Compatibility defaults

- Default output is the original Quake III `VM_MAGIC` format used by 1.32 / 1.32c. The serialized header is 32 bytes.
- `-vq3` explicitly selects the same vanilla format.
- `-v2` opts into ioquake3's extended `VM_MAGIC_VER2` format and jump-table-target section. Do not use `-v2` when targeting the original 1.32 engine.
- The assembler's symbol and opcode lookup paths use hash tables; symbol sorting is deferred until after the first pass.

## Build

From this directory:

```sh
make
make test
```

Or:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The tool is standalone and does not require the engine submodule. It builds for Linux, macOS, and Windows with a C99 compiler.

## Tests

`tests/test_q3asm.py` builds the assembler (unless `--binary` is supplied), assembles a minimal `vmMain` fixture, checks the vanilla and extended QVM magic/header offsets, and verifies that invalid assembly does not produce a QVM. These are format smoke tests; full game-module compilation and in-engine execution still need to be run against the repository's pinned LCC and engine.

## Upstream

- Source: https://github.com/ioquake/ioq3/tree/main/code/tools/asm
- Upstream project documents `-vq3` for compatibility with the original Quake III 1.32 engine. This fork makes that format the safe default and keeps the extended format opt-in.
