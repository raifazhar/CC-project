# Compiler and tools
CXX = clang++
CC = clang
LEX = flex
YACC = bison

# LLVM setup
LLVM_CONFIG = llvm-config
LLVM_FLAGS = $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs --system-libs core orcjit native)

# Directories
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
IR_DIR = $(BUILD_DIR)/ir
DEBUG_DIR = $(BUILD_DIR)/debug

# Files
LEX_FILE = $(SRC_DIR)/ssc.l
YACC_FILE = $(SRC_DIR)/ssc.y
INPUT_FILE = $(SRC_DIR)/input.ssc

LEX_GEN_C = $(BUILD_DIR)/lex.yy.c
YACC_GEN_C = $(BUILD_DIR)/ssc.tab.c
YACC_GEN_H = $(BUILD_DIR)/ssc.tab.h

IR_CPP = $(SRC_DIR)/IR.cpp
AST_CPP = $(SRC_DIR)/AST.cpp
SYM_CPP = $(SRC_DIR)/Symbol_Table.cpp

IR_OBJ = $(OBJ_DIR)/IR.o
AST_OBJ = $(OBJ_DIR)/AST.o
SYM_OBJ = $(OBJ_DIR)/Symbol_Table.o

COMPILER_EXE = $(BIN_DIR)/ssc_compiler
COMPILER_IR = $(BIN_DIR)/ssc_compiler_ir
LLVM_IR = $(IR_DIR)/output.ll
OBJ_OUTPUT = $(OBJ_DIR)/output.o
DEBUG_OUT = $(DEBUG_DIR)/debug_output.txt

# Flags
CXXFLAGS = -std=c++17 -fno-exceptions -funwind-tables $(LLVM_FLAGS) -I.
LINKER = clang++
LINKER_FLAGS = $(LLVM_LIBS)

# Targets
.PHONY: all run clean ir

all: run

run: clean ir
	@llc -filetype=obj $(LLVM_IR) -o $(OBJ_OUTPUT)
	@$(LINKER) $(OBJ_OUTPUT) -o $(COMPILER_IR) $(LINKER_FLAGS)
	@echo "Running compiled executable..."
	@./$(COMPILER_IR)

ir: $(COMPILER_EXE)
	@mkdir -p $(IR_DIR) $(DEBUG_DIR)
	@$(COMPILER_EXE) $(INPUT_FILE) > $(LLVM_IR) 2> $(DEBUG_OUT)
	@test -s $(LLVM_IR) || (echo "Error: $(LLVM_IR) is empty" && exit 1)

$(COMPILER_EXE): $(LEX_GEN_C) $(YACC_GEN_C) $(IR_OBJ) $(AST_OBJ) $(SYM_OBJ)
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) -o $@ $(YACC_GEN_C) $(LEX_GEN_C) $(IR_OBJ) $(AST_OBJ) $(SYM_OBJ) $(LINKER_FLAGS)

$(IR_OBJ): $(IR_CPP)
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(AST_OBJ): $(AST_CPP)
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(SYM_OBJ): $(SYM_CPP)
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(LEX_GEN_C): $(LEX_FILE)
	@mkdir -p $(BUILD_DIR)
	@$(LEX) -o $@ $<

$(YACC_GEN_C): $(YACC_FILE)
	@mkdir -p $(BUILD_DIR)
	@$(YACC) -d -o $@ $<

clean:
	@rm -rf $(BUILD_DIR)

distclean: clean
	rm -f a.out ssc.output
	rm -rf $(BUILD_DIR)

# Help message
help:
	@echo "Available targets:"
	@echo "  make         - Compile the SSC compiler"
	@echo "  make ir      - Generate intermediate LLVM IR ($(IR_FILE))"
	@echo "  make run     - Generate IR, compile it, and run the executable"
	@echo "  make clean   - Remove compiled and intermediate files"
	@echo "  make distclean - Remove all build files and output"
	@echo "  make help    - Display this help message"
