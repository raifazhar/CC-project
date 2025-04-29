#ifndef IR_H
#define IR_H

#include "common_includes.h" // Centralized includes
#include "Symbol_Table.h"
#include <stack>
#include <queue>

// External declarations for indentation tracking
extern std::stack<int> indent_stack;
extern std::queue<int> dedent_buffer;
extern int current_indent;
extern bool start_of_line;
extern SymbolTable *globalSymbolTable;

Value *performBinaryOperation(Value *lhs, Value *rhs, const std::string &op);
Value *performComparison(Value *lhs, Value *rhs, const std::string &op);
void yyerror(const char *err);
void initLLVM();
void printLLVMIR();
void addReturnInstr();

#endif // HEADER_FILE_NAME_H
