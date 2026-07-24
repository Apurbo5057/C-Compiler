# C-to-8086 compiler
#
# Override these if your ANTLR install lives elsewhere:
#   make ANTLR_JAR=... ANTLR_INCLUDE=... ANTLR_LIB=...

ANTLR_JAR     ?= /usr/local/lib/antlr-4.13.2-complete.jar
ANTLR_INCLUDE ?= /usr/local/include/antlr4-runtime
ANTLR_LIB     ?= /usr/local/lib

SRC   := src
BUILD := build
GEN   := $(BUILD)/generated
OBJ   := $(BUILD)/obj
BIN   := $(BUILD)/c8086cc

CXXFLAGS := -std=c++17 -w -I$(ANTLR_INCLUDE) -I$(GEN) -I$(SRC)
LDFLAGS  := -L$(ANTLR_LIB) -lantlr4-runtime -pthread

GENERATED := $(GEN)/C8086Lexer.cpp $(GEN)/C8086CodeGen.cpp
OBJECTS   := $(OBJ)/C8086Lexer.o $(OBJ)/C8086CodeGen.o \
             $(OBJ)/Main.o $(OBJ)/CodeGenerator.o

.PHONY: all clean run
all: $(BIN)

# The lexer is generated first; the parser grammar imports its token vocabulary.
$(GEN)/C8086Lexer.cpp: $(SRC)/C8086Lexer.g4
	@mkdir -p $(GEN)
	cd $(SRC) && java -Xmx500M -cp $(ANTLR_JAR) org.antlr.v4.Tool \
	    -Dlanguage=Cpp -o $(abspath $(GEN)) -lib $(abspath $(GEN)) C8086Lexer.g4

$(GEN)/C8086CodeGen.cpp: $(SRC)/C8086CodeGen.g4 $(GEN)/C8086Lexer.cpp
	cd $(SRC) && java -Xmx500M -cp $(ANTLR_JAR) org.antlr.v4.Tool \
	    -Dlanguage=Cpp -o $(abspath $(GEN)) -lib $(abspath $(GEN)) C8086CodeGen.g4

$(OBJ)/%.o: $(GEN)/%.cpp
	@mkdir -p $(OBJ)
	g++ $(CXXFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.cpp $(GENERATED)
	@mkdir -p $(OBJ)
	g++ $(CXXFLAGS) -c $< -o $@

$(BIN): $(OBJECTS)
	g++ -std=c++17 -w $(OBJECTS) $(LDFLAGS) -o $@

# make run FILE=tests/test1_i.c
run: $(BIN)
	@mkdir -p output
	LD_LIBRARY_PATH=$(ANTLR_LIB) $(BIN) $(FILE)

clean:
	rm -rf $(BUILD) output
