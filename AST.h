#ifndef AST_H
#define AST_H

#include "IR.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Symbol_Table.h"
using namespace llvm;

// Definition of expression types
enum class ExprType
{
    Number,
    Binary,
    Unary,
    FuncCall,
    Integer,
    Real,
    Identifier,
    String,
    Char,
    Date,
    Boolean
};

class ASTNode
{
public:
    enum NodeType
    {
        NK_Type,
        NK_CharLiteral,
        NK_StringLiteral,
        NK_IntegerLiteral,
        NK_RealLiteral,
        NK_DateLiteral,
        NK_Identifier,
        NK_Assignment,
        NK_BinaryOp,
        NK_UnaryOp,
        NK_PrintStr,
        NK_PrintRealLiteral,
        NK_Comparison,
        NK_If,
        NK_For,
        NK_While,
        NK_Repeat,
        NK_Procedure,
        NK_Function,
        NK_FuncCall,
        NK_Return,
        NK_Declaration,
        NK_LogicalOp,
        NK_StatementBlock,
        NK_BooleanLiteral
    };

    NodeType nodeType;
    virtual ~ASTNode() = default; // Virtual destructor
    virtual void analyze(SymbolTable &symtab) = 0;
    virtual llvm::Value *codegen() = 0;

protected:
    ASTNode(NodeType type) : nodeType(type) {}
};

class TypeAST : public ASTNode
{
public:
    std::string type;
    using TypeGenerator = std::function<llvm::Type *(llvm::LLVMContext &)>;

    static const std::unordered_map<std::string, TypeAST::TypeGenerator> typeMap;

    TypeAST(const std::string &t) : ASTNode(NK_Type), type(t) {}
    void analyze(SymbolTable &symtab) override {}
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Type;
    }
};

class CharLiteralAST : public ASTNode
{
public:
    char value;

    CharLiteralAST(char val) : ASTNode(NK_CharLiteral), value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_CharLiteral;
    }
};

class StringLiteralAST : public ASTNode
{
public:
    std::string value;

    StringLiteralAST(const std::string &val) : ASTNode(NK_StringLiteral), value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_StringLiteral;
    }
};

class IntegerLiteralAST : public ASTNode
{
public:
    int value;

    IntegerLiteralAST(int val) : ASTNode(NK_IntegerLiteral), value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_IntegerLiteral;
    }
};

class RealLiteralAST : public ASTNode
{
public:
    double value;

    RealLiteralAST(double val) : ASTNode(NK_RealLiteral), value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_RealLiteral;
    }
};

class DateLiteralAST : public ASTNode
{
public:
    std::string value;

    DateLiteralAST(const std::string &val) : ASTNode(NK_DateLiteral), value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_DateLiteral;
    }
};

class IdentifierAST : public ASTNode
{
public:
    std::string name;

    IdentifierAST(const std::string &name) : ASTNode(NK_Identifier), name(name) {}
    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Identifier;
    }
};

class AssignmentAST : public ASTNode
{
public:
    std::string identifier;
    ASTNode *expression;
    TypeAST *declaredType; // Type from declaration, null if not declared

    AssignmentAST(const std::string &id, ASTNode *expr, TypeAST *type = nullptr)
        : ASTNode(NK_Assignment), identifier(id), expression(expr), declaredType(type) {}

    void analyze(SymbolTable &symtab) override
    {
        if (expression)
            expression->analyze(symtab);
    }

    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Assignment;
    }
};

class BinaryOpAST : public ASTNode
{
public:
    char op;
    ASTNode *expression1;
    ASTNode *expression2;

    BinaryOpAST(ASTNode *pexp1, ASTNode *pexpr2, int operation)
        : ASTNode(NK_BinaryOp), expression1(pexp1), expression2(pexpr2), op(operation) {}

    void analyze(SymbolTable &symtab) override
    {
        if (expression1)
            expression1->analyze(symtab);
        if (expression2)
            expression2->analyze(symtab);
    }
    Value *codegen() override;
    Value *codegen_add_one();

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_BinaryOp;
    }
};

class UnaryOpAST : public ASTNode
{
public:
    char op; // For operations like '-', '!', etc.
    ASTNode *operand;

    UnaryOpAST(char opr, ASTNode *operandNode)
        : ASTNode(NK_UnaryOp), op(opr), operand(operandNode) {}

    void analyze(SymbolTable &symtab) override
    {
        if (operand)
            operand->analyze(symtab);
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_UnaryOp;
    }
};

class PrintStrAST : public ASTNode
{
public:
    std::string str;

    PrintStrAST(const std::string &s) : ASTNode(NK_PrintStr), str(s) {}
    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_PrintStr;
    }
};

class PrintRealLiteralAST : public ASTNode
{
public:
    ASTNode *expression;
    PrintRealLiteralAST(ASTNode *expr) : ASTNode(NK_PrintRealLiteral), expression(expr) {}
    void analyze(SymbolTable &symtab) override
    {
        if (expression)
            expression->analyze(symtab);
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_PrintRealLiteral;
    }
};

class ComparisonAST : public ASTNode
{
public:
    int cmpOp; // Example: 1 for '>', 2 for '<', 3 for '==', etc.
    ASTNode *LHS;
    ASTNode *RHS;

    ComparisonAST(int op, ASTNode *lhs, ASTNode *rhs)
        : ASTNode(NK_Comparison), cmpOp(op), LHS(lhs), RHS(rhs) {}

