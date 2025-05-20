#ifndef AST_H
#define AST_H

#include "IR.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Symbol_Table.h"
using namespace llvm;
class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual llvm::Value *codegen() = 0;
    virtual bool semanticCheck() { return true; };
    ;
};

// --- Type ---
class TypeAST : public ASTNode
{
public:
    std::string type;
    using TypeGenerator = std::function<llvm::Type *(llvm::LLVMContext &)>;
    static const std::unordered_map<std::string, TypeGenerator> typeMap;

    TypeAST(const std::string &t) : type(t) {}
    Value *codegen() override
    {
        return nullptr;
    }
    Type *giveType()
    {
        return TypeAST::typeMap.at(type)(context);
    }
    bool semanticCheck() override
    {
        if (typeMap.find(type) == typeMap.end())
        {
            llvm::errs() << "Semantic error: Unknown type '" << type << "'\n";
            return false;
        }
        return true;
    }
};

// --- Literals ---
class CharLiteralAST : public ASTNode
{
public:
    char value;
    CharLiteralAST(char val) : value(val) {}
    Value *codegen()
    {
        return ConstantInt::get(Type::getInt8Ty(context), value);
    }
};

class StringLiteralAST : public ASTNode
{
public:
    std::string value;
    StringLiteralAST(const std::string &val) : value(val) {}
    Value *codegen()
    {
        Constant *strConst = ConstantDataArray::getString(context, value);
        GlobalVariable *gvar = new GlobalVariable(
            *module,
            strConst->getType(),
            true,
            GlobalValue::PrivateLinkage,
            strConst,
            ".str");
        return gvar;
    }
};

class IntegerLiteralAST : public ASTNode
{
public:
    int value;
    IntegerLiteralAST(int val) : value(val) {}
    Value *codegen()
    {
        return ConstantInt::get(Type::getInt32Ty(context), value);
    }
};

class RealLiteralAST : public ASTNode
{
public:
    double value;
    RealLiteralAST(double val) : value(val) {}
    Value *codegen()
    {
        return ConstantFP::get(context, APFloat(value));
    }
};

class DateLiteralAST : public ASTNode
{
public:
    std::string value;
    DateLiteralAST(const std::string &val) : value(val) {}
    Value *codegen()
    {
        Constant *strConst = ConstantDataArray::getString(context, value);
        GlobalVariable *gvar = new GlobalVariable(
            *module,
            strConst->getType(),
            true,
            GlobalValue::PrivateLinkage,
            strConst,
            ".date");
        return gvar;
    }
};

class BooleanLiteralAST : public ASTNode
{
public:
    bool value;
    BooleanLiteralAST(bool val) : value(val) {}
    Value *codegen()
    {
        return ConstantInt::get(Type::getInt1Ty(context), value);
    }
};

// --- Identifiers and Assignments ---
class IdentifierAST : public ASTNode
{
public:
    std::string name;
    IdentifierAST(const std::string &name) : name(name) {}
    bool semanticCheck() override
    {
        if (!globalSymbolTable->lookupSymbol(name))
        {
            llvm::errs() << "Semantic error: Undeclared identifier '" << name << "'\n";
            return false;
        }
        return true;
    }
    Value *codegen() override;
};

class DeclarationAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    TypeAST *type;

    DeclarationAST(IdentifierAST *id, TypeAST *t)
        : identifier(id), type(t) {}
    bool semanticCheck() override
    {
        // Check type validity first
        if (!type->semanticCheck())
            return false;

        // Check if identifier already declared
        if (globalSymbolTable->checkDeclaration(identifier->name))
        {
            llvm::errs() << "Semantic error: Variable '" << identifier->name << "' already declared\n";
            return false;
        }
        llvm::errs() << identifier->name << "' set declared\n";
        globalSymbolTable->declareSymbol(identifier->name, type->giveType());
        return true;
    }
    Value *codegen() override;
};

class ArrayAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    TypeAST *type;
    size_t firstIndex; // Size of the array
    size_t lastIndex;

    ArrayAST(IdentifierAST *id, TypeAST *t, size_t first, size_t last)
        : identifier(id), type(t), firstIndex(first), lastIndex(last) {}
    bool semanticCheck() override
    {
        if (!type->semanticCheck())
            return false;
        if (globalSymbolTable->checkDeclaration(identifier->name))
        {
            llvm::errs() << "Semantic error: Array '" << identifier->name << "' already declared\n";
            return false;
        }
        if (lastIndex < firstIndex)
        {
            llvm::errs() << "Semantic error: Array '" << identifier->name << "' last index less than first index\n";
            return false;
        }
        globalSymbolTable->declareSymbol(identifier->name, type->giveType(), true, firstIndex, lastIndex);
        return true;
    }
    Value *codegen() override;
};

class AssignmentAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    ASTNode *expression;

    AssignmentAST(IdentifierAST *id, ASTNode *expr)
        : identifier(id), expression(expr) {}
    bool semanticCheck() override
    {
        // Identifier must be declared
        if (!identifier->semanticCheck())
            return false;

        // Type compatibility check
        if (!expression->semanticCheck())
            return false;

        Value *var = globalSymbolTable->lookupSymbol(identifier->name);
        if (!var)
        {
            llvm::errs() << "Semantic error: Undeclared variable '" << identifier->name << "'\n";
            return false;
        }

        // Assume types are compatible (extend this as needed)
        return true;
    }
    Value *codegen() override;
};

class ArrayAssignmentAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    ASTNode *expression;
    ASTNode *index;

    ArrayAssignmentAST(IdentifierAST *id, ASTNode *expr, ASTNode *index)
        : identifier(id), expression(expr), index(index) {}
    bool semanticCheck() override
    {
        if (!identifier->semanticCheck())
            return false;
        if (!index->semanticCheck())
            return false;
        if (!expression->semanticCheck())
            return false;
        if (!globalSymbolTable->lookupSymbol(identifier->name))
        {
            llvm::errs() << "Semantic error: Undeclared array '" << identifier->name << "'\n";
            return false;
        }
        return true;
    }
    Value *codegen() override;
};

class ArrayAccessAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    ASTNode *index;
    ArrayAccessAST(IdentifierAST *id, ASTNode *index)
        : identifier(id), index(index) {}

    bool semanticCheck() override
    {
        if (!identifier->semanticCheck())
            return false;
        if (!index->semanticCheck())
            return false;
        if (!globalSymbolTable->lookupSymbol(identifier->name))
        {
            llvm::errs() << "Semantic error: Undeclared array '" << identifier->name << "'\n";
            return false;
        }
        return true;
    }
    Value *codegen() override;
};
class OutputAST : public ASTNode
{
public:
    std::vector<ASTNode *> expressions;

    // Constructor takes a vector
    OutputAST(std::vector<ASTNode *> expressions)
        : expressions(std::move(expressions)) {}
    bool semanticCheck() override
    {
        bool ok = true;
        for (ASTNode *expr : expressions)
        {
            if (!expr->semanticCheck())
                ok = false;
        }
        return ok;
    }

    Value *codegen() override;
};

class InputAST : public ASTNode
{
public:
    ASTNode *target;

    InputAST(ASTNode *id)
        : target(id) {}
    bool semanticCheck() override
    {
        return target->semanticCheck();
    }
    Value *codegen() override;
};

// --- Expressions ---
class BinaryOpAST : public ASTNode
{
public:
    std::string op;
    ASTNode *expression1;
    ASTNode *expression2;

    BinaryOpAST(ASTNode *lhs, ASTNode *rhs, std::string operation)
        : op(operation), expression1(lhs), expression2(rhs) {}

    bool semanticCheck() override
    {
        bool leftOk = expression1->semanticCheck();
        bool rightOk = expression2->semanticCheck();

        if (!leftOk || !rightOk)
            return false;

        // Optionally check type compatibility here

        return true;
    }
    Value *codegen() override;
};

class UnaryOpAST : public ASTNode
{
public:
    ASTNode *expression;
    std::string op;

    UnaryOpAST(ASTNode *exp, std::string operation)
        : expression(exp), op(operation) {}

    bool semanticCheck() override
    {
        return expression->semanticCheck();
    }
    Value *codegen() override
    {
        Value *v = expression->codegen();
        if (!v)
            return nullptr;
        if (op == "-")
        {
            if (v->getType()->isIntegerTy())
                return builder.CreateNeg(v, "negtmp");
            else if (v->getType()->isDoubleTy())
                return builder.CreateFNeg(v, "fnegtmp");
        }
        errs() << "Unknown unary op: " << op << "\n";
        return nullptr;
    }
};

class ComparisonAST : public ASTNode
{
public:
    std::string cmpOp;
    ASTNode *LHS;
    ASTNode *RHS;

    ComparisonAST(ASTNode *lhs, ASTNode *rhs, std::string op)
        : LHS(lhs), RHS(rhs), cmpOp(op) {}
    bool semanticCheck() override
    {
        bool lhsOk = LHS->semanticCheck();
        bool rhsOk = RHS->semanticCheck();
        return lhsOk && rhsOk;
    }
    Value *codegen() override;
};
class LogicalOpAST : public ComparisonAST
{
public:
    LogicalOpAST(ASTNode *left, ASTNode *right, const std::string &operation)
        : ComparisonAST(left, right, operation) {}
    Value *codegen() override;
};
// --- Statements ---
class StatementBlockAST : public ASTNode
{
public:
    std::vector<ASTNode *> statements;
    StatementBlockAST(const std::vector<ASTNode *> &stmts) : statements(stmts) {}
    bool semanticCheck() override
    {
        bool ok = true;
        for (ASTNode *stmt : statements)
        {
            if (!stmt->semanticCheck())
                ok = false;
        }
        return ok;
    }
    Value *codegen() override;
};

