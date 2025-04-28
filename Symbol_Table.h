// SymbolTable.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common_includes.h"
#include "CodegenContext.h"

class SymbolTable
{
    struct SymbolEntry
    {
        llvm::Value *value;
        llvm::Type *type;
        bool isArray;
        int startIndex;
        int endIndex;
    };

public:
    SymbolTable(CodegenContext &ctx); // Constructor accepting CodegenContext reference

    void enterScope();
    void exitScope();
    llvm::Value *lookupSymbol(const std::string &id, llvm::Value *index = nullptr);
    void setSymbol(const std::string &id, llvm::Value *value, llvm::Type *type, bool isArray = false, int startIndex = -1, int endIndex = -1);
    llvm::Value *createNewSymbol(const std::string &id, llvm::Type *type, bool isArray = false, int startIndex = -1, int endIndex = -1);
    Type *getSymbolType(const std::string &id);

private:
    std::stack<std::unordered_map<std::string, SymbolEntry>> SymbolTableStack;
    CodegenContext &codegenContext; // Reference to CodegenContext
};

#endif // SYMBOLTABLE_H
