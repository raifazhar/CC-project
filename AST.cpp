#include "AST.h"
#include "IR.h"
#include <llvm/Support/raw_ostream.h>
#include <fstream>

#define DEBUG_PRINT_FUNCTION()                                                               \
    do                                                                                       \
    {                                                                                        \
        const char *filename = "debug_output.txt";                                           \
        std::ofstream debug_file(filename, std::ios::app);                                   \
        if (debug_file.is_open())                                                            \
        {                                                                                    \
            debug_file << "Entering " << __func__;                                           \
            if (!builder.GetInsertBlock())                                                   \
            {                                                                                \
                debug_file << " insertion point not set in " << __PRETTY_FUNCTION__ << "\n"; \
            }                                                                                \
            else                                                                             \
            {                                                                                \
                debug_file << " insertion point set in " << __PRETTY_FUNCTION__ << "\n";     \
            }                                                                                \
            debug_file.close();                                                              \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
            errs() << "Error opening debug file: " << filename << "\n";                      \
        }                                                                                    \
    } while (0)

Value *TypeAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    // Create a constant value of the appropriate type
    if (type == "int" || type == "INTEGER")
    {
        return ConstantInt::get(Type::getInt32Ty(context), 0);
    }
    else if (type == "real" || type == "REAL")
    {
        return ConstantFP::get(Type::getDoubleTy(context), 0.0);
    }
    else if (type == "boolean" || type == "BOOLEAN")
    {
        return ConstantInt::get(Type::getInt1Ty(context), 0);
    }
    else if (type == "char" || type == "CHAR")
    {
        return ConstantInt::get(Type::getInt8Ty(context), 0);
    }
    else if (type == "string" || type == "STRING")
    {
        return ConstantPointerNull::get(PointerType::get(Type::getInt8Ty(context), 0));
    }
    else if (type == "date" || type == "DATE")
    {
        return ConstantPointerNull::get(PointerType::get(Type::getInt8Ty(context), 0));
    }
    else
    {
        errs() << "Unknown type: " << type << "\n";
        return nullptr;
    }
}

Value *CharLiteralAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    return ConstantInt::get(Type::getInt8Ty(context), value);
}

Value *StringLiteralAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    // Create a global string constant
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

Value *IntegerLiteralAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    return ConstantInt::get(Type::getInt32Ty(context), value);
}

Value *RealLiteralAST::codegen()
{
    return createDoubleConstant(value);
}

Value *DateLiteralAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    // Create a global string constant for the date
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

Value *IdentifierAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *ptr = getFromSymbolTable(name);

    if (!ptr)
    {
        errs() << "Error: Symbol not found for " << name << "\n";
        return nullptr;
    }

    if (isa<AllocaInst>(ptr))
    {
        // Case 1: The pointer is an AllocaInst (local variable)
        auto *allocaInst = dyn_cast<AllocaInst>(ptr);
        Type *elementType = allocaInst->getAllocatedType();
        Value *loadedValue = builder.CreateLoad(elementType, ptr, "loaded_identifier");
        return loadedValue;
    }
    else if (isa<GlobalVariable>(ptr))
    {
        // Case 2: The pointer is a GlobalVariable (global variable)
        auto *globalVar = dyn_cast<GlobalVariable>(ptr);
        Type *elementType = globalVar->getValueType();
        Value *loadedValue = builder.CreateLoad(elementType, ptr, "loaded_global");
        return loadedValue;
    }
    else if (isa<Argument>(ptr))
    {
        // Case 3: The pointer is an Argument (function parameter)
        return ptr;
    }
    else
    {
        // Handle unexpected cases
        errs() << "Unexpected pointer type for " << name << "\n";
        return nullptr;
    }
}

Value *AssignmentAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *exprVal = expression->codegen();
    Value *ptr = getFromSymbolTable(identifier);
    setDouble(identifier, exprVal);
    return exprVal;
}