    void analyze(SymbolTable &symtab) override
    {
        if (LHS)
            LHS->analyze(symtab);
        if (RHS)
            RHS->analyze(symtab);
    }
    Value *codegen() override;
    Value *codegen_single();

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Comparison;
    }
};

class IfAST : public ASTNode
{
public:
    ComparisonAST *condition; // Changed from ASTNode* to ComparisonAST*
    std::vector<ASTNode *> thenBlock;
    std::vector<ASTNode *> elseBlock;

    IfAST(ComparisonAST *cond, const std::vector<ASTNode *> &thenBlk, const std::vector<ASTNode *> &elseBlk)
        : ASTNode(NK_If), condition(cond), thenBlock(thenBlk), elseBlock(elseBlk) {}

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

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_If;
    }
};

class ForAST : public ASTNode
{
public:
    AssignmentAST *assignment; // Changed from ASTNode* to AssignmentAST*
    ComparisonAST *condition;  // Changed from ASTNode* to ComparisonAST*
    BinaryOpAST *expression;   // Changed from ASTNode* to BinaryOpAST* (typically increment/decrement)
    std::vector<ASTNode *> forBlock;

    ForAST(AssignmentAST *assign, ComparisonAST *cond, BinaryOpAST *exp, const std::vector<ASTNode *> &forBlk)
        : ASTNode(NK_For), assignment(assign), condition(cond), expression(exp), forBlock(forBlk) {}

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

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_For;
    }
};

class WhileAST : public ASTNode
{
public:
    ComparisonAST *condition; // Changed from ASTNode* to ComparisonAST*
    std::vector<ASTNode *> body;

    WhileAST(ComparisonAST *cond, const std::vector<ASTNode *> &bodyStatements)
        : ASTNode(NK_While), condition(cond), body(bodyStatements) {}

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

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_While;
    }
};

class RepeatAST : public ASTNode
{
public:
    ComparisonAST *condition; // Changed from ASTNode* to ComparisonAST*
    std::vector<ASTNode *> body;

    RepeatAST(ComparisonAST *cond, const std::vector<ASTNode *> &bodyStatements)
        : ASTNode(NK_Repeat), condition(cond), body(bodyStatements) {}

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

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Repeat;
    }
};

class ProcedureAST : public ASTNode
{
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<ASTNode *> statementsBlock;

    ProcedureAST(const std::string &procName, const std::vector<std::string> &params,
                 const std::vector<ASTNode *> &stmtBlock)
        : ASTNode(NK_Procedure), name(procName), parameters(params), statementsBlock(stmtBlock) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *stmt : statementsBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Procedure;
    }
};

class FuncAST : public ASTNode
{
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<ASTNode *> statementsBlock;
    TypeAST *returnType;

    FuncAST(const std::string &n, const std::vector<std::string> &params, const std::vector<ASTNode *> &block, TypeAST *retType)
        : ASTNode(NK_Function), name(n), parameters(params), statementsBlock(block), returnType(retType) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *stmt : statementsBlock)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Function;
    }
};

class FuncCallAST : public ASTNode
{
public:
    std::string name;
    std::vector<ASTNode *> arguments;

    FuncCallAST(const std::string &funcName, const std::vector<ASTNode *> &args)
        : ASTNode(NK_FuncCall), name(funcName), arguments(args) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *arg : arguments)
        {
            if (arg)
                arg->analyze(symtab);
        }
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_FuncCall;
    }
};

class ReturnAST : public ASTNode
{
public:
    ASTNode *expression;

    ReturnAST(ASTNode *expr) : ASTNode(NK_Return), expression(expr) {}

    void analyze(SymbolTable &symtab) override
    {
        if (expression)
            expression->analyze(symtab);
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_Return;
    }
};

// Add missing classes
class DeclarationAST : public ASTNode
{
public:
    std::string identifier;
    TypeAST *type;

    DeclarationAST(const std::string &id, TypeAST *t)
        : ASTNode(NK_Declaration), identifier(id), type(t) {}

    void analyze(SymbolTable &symtab) override
    {
        if (type)
            type->analyze(symtab);
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        // Check if the node is of type DeclarationAST
        llvm::errs() << "Checking class of AST\n";
        return node->nodeType == NK_Declaration;
    }
};

class LogicalOpAST : public ASTNode
{
public:
    std::string op;
    ASTNode *lhs;
    ASTNode *rhs;

    LogicalOpAST(const std::string &operation, ASTNode *left, ASTNode *right)
        : ASTNode(NK_LogicalOp), op(operation), lhs(left), rhs(right) {}

    void analyze(SymbolTable &symtab) override
    {
        if (lhs)
            lhs->analyze(symtab);
        if (rhs)
            rhs->analyze(symtab);
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_LogicalOp;
    }
};

class StatementBlockAST : public ASTNode
{
public:
    std::vector<ASTNode *> statements;

    StatementBlockAST(const std::vector<ASTNode *> &stmts) : ASTNode(NK_StatementBlock), statements(stmts) {}

    void analyze(SymbolTable &symtab) override
    {
        for (auto *stmt : statements)
        {
            if (stmt)
                stmt->analyze(symtab);
        }
    }
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_StatementBlock;
    }
};

class BooleanLiteralAST : public ASTNode
{
public:
    bool value;

    BooleanLiteralAST(bool val) : ASTNode(NK_BooleanLiteral), value(val) {}

    void analyze(SymbolTable &symtab) override {} // Empty implementation
    Value *codegen() override;

    static bool classof(const ASTNode *node)
    {
        return node->nodeType == NK_BooleanLiteral;
    }
};

#endif // AST_H
