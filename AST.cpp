#include "AST.h"
#include "IR.h"
#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <unordered_map>
#include <iostream> // Add this for complete std::ofstream definition

#define DEBUG_PRINT_FUNCTION()                                                               \
    do                                                                                       \
    {                                                                                        \
        const char *filename = "build/debug/llvm_debug.output.txt";                          \
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

const std::unordered_map<std::string, TypeAST::TypeGenerator> TypeAST::typeMap = {
    {"INTEGER", [](llvm::LLVMContext &ctx)
     {
         return llvm::Type::getInt32Ty(ctx);
     }},
    {"REAL", [](llvm::LLVMContext &ctx)
     {
         return llvm::Type::getDoubleTy(ctx);
     }},
    {"STRING", [](llvm::LLVMContext &ctx)
     {
         return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(ctx));
     }},
    {"CHAR", [](llvm::LLVMContext &ctx)
     {
         return llvm::Type::getInt8Ty(ctx);
     }},
    {"BOOLEAN", [](llvm::LLVMContext &ctx)
     {
         return llvm::Type::getInt1Ty(ctx);
     }},
    {"DATE", [](llvm::LLVMContext &ctx)
     {
         return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(ctx)); // or however you represent it
     }},
};

Value *TypeAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    return nullptr;
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

    // 1. Get the variable from the symbol table
    Value *var = globalSymbolTable->lookupSymbol(identifier);
    if (!var)
    {
        llvm::errs() << "Unknown variable: " << identifier << "\n";
        return nullptr;
    }

    // 2. Get the type of the variable from the symbol table
    llvm::Type *varType = globalSymbolTable->getSymbolType(identifier);
    if (!varType)
    {
        llvm::errs() << "Failed to get type for variable: " << identifier << "\n";
        return nullptr;
    }

    // 3. Generate code for the value being assigned
    Value *val = expression->codegen();
    if (!val)
    {
        return nullptr;
    }

    // 4. Type check: Compare the type of the variable with the type of the value being assigned
    llvm::Type *valType = val->getType();
    llvm::errs() << "Variable type: " << *varType << " Value type: " << *valType << "\n";

    // If the types don't match, report an error
    if (varType->getTypeID() != valType->getTypeID())
    {
        llvm::outs() << "Type mismatch: Cannot assign " << *valType << " to " << *varType << "\n";
        return nullptr;
    }

    // 5. Debug output to check types (optional, for debugging purposes)
    llvm::errs() << "Variable type: " << *varType << "\n";
    llvm::errs() << "Value type: " << *valType << "\n";

    // 6. Store the value in the variable
    builder.CreateStore(val, var);

    return val;
}

Value *BinaryOpAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    errs() << "I am being called";
    // Handle ++ / -- (postfix): expression2 is null
    if (expression2 == nullptr && (op == '+' || op == '-'))
    {
        // Since we're handling postfix operations, expression1 must be an identifier
        if (!isa<IdentifierAST>(expression1))
        {
            errs() << "Error: Increment/Decrement only valid on variables.\n";
            return nullptr;
        }

        auto var = static_cast<IdentifierAST *>(expression1);
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

        return oldVal; // Return old value for post-inc semantics
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

Value *PrintRealLiteralAST::codegen()
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
    DEBUG_PRINT_FUNCTION();

    Function *function = builder.GetInsertBlock()->getParent();

    // Create basic blocks
    BasicBlock *condBB = BasicBlock::Create(context, "for.cond", function);
    BasicBlock *loopBB = BasicBlock::Create(context, "for.body", function);
    BasicBlock *afterBB = BasicBlock::Create(context, "for.end", function);

    // Initialize loop variable
    if (assignment)
    {
        assignment->codegen();
    }

    // Branch to condition block
    builder.CreateBr(condBB);

    // Generate condition block
    builder.SetInsertPoint(condBB);
    AssignmentAST *assignmentNode = (AssignmentAST *)assignment;
    Value *loopVar = getFromSymbolTable(assignmentNode->identifier);
    Value *currentValue = builder.CreateLoad(Type::getDoubleTy(context), loopVar, "current");
    Value *endValue = condition->codegen();
    Value *condVal = builder.CreateFCmpOLE(currentValue, endValue, "loopcond");
    builder.CreateCondBr(condVal, loopBB, afterBB);

    // Generate loop body
    builder.SetInsertPoint(loopBB);
    for (auto *stmt : forBlock)
    {
        stmt->codegen();
    }

    // Update loop variable using the step expression
    currentValue = builder.CreateLoad(Type::getDoubleTy(context), loopVar, "current");
    // Use the step expression from the AST
    if (expression)
    {
        // Generate code for the step expression and add it to current value
        Value *stepValue = expression->codegen();
        Value *nextValue = builder.CreateFAdd(currentValue, stepValue, "next");
        builder.CreateStore(nextValue, loopVar);
    }
    else
    {
        // Default increment by 1 if no step was provided
        Value *one = ConstantFP::get(Type::getDoubleTy(context), 1.0);
        Value *nextValue = builder.CreateFAdd(currentValue, one, "next");
        builder.CreateStore(nextValue, loopVar);
    }

    // Branch back to condition
    builder.CreateBr(condBB);

    // Move to after block
    builder.SetInsertPoint(afterBB);
    return nullptr;
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
    llvm::errs() << "Generating code for declaration: " << identifier << " of type " << type->type << "\n";

    // Get the default value for this type (e.g., 0 for integer types)
    CodegenContext ctx(context, module, builder, builder.GetInsertBlock()->getParent());
    llvm::Type *varType = TypeAST::typeMap.at(type->type)(context);

    AllocaInst *alloca = builder.CreateAlloca(varType, nullptr, identifier);

    globalSymbolTable->setSymbol(identifier, alloca, varType);

    llvm::errs() << "Declaration codegen completed for: " << identifier << "\n";
    return alloca;
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