Value *BinaryOpAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    // Handle ++ / -- (postfix): expression2 is null
    if (expression2 == nullptr && (op == '+' || op == '-'))
    {
        IdentifierAST *var = dynamic_cast<IdentifierAST *>(expression1);
        if (!var)
        {
            errs() << "Error: Increment/Decrement only valid on variables.\n";
            return nullptr;
        }

        Value *varPtr = getFromSymbolTable(var->name);
        if (!varPtr || !varPtr->getType()->isPointerTy())
        {
            errs() << "Invalid variable for post-inc/dec: " << var->name << "\n";
            return nullptr;
        }

        Value *oldVal = builder.CreateLoad(Type::getDoubleTy(context), varPtr, "loadtmp");
        Value *one = ConstantFP::get(Type::getDoubleTy(context), 1.0);
        Value *newVal = (op == '+')
                            ? builder.CreateFAdd(oldVal, one, "inc")
                            : builder.CreateFSub(oldVal, one, "dec");

        builder.CreateStore(newVal, varPtr);

        // Return old value for post-inc semantics
        return oldVal;
    }

    // Normal binary operation: i + 1, a * b, etc.
    Value *exprVal1 = expression1->codegen();
    Value *exprVal2 = expression2->codegen();

    errs() << op << "\n";
    return performBinaryOperation(exprVal1, exprVal2, op);
}

Value *BinaryOpAST::codegen_add_one()
{
    DEBUG_PRINT_FUNCTION();

    // Assume expression1 is a VariableAST
    IdentifierAST *var = (IdentifierAST *)expression1;
    Value *varPtr = getFromSymbolTable(var->name);
    // Load, increment, store
    Value *oldVal = builder.CreateLoad(Type::getDoubleTy(context), varPtr, "loadtmp");
    Value *one = ConstantFP::get(Type::getDoubleTy(context), 1.0);
    Value *newVal;

    if (op == '+')
    {
        newVal = builder.CreateFAdd(oldVal, one, "addtmp");
    }
    else if (op == '-')
    {
        newVal = builder.CreateFSub(oldVal, one, "subtmp");
    }

    // If varPtr is a pointer, store the result back to the original variable
    if (varPtr->getType()->isPointerTy())
    {
        builder.CreateStore(newVal, varPtr);
    }

    // Optionally return newVal or oldVal for post-increment semantics
    return newVal;
}

Value *PrintStrAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    printString(str);
    return nullptr; // No value produced.
}

Value *PrintDoubleAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *val = expression->codegen();
    printDouble(val);
    return nullptr; // No value produced.
}

Value *ComparisonAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    Value *lhsVal = LHS->codegen();
    Value *rhsVal = RHS->codegen();
    return performComparison(lhsVal, rhsVal, cmpOp);
}

Value *ComparisonAST::codegen_single()
{
    DEBUG_PRINT_FUNCTION();

    Value *lhsVal = LHS->codegen();
    Value *rhsVal = ConstantFP::get(Type::getDoubleTy(context), 0.0);
    return performComparison(lhsVal, rhsVal, cmpOp);
}

