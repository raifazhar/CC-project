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
Value *IdentifierAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *ptr = globalSymbolTable->lookupSymbol(name);
    if (!ptr)
    {
        errs() << "Error: Symbol not found for " << name << "\n";
        return nullptr;
    }

    if (isa<AllocaInst>(ptr))
    {
        auto *allocaInst = dyn_cast<AllocaInst>(ptr);
        Type *elementType = allocaInst->getAllocatedType();
        return builder.CreateLoad(elementType, ptr, "loaded_" + name);
    }
    else if (isa<GlobalVariable>(ptr))
    {
        auto *globalVar = dyn_cast<GlobalVariable>(ptr);
        Type *elementType = globalVar->getValueType();
        return builder.CreateLoad(elementType, ptr, "loaded_" + name);
    }
    else if (isa<Argument>(ptr))
    {
        return ptr;
    }
    else
    {
        errs() << "Unexpected pointer type for " << name << "\n";
        return nullptr;
    }
}

Value *DeclarationAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    llvm::errs() << "Generating code for declaration: " << identifier->name << " of type " << type->type << "\n";

    // Get the default value for this type (e.g., 0 for integer types)
    llvm::Type *varType = TypeAST::typeMap.at(type->type)(context);

    AllocaInst *alloca = builder.CreateAlloca(varType, nullptr, identifier->name);

    globalSymbolTable->setSymbol(identifier->name, alloca, varType);

    llvm::errs() << "Declaration codegen completed for: " << identifier->name << "\n";
    return alloca;
}

Value *ArrayAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    llvm::errs() << "Generating code for declaration: " << identifier->name << " of type " << type->type << "\n";

    // Get the default value for this type (e.g., 0 for integer types)
    llvm::Type *varType = TypeAST::typeMap.at(type->type)(context);
    Value *arraySize = ConstantInt::get(Type::getInt32Ty(context), size);
    AllocaInst *alloca = builder.CreateAlloca(varType, arraySize, identifier->name);

    globalSymbolTable->setSymbol(identifier->name, alloca, varType, true, 0, size - 1);

    llvm::errs() << "Declaration codegen completed for: " << identifier->name << "\n";
    return alloca;
}

Value *AssignmentAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    // 1. Look up the variable pointer (not load its value)
    Value *varPtr = globalSymbolTable->lookupSymbol(identifier->name);
    if (!varPtr)
    {
        llvm::errs() << "Unknown variable: " << identifier->name << "\n";
        return nullptr;
    }

    // 2. Get the type of the variable from the symbol table
    llvm::Type *varType = globalSymbolTable->getSymbolType(identifier->name);
    if (!varType)
    {
        llvm::errs() << "Failed to get type for variable: " << identifier->name << "\n";
        return nullptr;
    }

    // 3. Generate code for the value being assigned
    Value *val = expression->codegen();
    if (!val)
    {
        return nullptr;
    }

    // 4. Type check
    llvm::Type *valType = val->getType();
    if (varType->getTypeID() != valType->getTypeID())
    {
        llvm::outs() << "Type mismatch: Cannot assign " << *valType << " to " << *varType << "\n";
        return nullptr;
    }

    // 5. Store the value in the variable pointer
    builder.CreateStore(val, varPtr);

    return val;
}

Value *ArrayAssignmentAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    // 1. Generate code for index
    Value *indexValue = index->codegen();
    if (!indexValue)
    {
        llvm::errs() << "Failed to generate index for array: " << identifier->name << "\n";
        return nullptr;
    }

    // 2. Look up the pointer to the array element
    Value *elementPtr = globalSymbolTable->lookupSymbol(identifier->name, indexValue);
    if (!elementPtr)
    {
        llvm::errs() << "Unknown array or invalid access: " << identifier->name << "\n";
        return nullptr;
    }

    // 3. Generate code for the value being assigned
    Value *val = expression->codegen();
    if (!val)
    {
        return nullptr;
    }

    // 4. Type check: Make sure the value matches the array element type
    llvm::Type *expectedType = globalSymbolTable->getSymbolType(identifier->name);
    llvm::Type *valType = val->getType();
    if (expectedType->getTypeID() != valType->getTypeID())
    {
        llvm::errs() << "Type mismatch: Cannot assign " << *valType << " to " << *expectedType << "\n";
        return nullptr;
    }

    // 5. Store the value into the array element
    builder.CreateStore(val, elementPtr);

    return val;
}

