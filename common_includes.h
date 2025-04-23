#ifndef COMMON_INCLUDES_H
#define COMMON_INCLUDES_H

// Include LLVM libraries
#include <llvm/IR/Value.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/IR/Verifier.h"

// C++ standard libraries
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <utility>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstddef> // for size_t
#include <string>  // if you're using std::string
#include <vector>  // if you're using std::vector
#include <memory>  // if you're using smart pointers

using namespace llvm;

// Declare common externs and global variables
extern LLVMContext context;
extern Module *module;
extern IRBuilder<> builder;
extern Function *mainFunction;

#endif // COMMON_INCLUDES_H
