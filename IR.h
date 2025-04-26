#ifndef IR_H
#define IR_H

#include "common_includes.h" // Centralized includes
#include "Symbol_Table.h"
#include "CodegenContext.h"
#include <stack>
#include <queue>

// External declarations for indentation tracking
extern std::stack<int> indent_stack;
extern std::queue<int> dedent_buffer;
extern int current_indent;
extern bool start_of_line;

extern SymbolTable *globalSymbolTable;
void initSymbolTable(CodegenContext &ctx);
SymbolTable *getSymbolTable();
Value *getFromSymbolTable(const std::string &id);
void setDouble(const std::string &id, Value *value);
void printString(const std::string &str);
void printDouble(Value *value);
Value *performBinaryOperation(Value *lhs, Value *rhs, const std::string &op);
Value *performComparison(Value *lhs, Value *rhs, const std::string &op);
void printStatements(const std::vector<Value *> *stmtList);
void yyerror(const char *err);
void initLLVM();
void printLLVMIR();
void addReturnInstr();
Value *createDoubleConstant(double val);
Value *loadIfPointer(Value *operand);

#endif // HEADER_FILE_NAME_H
