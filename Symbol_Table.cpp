// SymbolTable.cpp
#include "Symbol_Table.h"
#include <llvm/IR/IRBuilder.h>

SymbolTable::SymbolTable(CodegenContext &ctx) : codegenContext(ctx) {}

void SymbolTable::enterScope()
{
    SymbolTableStack.push({});
}

void SymbolTable::exitScope()
{
    if (!SymbolTableStack.empty())
    {
        SymbolTableStack.pop();
    }
}

Value *SymbolTable::lookupSymbol(const std::string &id)
{
    auto scopes = SymbolTableStack;
    while (!scopes.empty())
    {
        auto &scope = scopes.top();
        auto it = scope.find(id);
        if (it != scope.end())
            return it->second.first;
        scopes.pop();
    }
    return nullptr;
}

void SymbolTable::setSymbol(const std::string &id, llvm::Value *value, llvm::Type *type)
{
    if (!SymbolTableStack.empty())
    {
        SymbolTableStack.top()[id] = {value, type};
    }
}

Value *SymbolTable::createNewSymbol(const std::string &id, llvm::Type *type)
{
    Value *alloca = builder.CreateAlloca(type, nullptr, id);
    SymbolTableStack.top()[id] = {alloca, type};
    return alloca;
}
