#!/usr/bin/env python3
"""Smoke tests for Q3ASM's vanilla and extended QVM headers."""
import argparse
import pathlib
import shutil
import struct
import subprocess
import tempfile


def run_tests(binary):
    binary = pathlib.Path(binary).resolve()
    with tempfile.TemporaryDirectory(prefix="q3asm-test-") as temp:
        root = pathlib.Path(temp)
        asm = root / "minimal.asm"
        asm.write_text("proc vmMain 0 0\nCNSTI4 0\nendproc vmMain 0 0\n", encoding="ascii")

        vanilla_base = root / "vanilla"
        subprocess.run([str(binary), "-o", str(vanilla_base), str(asm)],
                       check=True, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        vanilla = pathlib.Path(str(vanilla_base) + ".qvm").read_bytes()
        assert len(vanilla) >= 32, "vanilla QVM is shorter than its 32-byte header"
        fields = struct.unpack("<8I", vanilla[:32])
        assert fields[0] == 0x12721444, "default output must use VM_MAGIC"
        assert fields[2] == 32, "vanilla QVM codeOffset must be 32"
        assert fields[3] > 0, "vanilla QVM must contain code"

        extended_base = root / "extended"
        subprocess.run([str(binary), "-v2", "-o", str(extended_base), str(asm)],
                       check=True, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        extended = pathlib.Path(str(extended_base) + ".qvm").read_bytes()
        assert len(extended) >= 36, "extended QVM is shorter than its 36-byte header"
        extended_fields = struct.unpack("<9I", extended[:36])
        assert extended_fields[0] == 0x12721445, "-v2 must use VM_MAGIC_VER2"
        assert extended_fields[2] == 36, "extended QVM codeOffset must be 36"

        bad_asm = root / "bad.asm"
        bad_asm.write_text("not_a_real_opcode\n", encoding="ascii")
        bad_base = root / "bad"
        result = subprocess.run([str(binary), "-o", str(bad_base), str(bad_asm)],
                                cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode != 0, "invalid assembly must return a failure code"
        assert not pathlib.Path(str(bad_base) + ".qvm").exists(), "invalid assembly must not emit a QVM"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", help="existing q3asm executable")
    args = parser.parse_args()
    if args.binary:
        run_tests(args.binary)
        return
    compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
    if not compiler:
        raise SystemExit("No C compiler found (cc, clang, or gcc required)")
    source = pathlib.Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="q3asm-build-") as temp:
        binary = pathlib.Path(temp) / ("q3asm.exe" if shutil.which("cl") else "q3asm")
        subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Wextra",
                        "-I", str(source), str(source / "q3asm.c"),
                        str(source / "cmdlib.c"), "-o", str(binary)],
                       check=True)
        run_tests(binary)


if __name__ == "__main__":
    main()
