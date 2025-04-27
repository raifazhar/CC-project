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
    Value *codegen() override;
};

class DeclarationAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    TypeAST *type;

    DeclarationAST(IdentifierAST *id, TypeAST *t)
        : identifier(id), type(t) {}
    Value *codegen() override;
};
class AssignmentAST : public ASTNode
{
public:
    IdentifierAST *identifier;
    ASTNode *expression;
    TypeAST *declaredType;

    AssignmentAST(IdentifierAST *id, ASTNode *expr, TypeAST *type = nullptr)
        : identifier(id), expression(expr), declaredType(type) {}
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
    Value *codegen() override;
};

class LogicalOpAST : public ASTNode
{
public:
    std::string op;
    ASTNode *lhs;
    ASTNode *rhs;

    LogicalOpAST(const std::string &operation, ASTNode *left, ASTNode *right)
        : op(operation), lhs(left), rhs(right) {}
    Value *codegen() override;
};

class ComparisonAST : public ASTNode
{
public:
    std::string cmpOp;
    ASTNode *LHS;
    ASTNode *RHS;

    ComparisonAST(ASTNode *lhs, ASTNode *rhs, std::string op)
        : LHS(lhs), RHS(rhs), cmpOp(op) {}
    Value *codegen() override;
    Value *codegen_single();
};

// --- Statements ---
class StatementBlockAST : public ASTNode
{
public:
    std::vector<ASTNode *> statements;
    StatementBlockAST(const std::vector<ASTNode *> &stmts) : statements(stmts) {}
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
    Value *codegen() override;
};

class WhileAST : public ASTNode
{
public:
    ComparisonAST *condition;
    std::vector<ASTNode *> body;

    WhileAST(ComparisonAST *cond, const std::vector<ASTNode *> &bodyStatements)
        : condition(cond), body(bodyStatements) {}
    Value *codegen() override;
};

class RepeatAST : public ASTNode
{
public:
    ComparisonAST *condition;
    std::vector<ASTNode *> body;

    RepeatAST(ComparisonAST *cond, const std::vector<ASTNode *> &bodyStatements)
        : condition(cond), body(bodyStatements) {}
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
    Value *codegen() override;
};
class ReturnAST : public ASTNode
{
public:
    ASTNode *expression;
    ReturnAST(ASTNode *expr) : expression(expr) {}
    Value *codegen() override;
};

class FuncCallAST : public ASTNode
{
public:
    std::string name;
    std::vector<ASTNode *> arguments;

    FuncCallAST(const std::string &funcName, const std::vector<ASTNode *> &args)
        : name(funcName), arguments(args) {}
    Value *codegen() override;
};
#endif // AST_H