Value *IfAST::codegen()
{
    Value *condVal = condition->codegen();
    Function *function = builder.GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(context, "if.then", function);
    BasicBlock *elseBB = BasicBlock::Create(context, "if.else", function);
    BasicBlock *mergeBB = BasicBlock::Create(context, "if.end", function);

    builder.CreateCondBr(condVal, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    for (auto *stmt : thenBlock)
    {
        stmt->codegen();
    }
    builder.CreateBr(mergeBB);

    builder.SetInsertPoint(elseBB);
    for (auto *stmt : elseBlock)
    {
        stmt->codegen();
    }
    builder.CreateBr(mergeBB);

    builder.SetInsertPoint(mergeBB);
    return nullptr;
}

Value *ForAST::codegen()
{
    // Generate the assignment (initialization) code
    if (assignment)
    {
        assignment->codegen();
    }

    // Generate the condition code
    Value *condVal = condition->codegen();

    // Create the basic blocks for the loop
    Function *function = builder.GetInsertBlock()->getParent();
    BasicBlock *loopBB = BasicBlock::Create(context, "loop", function);
    BasicBlock *loopEndBB = BasicBlock::Create(context, "loop.end", function);

    // Create the condition check before entering the loop
    builder.CreateCondBr(condVal, loopBB, loopEndBB);

    // Start the loop body
    builder.SetInsertPoint(loopBB);

    // Execute all statements inside the loop body
    for (auto *stmt : forBlock)
    {
        stmt->codegen();
    }

    // Execute the expression after the loop body (this is typically the increment expression in for loops)
    expression->codegen();

    // Check the condition again and either repeat the loop or end it
    condVal = condition->codegen();                   // Re-evaluate the condition
    builder.CreateCondBr(condVal, loopBB, loopEndBB); // Continue or exit based on condition

    // Set the insert point to the "loop.end" block after the loop terminates
    builder.SetInsertPoint(loopEndBB);

    return nullptr; // No value is returned
}
Value *WhileAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Function *function = builder.GetInsertBlock()->getParent();
    BasicBlock *condBB = BasicBlock::Create(context, "while.cond", function);
    BasicBlock *bodyBB = BasicBlock::Create(context, "while.body", function);
    BasicBlock *endBB = BasicBlock::Create(context, "while.end", function);

    // Branch to condition block
    builder.CreateBr(condBB);

    // Emit condition
    builder.SetInsertPoint(condBB);
    Value *condVal = condition->codegen();
    builder.CreateCondBr(condVal, bodyBB, endBB);

    // Emit body
    builder.SetInsertPoint(bodyBB);
    for (auto *stmt : body)
    {
        stmt->codegen();
    }
    builder.CreateBr(condBB);

    // Continue with end block
    builder.SetInsertPoint(endBB);
    return nullptr;
}
Value *RepeatAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Function *function = builder.GetInsertBlock()->getParent();
    BasicBlock *bodyBB = BasicBlock::Create(context, "repeat.body", function);
    BasicBlock *condBB = BasicBlock::Create(context, "repeat.cond", function);
    BasicBlock *endBB = BasicBlock::Create(context, "repeat.end", function);

    // Branch to body block
    builder.CreateBr(bodyBB);

    // Emit body
    builder.SetInsertPoint(bodyBB);
    for (auto *stmt : body)
    {
        stmt->codegen();
    }
    builder.CreateBr(condBB);

    // Emit condition
    builder.SetInsertPoint(condBB);
    Value *condVal = condition->codegen();
    builder.CreateCondBr(condVal, endBB, bodyBB);

    // Continue with end block
    builder.SetInsertPoint(endBB);
    return nullptr;
}
Value *ProcedureAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    std::vector<Type *> paramTypes(parameters.size(), Type::getDoubleTy(context));
    FunctionType *funcType = FunctionType::get(Type::getVoidTy(context), paramTypes, false);

    Function *function = Function::Create(funcType, Function::ExternalLinkage, name, *module);

    BasicBlock *prevInsertBlock = builder.GetInsertBlock();
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);

    auto argIt = function->arg_begin();
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        Value *paramVal = &*argIt;
        paramVal->setName(parameters[i]);

        AllocaInst *alloca = builder.CreateAlloca(builder.getDoubleTy(), nullptr, parameters[i]);
        builder.CreateStore(paramVal, alloca);
        globalSymbolTable->setSymbol(parameters[i], alloca, builder.getDoubleTy());

        ++argIt;
    }

    for (auto *stmt : statementsBlock)
    {
        stmt->codegen();
    }

    builder.CreateRetVoid();
    builder.SetInsertPoint(prevInsertBlock);
    return function;
}