class IfAST : public ASTNode
{
public:
    ComparisonAST *condition;
    StatementBlockAST *thenBlock;
    StatementBlockAST *elseBlock;

    IfAST(ComparisonAST *cond, StatementBlockAST *thenBlk, StatementBlockAST *elseBlk)
        : condition(cond), thenBlock(thenBlk), elseBlock(elseBlk) {}
    bool semanticCheck() override
    {
        bool condOk = condition->semanticCheck();
        bool thenOk = thenBlock->semanticCheck();
        bool elseOk = elseBlock ? elseBlock->semanticCheck() : true;
        return condOk && thenOk && elseOk;
    }
    Value *codegen() override;
};

class ForAST : public ASTNode
{
public:
    AssignmentAST *assignment;
    ComparisonAST *condition;
    AssignmentAST *step; // changed type from BinaryOpAST* to AssignmentAST*
    StatementBlockAST *forBlock;

    ForAST(AssignmentAST *assign, ComparisonAST *cond, AssignmentAST *stepvalue, StatementBlockAST *forBlk)
        : assignment(assign), condition(cond), step(stepvalue), forBlock(forBlk) {}
    bool semanticCheck() override
    {
        bool assignmentok = assignment->semanticCheck();
        bool conditionok = condition->semanticCheck();
        bool forblockok = forBlock->semanticCheck();
        return assignmentok && conditionok && forblockok;
    }

    Value *codegen() override;
};

class WhileAST : public ASTNode
{
public:
    ComparisonAST *condition;
    StatementBlockAST *body;

    WhileAST(ComparisonAST *cond, StatementBlockAST *bodyStatements)
        : condition(cond), body(bodyStatements) {}
    bool semanticCheck() override
    {
        return body->semanticCheck();
    }
    Value *codegen() override;
};

class RepeatAST : public ASTNode
{
public:
    ComparisonAST *condition;
    StatementBlockAST *body;

    RepeatAST(ComparisonAST *cond, StatementBlockAST *bodyStatements)
        : condition(cond), body(bodyStatements) {}
    bool semanticCheck() override
    {
        return body->semanticCheck();
    }
    Value *codegen() override;
};

// --- Functions and Procedures ---
class ParameterAST : public ASTNode
{
public:
    TypeAST *type;    // like "INTEGER", "REAL", etc.
    std::string name; // variable name

    ParameterAST(TypeAST *type, const std::string &name)
        : type(type), name(name) {}
    bool semanticCheck() override
    {
        return type->semanticCheck();
    }

    Value *codegen() override
    {
        return nullptr;
    }
};

class ProcedureAST : public ASTNode
{
public:
    IdentifierAST *Identifier;
    std::vector<ParameterAST *> parameters;
    StatementBlockAST *statementsBlock;

    ProcedureAST(IdentifierAST *procidentifier, const std::vector<ParameterAST *> &params, StatementBlockAST *stmtBlock)
        : Identifier(procidentifier), parameters(params), statementsBlock(stmtBlock) {}
    bool semanticCheck() override
    {
        bool ok = true;
        for (auto *param : parameters)
        {
            if (!param->semanticCheck())
                ok = false;
        }
        if (!statementsBlock->semanticCheck())
            ok = false;
        return ok;
    }

    Value *codegen() override;
};

class FuncAST : public ASTNode
{
public:
    IdentifierAST *Identifier;
    std::vector<ParameterAST *> parameters;
    StatementBlockAST *statementsBlock;
    TypeAST *returnType;

    FuncAST(IdentifierAST *funcidentifier, const std::vector<ParameterAST *> &params, StatementBlockAST *block, TypeAST *retType)
        : Identifier(funcidentifier), parameters(params), statementsBlock(block), returnType(retType) {}
    bool semanticCheck() override
    {
        bool ok = true;
        if (!returnType->semanticCheck())
            ok = false;
        for (auto *param : parameters)
        {
            if (!param->semanticCheck())
                ok = false;
        }
        if (!statementsBlock->semanticCheck())
            ok = false;
        return ok;
    }
    Value *codegen() override;
};
class ReturnAST : public ASTNode
{
public:
    ASTNode *expression;
    ReturnAST(ASTNode *expr) : expression(expr) {}
    bool semanticCheck() override
    {

        return expression->semanticCheck();
    }

    Value *codegen() override;
};

class FuncCallAST : public ASTNode
{
public:
    std::string name;
    std::vector<ASTNode *> arguments;

    FuncCallAST(const std::string &funcName, const std::vector<ASTNode *> &args)
        : name(funcName), arguments(args) {}
    bool semanticCheck() override
    {
        bool ok = true;
        for (ASTNode *arg : arguments)
        {
            if (!arg->semanticCheck())
                ok = false;
        }
        // Add lookup for function if you maintain a procedure/function symbol table
        return ok;
    }

    Value *codegen() override;
};
#endif // AST_H
