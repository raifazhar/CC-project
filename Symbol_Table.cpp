#include "Symbol_Table.h"
#include "IR.h"
#include <llvm/IR/IRBuilder.h>

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

llvm::Value *SymbolTable::lookupSymbol(const std::string &id, llvm::Value *index)
{
    auto scopes = SymbolTableStack; // now works because shared_ptr is copyable
    while (!scopes.empty())
    {
        auto &scope = scopes.top();
        auto it = scope.find(id);
        if (it != scope.end())
        {
            auto sym = it->second;

            if (index == nullptr)
            {
                return sym->getValue();
            }

            if (auto *arraySym = dynamic_cast<ArraySymbol *>(sym.get()))
            {
                llvm::Value *arrayPtr = arraySym->getValue();         // Alloca'd [N x i32]*
                llvm::Type *elementType = arraySym->getElementType(); // i32

                llvm::Value *zero = llvm::ConstantInt::get(builder.getInt32Ty(), 0);

                if (arraySym->getStartIndex() != 0)
                {
                    llvm::Value *startIdx = llvm::ConstantInt::get(builder.getInt32Ty(), arraySym->getStartIndex());
                    index = builder.CreateSub(index, startIdx, id + "_adjusted_index");
                }

                llvm::Value *gep = builder.CreateGEP(
                    arraySym->getType(), // This is [N x i32]
                    arrayPtr,
                    {zero, index}, // Accessing index-th element
                    id + "_elem_ptr");

                return gep;
            }
            else
            {
                return nullptr;
            }
        }
        scopes.pop();
    }
    return nullptr;
}

llvm::Type *SymbolTable::getSymbolType(const std::string &id)
{
    auto scopes = SymbolTableStack;
    while (!scopes.empty())
    {
        auto &scope = scopes.top();
        auto it = scope.find(id);
        if (it != scope.end())
        {
            return it->second->getType();
        }
        scopes.pop();
    }
    return nullptr;
}

void SymbolTable::declareSymbol(const std::string &id, llvm::Type *type, bool isArray, int startIndex, int endIndex)
{
    if (SymbolTableStack.empty())
        return;

    if (isArray)
    {
        int size = endIndex - startIndex + 1;
        llvm::Type *arrayType = llvm::ArrayType::get(type, size); // [N x i32]
        SymbolTableStack.top()[id] = std::make_shared<ArraySymbol>(type, nullptr, startIndex, endIndex);
    }
    else
    {
        SymbolTableStack.top()[id] = std::make_shared<VariableSymbol>(type, nullptr);
    }
}

AllocaInst *SymbolTable::allocateSymbol(const std::string &id)
{
    std::stack<std::unordered_map<std::string, std::shared_ptr<Symbol>>> temp;

    while (!SymbolTableStack.empty())
    {
        auto &scope = SymbolTableStack.top();
        auto it = scope.find(id);
        if (it != scope.end())
        {
            auto sym = it->second;
            if (!sym->getValue())
            {
                AllocaInst *alloca = builder.CreateAlloca(sym->getType(), nullptr, id);
                sym->setValue(alloca);
                return alloca;
            }
            else
            {
                return llvm::cast<AllocaInst>(sym->getValue());
            }
        }
        temp.push(std::move(scope));
        SymbolTableStack.pop();
    }

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
        if (scopes.top().count(id))
            return true;
        scopes.pop();
    }
    return false;
}
