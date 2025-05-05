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

Value *AssignmentAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    // 1. Look up the variable pointer (not load its value)
    Value *varPtr = globalSymbolTable->lookupSymbol(identifier->name);
    if (!varPtr)
    {
        llvm::outs() << "Unknown variable: " << identifier->name << "\n";
        return nullptr;
    }

    // 2. Get the type of the variable from the symbol table
    llvm::Type *varType = globalSymbolTable->getSymbolType(identifier->name);
    if (!varType)
    {
        llvm::outs() << "Failed to get type for variable: " << identifier->name << "\n";
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

Value *ArrayAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    llvm::errs() << "Generating code for declaration: " << identifier->name << " of type " << type->type << "\n";

    // 1. Get the element type (like i32)
    llvm::Type *elementType = TypeAST::typeMap.at(type->type)(context);

    // 2. Create the array type [size x elementType]
    llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, lastIndex);

    // 3. Create an alloca of the array type
    AllocaInst *alloca = builder.CreateAlloca(arrayType, nullptr, identifier->name);

    // 4. Set it in the symbol table
    globalSymbolTable->setSymbol(identifier->name, alloca, arrayType, true, firstIndex, lastIndex - 1);

    llvm::errs() << "Declaration codegen completed for: " << identifier->name << "\n";
    return alloca;
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

    // 4. Type check: match **element type**, not array type
    llvm::Type *expectedArrayType = globalSymbolTable->getSymbolType(identifier->name);

    llvm::Type *expectedElementType = nullptr;
    if (expectedArrayType->isArrayTy())
    {
        expectedElementType = expectedArrayType->getArrayElementType();
    }
    else
    {
        expectedElementType = expectedArrayType; // fallback
    }

    llvm::Type *valType = val->getType();
    if (expectedElementType->getTypeID() != valType->getTypeID())
    {
        llvm::errs() << "Type mismatch: Cannot assign " << *valType << " to element type " << *expectedElementType << "\n";
        return nullptr;
    }

    // 5. Store the value into the array element
    builder.CreateStore(val, elementPtr);

    return val;
}

Value *ArrayAccessAST::codegen()
{
    DEBUG_PRINT_FUNCTION();

    // 1. Get the index value (like 'i' in arr[i])
    Value *indexValue = index->codegen();
    if (!indexValue)
    {
        llvm::errs() << "Invalid index expression\n";
        return nullptr;
    }

    // 2. Look up the pointer to the array element
    Value *elementPtr = globalSymbolTable->lookupSymbol(identifier->name, indexValue);
    if (!elementPtr)
    {
        llvm::errs() << "Array access failed: " << identifier->name << "\n";
        return nullptr;
    }

    // 3. Get the type of the element, not the array
    llvm::Type *arrayType = globalSymbolTable->getSymbolType(identifier->name);
    llvm::Type *elementType = arrayType->isArrayTy() ? arrayType->getArrayElementType() : arrayType;

    // 4. Load and return the value at that index
    Value *loadedValue = builder.CreateLoad(elementType, elementPtr);
    return loadedValue;
}

Value *OutputAST::codegen()
{
    if (!builder.GetInsertBlock())
    {
        BasicBlock &entry = mainFunction->getEntryBlock();
        builder.SetInsertPoint(&entry, entry.end());
    }
    Function *printfFunc = module->getFunction("printf");
    if (!printfFunc)
    {
        auto *printfTy = FunctionType::get(
            builder.getInt32Ty(),
            PointerType::get(builder.getInt8Ty(), 0),
            true);
        printfFunc = Function::Create(
            printfTy,
            Function::ExternalLinkage,
            "printf",
            module);
    }
    std::string fmt;
    std::vector<Value *> args;
    for (auto &exp : expressions)
    {
        Value *v = exp->codegen();
        if (!v)
            return nullptr;

        if (v->getType()->isIntegerTy(32))
            fmt += "%d";
        else if (v->getType()->isDoubleTy())
            fmt += "%f";
        else if (v->getType()->isIntegerTy(8))
            fmt += "%c";
        else if (v->getType()->isPointerTy())
            fmt += "%s";

        args.push_back(v);
    }

    Constant *fmtConst = ConstantDataArray::getString(
        context, fmt + "\n", true);
    auto *fmtGV = new GlobalVariable(
        *module,
        fmtConst->getType(), true,
        GlobalValue::PrivateLinkage,
        fmtConst);
    Value *fmtPtr = builder.CreateGlobalStringPtr(fmt, ".fmt");

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
    else if (varType->isIntegerTy(8))
        fmt = " %c";
    else if (varType->isPointerTy())
    {
        fmt = "%s";
    }
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
    globalSymbolTable->enterScope();
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
    condVal = builder.CreateICmpEQ(condVal, ConstantInt::get(Type::getInt1Ty(context), 0), "repeat_cond");
    builder.CreateCondBr(condVal, endBB, bodyBB);

    // Continue with end block
    builder.SetInsertPoint(endBB);
    globalSymbolTable->exitScope();
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
    globalSymbolTable->exitScope();
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
    globalSymbolTable->exitScope();
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

    if (callee->arg_size() != arguments.size())
    {
        errs() << "Function " << name << " called with incorrect number of arguments\n";
        return nullptr;
    }

    std::vector<Value *> args;
    auto argIt = callee->arg_begin();
    for (size_t i = 0; i < arguments.size(); ++i, ++argIt)
    {
        Value *argVal = arguments[i]->codegen();
        if (!argVal)
            return nullptr;

        if (argVal->getType() != argIt->getType())
        {
            errs() << "Type mismatch in argument " << i << " of function " << name << "\n";
            return nullptr;
        }

        args.push_back(argVal);
    }

    return builder.CreateCall(callee, args);
}

Value *LogicalOpAST::codegen()
{
    DEBUG_PRINT_FUNCTION();
    if (cmpOp == "NOT")
    {
        if (!RHS)
        {
            errs() << "NOT operation requires a right-hand side operand\n";
            return nullptr;
        }
        Value *rhsVal = RHS->codegen();
        if (!rhsVal)
        {
            return nullptr;
        }
        if (!rhsVal->getType()->isIntegerTy(1))
        {
            rhsVal = builder.CreateICmpEQ(rhsVal, ConstantInt::get(rhsVal->getType(), 0), "tobool");
        }
        return builder.CreateNot(rhsVal, "nottmp");
    }
    if (!LHS || !RHS)
    {
        errs() << "Error: NULL operand for binary logical operator: " << cmpOp << "\n";
        return nullptr;
    }
    Value *lhsVal = LHS->codegen();
    Value *rhsVal = RHS->codegen();
    if (!lhsVal || !rhsVal)
        return nullptr;

    if (!lhsVal->getType()->isIntegerTy(1))
    {
        lhsVal = builder.CreateICmpNE(
            lhsVal,
            ConstantInt::get(lhsVal->getType(), 0),
            "tobool");
    }

    if (!rhsVal->getType()->isIntegerTy(1))
    {
        rhsVal = builder.CreateICmpNE(
            rhsVal,
            ConstantInt::get(rhsVal->getType(), 0),
            "tobool");
    }
    if (cmpOp == "AND")
    {
        return builder.CreateAnd(lhsVal, rhsVal, "andtmp");
    }
    else if (cmpOp == "OR")
    {
        return builder.CreateOr(lhsVal, rhsVal, "ortmp");
    }
    else
    {
        errs() << "Unknown logical operator: " << cmpOp << "\n";
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
