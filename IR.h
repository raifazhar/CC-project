#ifndef IR_H
#define IR_H

#include "common_includes.h" // Centralized includes
#include "Symbol_Table.h"

SymbolTable symtab;
Value *getFromSymbolTable(const std::string &id);
void setDouble(const std::string &id, Value *value);
void printString(const std::string &str);
void printDouble(Value *value);
Value *performBinaryOperation(Value *lhs, Value *rhs, char op);
Value *performComparison(Value *lhs, Value *rhs, int op);
void printStatements(const std::vector<Value *> *stmtList);
void yyerror(const char *err);
void initLLVM();
void printLLVMIR();
void addReturnInstr();
Value *createDoubleConstant(double val);
Value *loadIfPointer(Value *operand);

#endif // HEADER_FILE_NAME_H
