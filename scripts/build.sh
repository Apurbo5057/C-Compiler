#!/usr/bin/env bash
#
# Builds the C-to-8086 compiler.
#
# Override any of these if your ANTLR install lives elsewhere:
#   ANTLR_JAR      path to antlr-4.13.2-complete.jar
#   ANTLR_INCLUDE  directory containing antlr4-runtime.h
#   ANTLR_LIB      directory containing libantlr4-runtime.so
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src"
BUILD="$ROOT/build"
GEN="$BUILD/generated"
OBJ="$BUILD/obj"

ANTLR_JAR="${ANTLR_JAR:-/usr/local/lib/antlr-4.13.2-complete.jar}"
ANTLR_INCLUDE="${ANTLR_INCLUDE:-/usr/local/include/antlr4-runtime}"
ANTLR_LIB="${ANTLR_LIB:-/usr/local/lib}"

if [[ ! -f "$ANTLR_JAR" ]]; then
    echo "error: ANTLR tool jar not found at $ANTLR_JAR" >&2
    echo "       run scripts/setup-antlr.sh, or set ANTLR_JAR=/path/to/antlr-4.13.2-complete.jar" >&2
    exit 1
fi

if [[ ! -d "$ANTLR_INCLUDE" ]]; then
    echo "error: ANTLR C++ runtime headers not found at $ANTLR_INCLUDE" >&2
    echo "       run scripts/setup-antlr.sh, or set ANTLR_INCLUDE=/path/to/antlr4-runtime" >&2
    exit 1
fi

mkdir -p "$GEN" "$OBJ"

# --- 1. Generate the lexer and the code-generating parser ------------------
# The lexer must be generated first: C8086CodeGen.g4 imports its token vocabulary.
echo ">> Generating parser from grammars"
(
    cd "$SRC"
    java -Xmx500M -cp "$ANTLR_JAR" org.antlr.v4.Tool \
        -Dlanguage=Cpp -o "$GEN" -lib "$GEN" C8086Lexer.g4
    java -Xmx500M -cp "$ANTLR_JAR" org.antlr.v4.Tool \
        -Dlanguage=Cpp -o "$GEN" -lib "$GEN" C8086CodeGen.g4
)

# --- 2. Compile -----------------------------------------------------------
CXXFLAGS=(-std=c++17 -w -I"$ANTLR_INCLUDE" -I"$GEN" -I"$SRC")

echo ">> Compiling"
for unit in "$GEN/C8086Lexer.cpp" "$GEN/C8086CodeGen.cpp" "$SRC/Main.cpp" "$SRC/CodeGenerator.cpp"; do
    obj="$OBJ/$(basename "${unit%.cpp}").o"
    echo "   CXX $(basename "$unit")"
    g++ "${CXXFLAGS[@]}" -c "$unit" -o "$obj"
done

# --- 3. Link --------------------------------------------------------------
echo ">> Linking"
g++ -std=c++17 -w "$OBJ"/*.o -L"$ANTLR_LIB" -lantlr4-runtime -pthread -o "$BUILD/c8086cc"

echo ">> Built $BUILD/c8086cc"
