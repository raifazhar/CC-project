// SymbolTable.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common_includes.h"
#include "CodegenContext.h"

class SymbolTable
{
public:
    SymbolTable(CodegenContext &ctx); // Constructor accepting CodegenContext reference

    void enterScope();
    void exitScope();
    llvm::Value *lookupSymbol(const std::string &id);
    void setSymbol(const std::string &id, llvm::Value *value, llvm::Type *type);
    llvm::Value *createNewSymbol(const std::string &id, llvm::Type *type);

private:
    std::stack<std::map<std::string, std::pair<llvm::Value *, llvm::Type *>>> SymbolTableStack;
    CodegenContext &codegenContext; // Reference to CodegenContext
};

#endif // SYMBOLTABLE_H
