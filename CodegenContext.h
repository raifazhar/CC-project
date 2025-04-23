#ifndef CODEGEN_CONTEXT_H
#define CODEGEN_CONTEXT_H

#include "common_includes.h"

class CodegenContext {
public:
    LLVMContext& context;
    Module* module;
    IRBuilder<>& builder;
    Function* currentFunction;

    CodegenContext(LLVMContext& ctx, Module* mod, IRBuilder<>& b, Function* func)
        : context(ctx), module(mod), builder(b), currentFunction(func) {}
};

#endif // CODEGEN_CONTEXT_H 