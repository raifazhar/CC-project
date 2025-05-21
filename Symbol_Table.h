// SymbolTable.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common_includes.h"
#include <stack>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

// Base class for all symbol types
class Symbol
{
public:
    enum class Kind
    {
        Variable,
        Array,
        Function,
        Struct,
        Class
    };

    explicit Symbol(Kind k) : kind(k) {}
    virtual ~Symbol() = default;

    Kind kind;

    virtual llvm::Type *getType() const = 0;
    virtual llvm::Value *getValue() const = 0;
    virtual void setValue(llvm::Value *val) = 0;
};

// ───────────────────────────────────────────
// Variable Symbol
class VariableSymbol : public Symbol
{
    llvm::Value *value;
    llvm::Type *type;

public:
    VariableSymbol(llvm::Type *t, llvm::Value *v = nullptr)
        : Symbol(Kind::Variable), value(v), type(t) {}

    llvm::Type *getType() const override { return type; }
    llvm::Value *getValue() const override { return value; }
    void setValue(llvm::Value *v) override { value = v; }
};

// ───────────────────────────────────────────
// Array Symbol
class ArraySymbol : public Symbol
{
    llvm::Value *value;
    llvm::Type *elementType;
    int startIndex;
    int endIndex;

public:
    ArraySymbol(llvm::Type *elemType, llvm::Value *v, int start, int end)
        : Symbol(Kind::Array), value(v), elementType(elemType),
          startIndex(start), endIndex(end) {}

    llvm::Type *getType() const override
    {
        int size = endIndex - startIndex + 1;
        return llvm::ArrayType::get(elementType, size);
    }

    llvm::Value *getValue() const override { return value; }
    void setValue(llvm::Value *v) override { value = v; }

    llvm::Type *getElementType() const { return elementType; }
    int getStartIndex() const { return startIndex; }
    int getEndIndex() const { return endIndex; }
};

// ───────────────────────────────────────────
// Function Symbol
class FunctionSymbol : public Symbol
{
    llvm::Function *function;
    llvm::Type *returnType;
    std::vector<llvm::Type *> paramTypes;

public:
    FunctionSymbol(llvm::Function *func, llvm::Type *retType,
                   std::vector<llvm::Type *> params)
        : Symbol(Kind::Function), function(func),
          returnType(retType), paramTypes(std::move(params)) {}

    llvm::Type *getType() const override { return returnType; }
    llvm::Value *getValue() const override { return function; }
    void setValue(llvm::Value *val) override
    {
        function = llvm::cast<llvm::Function>(val);
    }

    llvm::Function *getFunction() const { return function; }
    const std::vector<llvm::Type *> &getParamTypes() const { return paramTypes; }
};

// ───────────────────────────────────────────
// Symbol Table with scoped support
class SymbolTable
{
public:
    SymbolTable() = default;

    void enterScope();
    void exitScope();

    llvm::Value *lookupSymbol(const std::string &id, llvm::Value *index = nullptr);
    void declareSymbol(const std::string &id, llvm::Type *type,
                       bool isArray = false, int startIndex = -1, int endIndex = -1);
    llvm::AllocaInst *allocateSymbol(const std::string &id);
    llvm::Type *getSymbolType(const std::string &id);
    bool checkDeclaration(const std::string &id);

private:
    std::stack<std::unordered_map<std::string, std::shared_ptr<Symbol>>> SymbolTableStack;
};

#endif // SYMBOL_TABLE_H
