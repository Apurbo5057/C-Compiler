#!/usr/bin/env bash
#
# One-time setup of the ANTLR 4.13.2 tool jar and the ANTLR C++ runtime.
# Tested on Ubuntu / WSL. Requires sudo.
#
set -euo pipefail

ANTLR_VERSION="4.13.2"
JAR_URL="https://www.antlr.org/download/antlr-${ANTLR_VERSION}-complete.jar"

echo ">> Installing build dependencies"
sudo apt-get update
sudo apt-get install -y default-jdk g++ cmake make git pkg-config uuid-dev

echo ">> Fetching the ANTLR tool jar"
sudo curl -fsSL "$JAR_URL" -o "/usr/local/lib/antlr-${ANTLR_VERSION}-complete.jar"

echo ">> Building the ANTLR C++ runtime from source"
tmp="$(mktemp -d)"
git clone --depth 1 --branch "${ANTLR_VERSION}" https://github.com/antlr/antlr4.git "$tmp/antlr4"
cmake -S "$tmp/antlr4/runtime/Cpp" -B "$tmp/antlr4/runtime/Cpp/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$tmp/antlr4/runtime/Cpp/build" -j "$(nproc)"
sudo cmake --install "$tmp/antlr4/runtime/Cpp/build"
sudo ldconfig
rm -rf "$tmp"

echo ">> Done."
echo "   tool jar : /usr/local/lib/antlr-${ANTLR_VERSION}-complete.jar"
echo "   headers  : /usr/local/include/antlr4-runtime"
echo "   library  : /usr/local/lib/libantlr4-runtime.so"
