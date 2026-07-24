#!/usr/bin/env bash
#
# Removes every generated and compiled artifact.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

rm -rf "$ROOT/build" "$ROOT/output"
echo ">> Cleaned build/ and output/"