Value *OutputAST::codegen()
{
    // ————————————————————————————————————————————————
    // 1) Make sure the global 'builder' is pointing somewhere valid
    // ————————————————————————————————————————————————
    if (!builder.GetInsertBlock())
    {
        // If we haven't yet set an insertion point, drop into the
        // entry block of 'mainFunction' (or whatever Function you want).
        BasicBlock &entry = mainFunction->getEntryBlock();
        builder.SetInsertPoint(&entry, entry.end());
    }

    // From here on we can just use 'module', 'builder', and 'mainFunction'
    // that were declared extern in your driver.
    // ————————————————————————————————————————————————
    // 2) Declare-or-fetch printf
    // ————————————————————————————————————————————————
    Function *printfFunc = module->getFunction("printf");
    if (!printfFunc)
    {
        auto *printfTy = FunctionType::get(
            builder.getInt32Ty(),                     // int
            PointerType::get(builder.getInt8Ty(), 0), // char*
            /*isVarArg=*/true);
        printfFunc = Function::Create(
            printfTy,
            Function::ExternalLinkage,
            "printf",
            module);
    }

    // ————————————————————————————————————————————————
    // 3) Build the format string and argument list
    // ————————————————————————————————————————————————
    std::string fmt;
    std::vector<Value *> args;
    for (auto &exp : expressions)
    {
        Value *v = exp->codegen();
        if (!v)
            return nullptr; // real error

        if (v->getType()->isIntegerTy())
            fmt += "%d";
        else if (v->getType()->isDoubleTy())
            fmt += "%f";
        else if (v->getType()->isPointerTy())
            fmt += "%s";

        args.push_back(v);
    }

    // ————————————————————————————————————————————————
    // 4) Make a global format-string constant
    // ————————————————————————————————————————————————
    Constant *fmtConst = ConstantDataArray::getString(
        context, fmt + "\n", /*addNull=*/true);
    auto *fmtGV = new GlobalVariable(
        *module,
        fmtConst->getType(),
        /*isConstant=*/true,
        GlobalValue::PrivateLinkage,
        fmtConst);
    Value *fmtPtr = builder.CreateConstGEP1_32(
        fmtGV->getValueType(), fmtGV, 0);

    // ————————————————————————————————————————————————
    // 5) Emit the call, capture it, and return it
    // ————————————————————————————————————————————————
    args.insert(args.begin(), fmtPtr);
    CallInst *call = builder.CreateCall(printfFunc, args);
    return call;
}
Value *InputAST::codegen()
{
    if (!builder.GetInsertBlock())
    {
        BasicBlock &entry = mainFunction->getEntryBlock();
        builder.SetInsertPoint(&entry, entry.end());
    }

    Value *varPtr = globalSymbolTable->lookupSymbol(Identifier->name);
    Type *varType = globalSymbolTable->getSymbolType(Identifier->name);

    if (!varPtr || !varType)
    {
        errs() << "Error: undeclared variable '" << Identifier->name << "'\n";
        return nullptr;
    }

    FunctionCallee scanfFunc = module->getOrInsertFunction(
        "scanf",
        FunctionType::get(
            builder.getInt32Ty(),
            PointerType::getUnqual(builder.getInt8Ty()),
            true));

    std::string fmt;
    if (varType->isIntegerTy(32))
        fmt = "%d";
    else if (varType->isDoubleTy())
        fmt = "%lf";
    else
    {
        errs() << "Error: unsupported input type for '" << Identifier->name << "'\n";
        return nullptr;
    }

    Value *fmtPtr = builder.CreateGlobalStringPtr(fmt, ".fmt_" + Identifier->name);

    builder.CreateCall(scanfFunc, {fmtPtr, varPtr});

    return nullptr;
}

Value *BinaryOpAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    // Binary operation
    Value *lhs = expression1->codegen();
    Value *rhs = nullptr;
    if (expression2)
        rhs = expression2->codegen();
    return performBinaryOperation(lhs, rhs, op);
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
    globalSymbolTable->enterScope();
    Value *condVal = condition->codegen();
    Function *function = builder.GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(context, "if.then", function);
    BasicBlock *elseBB = BasicBlock::Create(context, "if.else", function);
    BasicBlock *mergeBB = BasicBlock::Create(context, "if.end", function);

    builder.CreateCondBr(condVal, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);

    thenBlock->codegen();

    builder.CreateBr(mergeBB);

    builder.SetInsertPoint(elseBB);

    if (elseBlock)
        elseBlock->codegen();

    builder.CreateBr(mergeBB);

    builder.SetInsertPoint(mergeBB);
    globalSymbolTable->exitScope();
    return nullptr;
}

