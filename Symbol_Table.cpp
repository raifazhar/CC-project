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

void SymbolTable::declareSymbol(const std::string &id, llvm::Type *type, bool isArray, int startIndex, int endIndex)
{
    // Set the symbol in the current scope with its value and type
    if (!SymbolTableStack.empty())
    {
        // Store the symbol as a SymbolEntry with Value and Type
        if (isArray)
        {
            llvm::Type *arraytype = llvm::ArrayType::get(type, endIndex - startIndex + 1);
            SymbolTableStack.top()[id] = {nullptr, arraytype, isArray, startIndex, endIndex};
            return;
        }
        SymbolTableStack.top()[id] = {nullptr, type, isArray, startIndex, endIndex};
    }
}

AllocaInst *SymbolTable::allocatteSymbol(const std::string &id)
{
    // Search for the symbol in all active scopes (actual stack, not copy)
    std::stack<std::unordered_map<std::string, SymbolEntry>> temp;

    while (!SymbolTableStack.empty())
    {
        auto &scope = SymbolTableStack.top();
        auto it = scope.find(id);
        if (it != scope.end())
        {

            // Only allocate if not already done
            if (!it->second.value)
            {
                AllocaInst *alloca = builder.CreateAlloca(it->second.type, nullptr, id);
                it->second.value = alloca;
                return alloca;
            }
        }

        // Temporarily move scope out to traverse
        temp.push(std::move(scope));
        SymbolTableStack.pop();
    }

    // Restore stack after traversal
    while (!temp.empty())
    {
        SymbolTableStack.push(std::move(temp.top()));
        temp.pop();
    }

    return nullptr;
}

bool SymbolTable::checkDeclaration(const std::string &id)
{
    auto scopes = SymbolTableStack;
    while (!scopes.empty())
    {
        if (scopes.top().count(id) > 0)
            return true;
        scopes.pop();
    }
    return false;
}
