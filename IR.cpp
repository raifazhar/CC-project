#include "IR.h"

extern char *yytext;
extern int yylineno;

// Global symbol table instance
SymbolTable *globalSymbolTable = nullptr;

void initSymbolTable(CodegenContext &ctx)
{
    if (!globalSymbolTable)
    {
        globalSymbolTable = new SymbolTable(ctx);
    }
}

SymbolTable *getSymbolTable()
{
    return globalSymbolTable;
}

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

    // Initialize symbol table with the current context
    CodegenContext ctx(context, module, builder, mainFunction);
    initSymbolTable(ctx);
}

void addReturnInstr()
{
    fprintf(stderr, "Adding return instruction\n");
    if (!builder.GetInsertBlock())
    {
        fprintf(stderr, "Error: No insertion point set for return instruction\n");
        return;
    }
    builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
    fprintf(stderr, "Return instruction added successfully\n");
}

Value *createDoubleConstant(double val)
{
    return ConstantFP::get(context, APFloat(val));
}

void printLLVMIR()
{
    fprintf(stderr, "Printing LLVM IR\n");
    if (module == nullptr)
    {
        fprintf(stderr, "Error: Module is null\n");
        return;
    }

    // Check if the module has any functions
    if (module->empty())
    {
        fprintf(stderr, "Warning: Module has no functions\n");
    }
    else
    {
        fprintf(stderr, "Module has %zu functions\n", module->size());
    }

    // Check if the main function has any basic blocks
    if (mainFunction == nullptr)
    {
        fprintf(stderr, "Error: Main function is null\n");
    }
    else if (mainFunction->empty())
    {
        fprintf(stderr, "Warning: Main function has no basic blocks\n");
    }
    else
    {
        fprintf(stderr, "Main function has %zu basic blocks\n", mainFunction->size());

        // Check if the entry block has any instructions
        BasicBlock *entry = &mainFunction->getEntryBlock();
        if (entry->empty())
        {
            fprintf(stderr, "Warning: Entry block has no instructions\n");
        }
        else
        {
            fprintf(stderr, "Entry block has %zu instructions\n", entry->size());
        }
    }

    // Print the module to stdout for the output file
    // Use a single print to avoid duplicates
    module->print(outs(), nullptr);
}

Value *getFromSymbolTable(const std::string &id)
{
    Value *val = globalSymbolTable->lookupSymbol(id);
    if (val)
        return val;

    // Symbol not found, create and store
    Value *defaultValue = builder.CreateAlloca(builder.getDoubleTy(), nullptr, id);
    globalSymbolTable->setSymbol(id, defaultValue, builder.getDoubleTy());
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

Value *performBinaryOperation(Value *lhs, Value *rhs, const std::string &op)
{
    Type *type = lhs->getType();

    // Binary operation: Check operand types
    if (!rhs && op != "++" && op != "--")
    {
        yyerror(("Missing right-hand operand for binary operator: " + op).c_str());
        exit(EXIT_FAILURE);
    }

    if (type->isIntegerTy())
    {
        if (op == "++")
            return builder.CreateAdd(lhs, ConstantInt::get(type, 1), "increment");
        if (op == "--")
            return builder.CreateSub(lhs, ConstantInt::get(type, 1), "decrement");
        if (op == "+")
            return builder.CreateAdd(lhs, rhs, "addtmp");
        if (op == "-")
            return builder.CreateSub(lhs, rhs, "subtmp");
        if (op == "*")
            return builder.CreateMul(lhs, rhs, "multmp");
        if (op == "/")
            return builder.CreateSDiv(lhs, rhs, "divtmp");
    }
    else if (type->isFloatingPointTy())
    {
        if (op == "++")
            return builder.CreateFAdd(lhs, ConstantFP::get(type, 1.0), "fincrement");
        if (op == "--")
            return builder.CreateFSub(lhs, ConstantFP::get(type, 1.0), "fdecrement");
        if (op == "+")
            return builder.CreateFAdd(lhs, rhs, "faddtmp");
        if (op == "-")
            return builder.CreateFSub(lhs, rhs, "fsubtmp");
        if (op == "*")
            return builder.CreateFMul(lhs, rhs, "fmultmp");
        if (op == "/")
            return builder.CreateFDiv(lhs, rhs, "fdivtmp");
    }

    yyerror(("Unsupported or illegal operator: " + op).c_str());
    exit(EXIT_FAILURE);
}

Value *performComparison(Value *lhs, Value *rhs, const std::string &op)
{
    // Floating-point comparison
    if (lhs->getType()->isFloatingPointTy() && rhs->getType()->isFloatingPointTy())
    {
        if (op == "<")
            return builder.CreateFCmpULT(lhs, rhs, "fless");
        if (op == ">")
            return builder.CreateFCmpUGT(lhs, rhs, "fgreater");
        if (op == "==")
            return builder.CreateFCmpUEQ(lhs, rhs, "fequal");
        if (op == "<=")
            return builder.CreateFCmpULE(lhs, rhs, "flessequal");
        if (op == ">=")
            return builder.CreateFCmpUGE(lhs, rhs, "fgreaterequal");
        if (op == "!=")
            return builder.CreateFCmpUNE(lhs, rhs, "fnotequal");

        yyerror("illegal floating-point comparison operation");
        exit(EXIT_FAILURE);
    }
    // Integer comparison
    else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy())
    {
        if (op == "<")
            return builder.CreateICmpSLT(lhs, rhs, "iless");
        if (op == ">")
            return builder.CreateICmpSGT(lhs, rhs, "igreater");
        if (op == "==")
            return builder.CreateICmpEQ(lhs, rhs, "iequal");
        if (op == "<=")
            return builder.CreateICmpSLE(lhs, rhs, "ilessequal");
        if (op == ">=")
            return builder.CreateICmpSGE(lhs, rhs, "igreaterequal");
        if (op == "!=")
            return builder.CreateICmpNE(lhs, rhs, "inotequal");

        yyerror("illegal integer comparison operation");
        exit(EXIT_FAILURE);
    }
    else
    {
        yyerror("comparison between incompatible types");
        exit(EXIT_FAILURE);
    }
}

void yyerror(const char *s)
{
    fprintf(stderr, "\nParse error at line %d: %s\n", yylineno, s);
    fprintf(stderr, "Current token: %s\n", yytext);
    fprintf(stderr, "Current line context: ");

    // Print context around the error
    for (int i = 0; i < strlen(yytext); i++)
    {
        fprintf(stderr, "%c", yytext[i]);
    }
    fprintf(stderr, "\n");

    // Print indentation state
    fprintf(stderr, "Current indentation level: %d\n", current_indent);
    fprintf(stderr, "Indentation stack size: %zu\n", indent_stack.size());
    fprintf(stderr, "Start of line: %s\n", start_of_line ? "true" : "false");
    fprintf(stderr, "Dedent buffer size: %zu\n\n", dedent_buffer.size());
}