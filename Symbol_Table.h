// SymbolTable.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common_includes.h"
// #include "CodegenContext.h"

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
    SymbolTable() = default;

    void enterScope();
    void exitScope();
    llvm::Value *lookupSymbol(const std::string &id, llvm::Value *index = nullptr);
    void declareSymbol(const std::string &id, llvm::Type *type, bool isArray = false, int startIndex = -1, int endIndex = -1);
    AllocaInst *allocatteSymbol(const std::string &id);
    Type *getSymbolType(const std::string &id);
    bool checkDeclaration(const std::string &id);

private:
    std::stack<std::unordered_map<std::string, SymbolEntry>> SymbolTableStack;
};

#endif // SYMBOLTABLE_H