Value *ForAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    globalSymbolTable->enterScope();
    Function *function = builder.GetInsertBlock()->getParent();

    // Create basic blocks
    BasicBlock *condBB = BasicBlock::Create(context, "for.cond", function);
    BasicBlock *loopBB = BasicBlock::Create(context, "for.body", function);
    BasicBlock *afterBB = BasicBlock::Create(context, "for.end", function);

    // Initialize loop variable (e.g., INDEX = 1)
    if (assignment)
        assignment->codegen();

    // Jump to condition block
    builder.CreateBr(condBB);

    // Condition block
    builder.SetInsertPoint(condBB);
    Value *condValue = condition->codegen(); // Should return i1 from ComparisonAST
    builder.CreateCondBr(condValue, loopBB, afterBB);

    // Loop body
    builder.SetInsertPoint(loopBB);
    forBlock->codegen();

    // Step update
    step->codegen(); // Generates and stores the increment/decrement
    // Loop back
    builder.CreateBr(condBB);

    // After loop
    builder.SetInsertPoint(afterBB);
    globalSymbolTable->exitScope();
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
    globalSymbolTable->enterScope();
    // Create the function type based on parameters
    std::vector<Type *> paramTypes;
    for (auto &param : parameters)
        paramTypes.push_back(TypeAST::typeMap.at(param->type->type)(context));

    FunctionType *funcType = FunctionType::get(Type::getVoidTy(context), paramTypes, false);
    Function *function = Function::Create(funcType, Function::ExternalLinkage, Identifier->name, *module);

    BasicBlock *prevInsertBlock = builder.GetInsertBlock();
    // Create the entry block for the function
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);

    // Allocate space for each parameter and store it
    auto argIt = function->arg_begin();
    for (auto &param : parameters)
    {
        Value *paramVal = &*argIt;
        paramVal->setName(param->name);

        llvm::Type *varType = TypeAST::typeMap.at(param->type->type)(context);
        AllocaInst *alloca = builder.CreateAlloca(varType, nullptr, param->name);
        builder.CreateStore(paramVal, alloca);
        globalSymbolTable->setSymbol(param->name, alloca, varType);

        ++argIt;
    }

    // Generate code for the statements in the block
    statementsBlock->codegen();

    // Return void
    builder.CreateRetVoid();
    globalSymbolTable->enterScope();
    builder.SetInsertPoint(prevInsertBlock);
    return function;
}

Value *FuncAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    globalSymbolTable->enterScope();
    // Create the function type based on parameters
    std::vector<Type *> paramTypes;
    for (auto &param : parameters)
        paramTypes.push_back(TypeAST::typeMap.at(param->type->type)(context));

    llvm::Type *FuncType = TypeAST::typeMap.at(returnType->type)(context);
    FunctionType *funcType = FunctionType::get(FuncType, paramTypes, false);
    Function *function = Function::Create(funcType, Function::ExternalLinkage, Identifier->name, *module);

    BasicBlock *prevInsertBlock = builder.GetInsertBlock();
    // Create the entry block for the function
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBB);

    // Allocate space for each parameter and store it
    auto argIt = function->arg_begin();
    for (auto &param : parameters)
    {
        Value *paramVal = &*argIt;
        paramVal->setName(param->name);

        llvm::Type *varType = TypeAST::typeMap.at(param->type->type)(context);
        AllocaInst *alloca = builder.CreateAlloca(varType, nullptr, param->name);
        builder.CreateStore(paramVal, alloca);
        globalSymbolTable->setSymbol(param->name, alloca, varType);

        ++argIt;
    }

    // Generate code for the statements in the block
    statementsBlock->codegen();

    // Return void
    globalSymbolTable->enterScope();
    builder.SetInsertPoint(prevInsertBlock);
    return function;
}

Value *ReturnAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    Value *returnValue = expression->codegen();
    builder.CreateRet(returnValue);
    return returnValue;
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
