// SymbolTable.cpp
#include "Symbol_Table.h"
#include "IR.h"
#include <llvm/IR/IRBuilder.h>

SymbolTable::SymbolTable(CodegenContext &ctx) : codegenContext(ctx) {}

void SymbolTable::enterScope()
{
    // Push a new scope to the stack
    SymbolTableStack.push({});
}

void SymbolTable::exitScope()
{
    // Pop the current scope from the stack
    if (!SymbolTableStack.empty())
    {
        SymbolTableStack.pop();
    }
}

llvm::Value *SymbolTable::lookupSymbol(const std::string &id)
{
    // Search through the symbol table scopes to find the variable
    auto scopes = SymbolTableStack;
    while (!scopes.empty())
    {
        auto &scope = scopes.top();
        auto it = scope.find(id);
        if (it != scope.end())
            return it->second.value; // Access the 'value' member of SymbolEntry
        scopes.pop();
    }
    return nullptr; // Return nullptr if the symbol is not found
}

llvm::Type *SymbolTable::getSymbolType(const std::string &id)
{
    auto scopes = SymbolTableStack;
    while (!scopes.empty())
    {
        auto &scope = scopes.top();
        auto it = scope.find(id);
        if (it != scope.end())
            return it->second.type; // Access the 'type' member of SymbolEntry
        scopes.pop();
    }
    return nullptr; // Return nullptr if the symbol is not found
}

void SymbolTable::setSymbol(const std::string &id, llvm::Value *value, llvm::Type *type)
{
    // Add a new symbol to the current scope with its value and type
    if (!SymbolTableStack.empty())
    {
        // Store the symbol as a SymbolEntry with Value and Type
        SymbolTableStack.top()[id] = {value, type};
    }
}

llvm::Value *SymbolTable::createNewSymbol(const std::string &id, llvm::Type *type)
{
    // Create an alloca instruction to allocate memory for the variable
    llvm::Value *alloca = builder.CreateAlloca(type, nullptr, id);

    // Store the variable in the current scope with its value (alloca) and type
    SymbolTableStack.top()[id] = {alloca, type};

    return alloca; // Return the alloca value (memory location for the variable)
}