Value *FuncAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    std::vector<Type *> paramTypes(parameters.size(), Type::getDoubleTy(context));
    Type *returnType = Type::getDoubleTy(context); // Functions return double by default
    FunctionType *funcType = FunctionType::get(returnType, paramTypes, false);

    Function *function = Function::Create(funcType, Function::ExternalLinkage, name, *module);

    BasicBlock *prevInsertBlock = builder.GetInsertBlock();
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);

    auto argIt = function->arg_begin();
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        Value *paramVal = &*argIt;
        paramVal->setName(parameters[i]);

        AllocaInst *alloca = builder.CreateAlloca(builder.getDoubleTy(), nullptr, parameters[i]);
        builder.CreateStore(paramVal, alloca);
        globalSymbolTable->setSymbol(parameters[i], alloca, builder.getDoubleTy());

        ++argIt;
    }

    Value *returnValue = nullptr;
    for (auto *stmt : statementsBlock)
    {
        returnValue = stmt->codegen();
    }

    if (!returnValue)
    {
        returnValue = ConstantFP::get(Type::getDoubleTy(context), 0.0);
    }
    builder.CreateRet(returnValue);
    builder.SetInsertPoint(prevInsertBlock);
    return function;
}

Value *FuncCallAST::codegen()
{
    Function *callee = module->getFunction(name);
    if (!callee)
    {
        errs() << "Unknown function: " << name << "\n";
        return nullptr;
    }

    std::vector<Value *> args;
    for (auto *arg : arguments)
    {
        args.push_back(arg->codegen());
    }

    return builder.CreateCall(callee, args);
}

Value *DeclarationAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    errs() << "Generating code for declaration: " << identifier << " of type " << type->type << "\n";

    // Create an alloca instruction for the variable
    AllocaInst *alloca = builder.CreateAlloca(builder.getDoubleTy(), nullptr, identifier);

    // Initialize the variable with a default value based on its type
    Value *defaultValue;
    if (type->type == "int" || type->type == "INTEGER")
    {
        defaultValue = ConstantInt::get(Type::getInt32Ty(context), 0);
    }
    else if (type->type == "real" || type->type == "REAL")
    {
        defaultValue = ConstantFP::get(Type::getDoubleTy(context), 0.0);
    }
    else if (type->type == "boolean" || type->type == "BOOLEAN")
    {
        defaultValue = ConstantInt::get(Type::getInt1Ty(context), 0);
    }
    else if (type->type == "char" || type->type == "CHAR")
    {
        defaultValue = ConstantInt::get(Type::getInt8Ty(context), 0);
    }
    else if (type->type == "string" || type->type == "STRING")
    {
        defaultValue = ConstantPointerNull::get(PointerType::get(Type::getInt8Ty(context), 0));
    }
    else if (type->type == "date" || type->type == "DATE")
    {
        defaultValue = ConstantPointerNull::get(PointerType::get(Type::getInt8Ty(context), 0));
    }
    else
    {
        defaultValue = ConstantFP::get(Type::getDoubleTy(context), 0.0);
    }

    // Store the default value in the alloca
    builder.CreateStore(defaultValue, alloca);

    // Store the alloca in the symbol table
    globalSymbolTable->setSymbol(identifier, alloca, builder.getDoubleTy());
    errs() << "Declaration codegen completed for: " << identifier << "\n";
    return alloca;
}

Value *NumberAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    return createDoubleConstant(value);
}

Value *LogicalOpAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *lhsVal = lhs->codegen();
    Value *rhsVal = rhs->codegen();

    if (op == "and")
    {
        return builder.CreateAnd(lhsVal, rhsVal, "andtmp");
    }
    else if (op == "or")
    {
        return builder.CreateOr(lhsVal, rhsVal, "ortmp");
    }
    else
    {
        errs() << "Unknown logical operator: " << op << "\n";
        return nullptr;
    }
}

Value *StatementBlockAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *lastValue = nullptr;
    for (auto *stmt : statements)
    {
        if (stmt)
        {
            lastValue = stmt->codegen();
        }
    }
    return lastValue;
}

Value *BooleanLiteralAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    return ConstantInt::get(Type::getInt1Ty(context), value);
}

Value *ReturnAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *returnValue = expression->codegen();
    builder.CreateRet(returnValue);
    return returnValue;
}