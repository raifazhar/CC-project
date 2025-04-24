#ifndef AST_H
#define AST_H

#include "IR.h"
#include <string>
#include <vector>
#include "Symbol_Table.h"
using namespace llvm;

class ASTNode
{
public:
    virtual ~ASTNode() = default; // Virtual destructor
    virtual void analyze(SymbolTable &symtab) = 0;
    virtual llvm::Value *codegen() = 0;
};

class TypeAST : public ASTNode
{
public:
    std::string type;

    TypeAST(const std::string &t) : type(t) {}
    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class CharLiteralAST : public ASTNode
{
public:
    char value;

    CharLiteralAST(char val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class StringLiteralAST : public ASTNode
{
public:
    std::string value;

    StringLiteralAST(const std::string &val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class IntegerLiteralAST : public ASTNode
{
public:
    int value;

    IntegerLiteralAST(int val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class RealLiteralAST : public ASTNode
{
public:
    double value;

    RealLiteralAST(double val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class DateLiteralAST : public ASTNode
{
public:
    std::string value;

    DateLiteralAST(const std::string &val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class IdentifierAST : public ASTNode
{
public:
    std::string name;

    IdentifierAST(const std::string &name) : name(name) {}
    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class AssignmentAST : public ASTNode
{
public:
    std::string identifier;
    ASTNode *expression;

    AssignmentAST(const std::string &id, ASTNode *expr)
        : identifier(id), expression(expr) {}

    void analyze(SymbolTable &symtab) override
    {
        if (expression)
            expression->analyze(symtab);
    }
    Value *codegen() override;
};

class BinaryOpAST : public ASTNode
{
public:
    char op;
    ASTNode *expression1;
    ASTNode *expression2;

    BinaryOpAST(ASTNode *pexp1, ASTNode *pexpr2, int operation)
        : expression1(pexp1), expression2(pexpr2), op(operation) {}

    void analyze(SymbolTable &symtab) override
    {
        if (expression1)
            expression1->analyze(symtab);
        if (expression2)
            expression2->analyze(symtab);
    }
    Value *codegen() override;
    Value *codegen_add_one();
};

class UnaryOpAST : public ASTNode
{
public:
    char op; // For operations like '-', '!', etc.
    ASTNode *operand;

    UnaryOpAST(char opr, ASTNode *operandNode)
        : op(opr), operand(operandNode) {}

    void analyze(SymbolTable &symtab) override
    {
        if (operand)
            operand->analyze(symtab);
    }
    Value *codegen() override;
};

class PrintStrAST : public ASTNode
{
public:
    std::string str;

    PrintStrAST(const std::string &s) : str(s) {}
    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

class PrintDoubleAST : public ASTNode
{
public:
    ASTNode *expression;

    PrintDoubleAST(ASTNode *expr) : expression(expr) {}
    void analyze(SymbolTable &symtab) override
    {
        if (expression)
            expression->analyze(symtab);
    }
    Value *codegen() override;
};

class ComparisonAST : public ASTNode
{
public:
    int cmpOp; // Example: 1 for '>', 2 for '<', 3 for '==', etc.
    ASTNode *LHS;
    ASTNode *RHS;

    ComparisonAST(int op, ASTNode *lhs, ASTNode *rhs)
        : cmpOp(op), LHS(lhs), RHS(rhs) {}

    void analyze(SymbolTable &symtab) override
    {
        if (LHS)
            LHS->analyze(symtab);
        if (RHS)
            RHS->analyze(symtab);
    }
    Value *codegen() override;
    Value *codegen_single();
};

class IfAST : public ASTNode
{
public:
    ASTNode *condition;
    std::vector<ASTNode *> thenBlock;
    std::vector<ASTNode *> elseBlock;

    IfAST(ASTNode *cond, const std::vector<ASTNode *> &thenBlk, const std::vector<ASTNode *> &elseBlk)
        : condition(cond), thenBlock(thenBlk), elseBlock(elseBlk) {}

    void analyze(SymbolTable &symtab) override
    {
        if (condition)
            condition->analyze(symtab);
        for (auto *stmt : thenBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
        for (auto *stmt : elseBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class ForAST : public ASTNode
{
public:
    ASTNode *assignment;
    ASTNode *condition;
    ASTNode *expression;
    std::vector<ASTNode *> forBlock;

    ForAST(ASTNode *assign, ASTNode *cond, ASTNode *exp, const std::vector<ASTNode *> &forBlk)
        : assignment(assign), condition(cond), expression(exp), forBlock(forBlk) {}

    void analyze(SymbolTable &symtab) override
    {
        if (assignment)
            assignment->analyze(symtab);
        if (condition)
            condition->analyze(symtab);
        if (expression)
            expression->analyze(symtab);
        for (auto *stmt : forBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class WhileAST : public ASTNode
{
public:
    ASTNode *condition;
    std::vector<ASTNode *> body;

    WhileAST(ASTNode *cond, const std::vector<ASTNode *> &bodyStatements)
        : condition(cond), body(bodyStatements) {}

    void analyze(SymbolTable &symtab) override
    {
        if (condition)
            condition->analyze(symtab);
        for (auto *stmt : body)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class RepeatAST : public ASTNode
{
public:
    ASTNode *condition;
    std::vector<ASTNode *> body;

    RepeatAST(ASTNode *cond, const std::vector<ASTNode *> &bodyStatements)
        : condition(cond), body(bodyStatements) {}

    void analyze(SymbolTable &symtab) override
    {
        if (condition)
            condition->analyze(symtab);
        for (auto *stmt : body)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class ProcedureAST : public ASTNode
{
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<ASTNode *> statementsBlock;

    ProcedureAST(const std::string &procName, const std::vector<std::string> &params,
                 const std::vector<ASTNode *> &stmtBlock)
        : name(procName), parameters(params), statementsBlock(stmtBlock) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *stmt : statementsBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class FuncAST : public ASTNode
{
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<ASTNode *> statementsBlock;
    TypeAST *returnType;

    FuncAST(const std::string &n, const std::vector<std::string> &params, const std::vector<ASTNode *> &block, TypeAST *retType)
        : name(n), parameters(params), statementsBlock(block), returnType(retType) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *stmt : statementsBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
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

    void analyze(SymbolTable &symtab) override
    {
        for (auto *arg : arguments)
        {
            if (arg)
                arg->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class ReturnAST : public ASTNode
{
public:
    ASTNode *expression;

    ReturnAST(ASTNode *expr) : expression(expr) {}

    void analyze(SymbolTable &symtab) override
    {
        if (expression)
            expression->analyze(symtab);
    }
    Value *codegen() override;
};

// Add missing classes
class DeclarationAST : public ASTNode
{
public:
    std::string identifier;
    TypeAST *type;

    DeclarationAST(const std::string &id, TypeAST *t)
        : identifier(id), type(t) {}

    void analyze(SymbolTable &symtab) override
    {
        if (type)
            type->analyze(symtab);
    }
    Value *codegen() override;
};

class NumberAST : public ASTNode
{
public:
    double value;

    NumberAST(double val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
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

    void analyze(SymbolTable &symtab) override
    {
        if (lhs)
            lhs->analyze(symtab);
        if (rhs)
            rhs->analyze(symtab);
    }
    Value *codegen() override;
};

class StatementBlockAST : public ASTNode
{
public:
    std::vector<ASTNode *> statements;

    StatementBlockAST(const std::vector<ASTNode *> &stmts) : statements(stmts) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *stmt : statements)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;
};

class BooleanLiteralAST : public ASTNode
{
public:
    bool value;

    BooleanLiteralAST(bool val) : value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;
};

#endif // AST_H
