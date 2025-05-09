// SymbolTable.cpp
#include "Symbol_Table.h"
#include "IR.h"
#include <llvm/IR/IRBuilder.h>


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

llvm::Value *SymbolTable::lookupSymbol(const std::string &id, llvm::Value *index)
{
    auto scopes = SymbolTableStack;
    while (!scopes.empty())
    {
        auto &scope = scopes.top();
        auto it = scope.find(id);
        if (it != scope.end())
        {
            if (index == nullptr)
            {
                return it->second.value;
            }

            if (it->second.isArray)
            {
                llvm::Value *zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);

                // Step 1: Adjust index if array startIndex != 0
                if (it->second.startIndex != 0)
                {
                    llvm::Value *startIdx = llvm::ConstantInt::get(builder.getInt32Ty(), it->second.startIndex);
                    index = builder.CreateSub(index, startIdx, id + "_adjusted_index");
                }

                // Step 2: Calculate GEP
                Value *ptr = builder.CreateGEP(
                    it->second.type, // array type [size x element]
                    it->second.value,
                    {zero, index});

                return ptr;
            }
            else
            {
                return nullptr;
            }
        }
        scopes.pop();
    }
    return nullptr; // Not found
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

void SymbolTable::setSymbol(const std::string &id, llvm::Value *value, llvm::Type *type, bool isArray, int startIndex, int endIndex)
{
    // Set the symbol in the current scope with its value and type
    if (!SymbolTableStack.empty())
    {
        // Store the symbol as a SymbolEntry with Value and Type
        SymbolTableStack.top()[id] = {value, type, isArray, startIndex, endIndex};
    }
}

llvm::Value *SymbolTable::createNewSymbol(const std::string &id, llvm::Type *type, bool isArray, int startIndex, int endIndex)
{
    if (!SymbolTableStack.empty() && SymbolTableStack.top().find(id) != SymbolTableStack.top().end())
    {
        return nullptr;
    }

    llvm::Type *allocType = type;
    if (isArray)
    {
        allocType = llvm::ArrayType::get(type, endIndex - startIndex + 1);
    }

    llvm::Value *alloca = builder.CreateAlloca(allocType, nullptr, id);

    SymbolTableStack.top()[id] = {alloca, allocType, isArray, startIndex, endIndex};

    return alloca;
}
