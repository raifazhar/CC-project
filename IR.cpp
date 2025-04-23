#include "IR.h"

// std::map<std::string, Value *> SymbolTable;
LLVMContext context;
Module *module = nullptr;
IRBuilder<> builder(context);
Function *mainFunction = nullptr;

void initLLVM()
{
    // errs() << "Intilize LLVM!\n";
    module = new Module("top", context);
    FunctionType *mainTy = FunctionType::get(builder.getInt32Ty(), false);
    mainFunction = Function::Create(mainTy, Function::ExternalLinkage, "main", module);
    BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunction);
    builder.SetInsertPoint(entry);
}

void addReturnInstr()
{
    builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
}

Value *createDoubleConstant(double val)
{
    return ConstantFP::get(context, APFloat(val));
}

void printLLVMIR()
{
    module->print(errs(), nullptr);
}

Value *getFromSymbolTable(const std::string &id)
{
    Value *val = symtab.lookupSymbol(id);
    if (val)
        return val;

    // Symbol not found, create and store
    Value *defaultValue = builder.CreateAlloca(builder.getDoubleTy(), nullptr, id);
    symtab.setSymbol(id, defaultValue, builder.getDoubleTy());
    return defaultValue;
}

void setDouble(const std::string &id, Value *value)
{
    Value *ptr = getFromSymbolTable(id);
    builder.CreateStore(value, ptr);
    Type *doubleType = Type::getDoubleTy(context);

    // Value* loadedValue = builder.CreateLoad(elementType, ptr, "load_identifier");
}

void printfLLVM(const char *format, Value *inputValue)
{
    Function *printfFunc = module->getFunction("printf");
    if (!printfFunc)
    {
        FunctionType *printfTy = FunctionType::get(builder.getInt32Ty(), PointerType::get(builder.getInt8Ty(), 0), true);
        printfFunc = Function::Create(printfTy, Function::ExternalLinkage, "printf", module);
    }
    Value *formatVal = builder.CreateGlobalStringPtr(format);
    builder.CreateCall(printfFunc, {formatVal, inputValue}, "printfCall");
}

void printString(const std::string &str)
{
    Value *strValue = builder.CreateGlobalStringPtr(str);
    printfLLVM("%s", strValue);
}

void printDouble(Value *value)
{
    printfLLVM("%f\n", value);
}

Value *performBinaryOperation(Value *lhs, Value *rhs, char op)
{
    switch (op)
    {
    case '+':
        return builder.CreateFAdd(lhs, rhs, "fadd");
    case '-':
        return builder.CreateFSub(lhs, rhs, "fsub");
    case '*':
        return builder.CreateFMul(lhs, rhs, "fmul");
    case '/':
        return builder.CreateFDiv(lhs, rhs, "fdiv");
    default:
        yyerror("illegal binary operation");
        exit(EXIT_FAILURE);
    }
}

Value *performComparison(Value *lhs, Value *rhs, int op)
{
    switch (op)
    {
    case 1:
        return builder.CreateFCmpUGT(lhs, rhs, "fgreater");
    case 2:
        return builder.CreateFCmpULT(lhs, rhs, "fless");
    case 3:
        return builder.CreateFCmpUEQ(lhs, rhs, "fequal");
    case 5:
        return builder.CreateFCmpULE(lhs, rhs, "flessequal");
    case 4:
        return builder.CreateFCmpUGE(lhs, rhs, "fgreaterequal");
    case 6:
        return builder.CreateFCmpUNE(lhs, rhs, "fnotequal");
    default:
        yyerror("illegal comparison operation");
        exit(EXIT_FAILURE);
    }
}

void yyerror(const char *s)
{
    fprintf(stderr, "Parse error: %s\n", s);
}