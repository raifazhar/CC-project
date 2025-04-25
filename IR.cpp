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

Value *performBinaryOperation(Value *lhs, Value *rhs, char op)
{
    Type *lhsType = lhs->getType();

    if (lhsType->isIntegerTy())
    {
        switch (op)
        {
        case '+':
            return builder.CreateAdd(lhs, rhs, "addtmp");
        case '-':
            return builder.CreateSub(lhs, rhs, "subtmp");
        case '*':
            return builder.CreateMul(lhs, rhs, "multmp");
        case '/':
            return builder.CreateSDiv(lhs, rhs, "divtmp"); // signed division
        default:
            yyerror("illegal binary operation for integer");
            exit(EXIT_FAILURE);
        }
    }
    else if (lhsType->isFloatingPointTy())
    {
        switch (op)
        {
        case '+':
            return builder.CreateFAdd(lhs, rhs, "faddtmp");
        case '-':
            return builder.CreateFSub(lhs, rhs, "fsubtmp");
        case '*':
            return builder.CreateFMul(lhs, rhs, "fmultmp");
        case '/':
            return builder.CreateFDiv(lhs, rhs, "fdivtmp");
        default:
            yyerror("illegal binary operation for float");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        yyerror("unsupported operand type");
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