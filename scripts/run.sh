#!/usr/bin/env bash
#
# Compiles a single C source file to 8086 assembly.
#
#   ./scripts/run.sh tests/test1_i.c
#
# All artifacts are written to ./output/ relative to the current directory.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/build/c8086cc"

if [[ $# -lt 1 ]]; then
    echo "usage: $0 <source.c>" >&2
    exit 1
fi

if [[ ! -x "$BIN" ]]; then
    echo "error: $BIN not found -- run scripts/build.sh first" >&2
    exit 1
fi

mkdir -p output
LD_LIBRARY_PATH="${ANTLR_LIB:-/usr/local/lib}:${LD_LIBRARY_PATH:-}" "$BIN" "$1"
