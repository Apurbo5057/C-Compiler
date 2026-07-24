#!/bin/bash

ANTLR_JAR="/usr/local/lib/antlr-4.13.2-complete.jar"

# Step 1: Generate Lexer and Parser in current directory
java -Xmx500M -cp "$ANTLR_JAR" org.antlr.v4.Tool -Dlanguage=Cpp C8086Lexer.g4
java -Xmx500M -cp "$ANTLR_JAR" org.antlr.v4.Tool -Dlanguage=Cpp C8086Parser.g4

# Step 2: Compile (no OUT_DIR needed)
g++ -std=c++17 -w -I/usr/local/include/antlr4-runtime -c C8086Lexer.cpp C8086Parser.cpp Ctester.cpp

# Step 3: Link and build
g++ -std=c++17 -w C8086Lexer.o C8086Parser.o Ctester.o -L/usr/local/lib/ -lantlr4-runtime -o Ctester.out -pthread

# Step 4: Run the parser
LD_LIBRARY_PATH=/usr/local/lib ./Ctester.out "$1"