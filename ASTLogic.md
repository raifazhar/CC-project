# Abstract Syntax Tree (AST) Code Generation Documentation

This documentation explains the structure and LLVM IR code generation behavior for each AST node in the pseudocode compiler.

## Type Mapping (`TypeAST`)

The `TypeAST::typeMap` defines mappings from high-level types (e.g., `INTEGER`, `REAL`) to LLVM types:

| Pseudocode Type | LLVM Type               |
| --------------- | ----------------------- |
| INTEGER         | `i32`                   |
| REAL            | `double`                |
| STRING          | `i8*` (null-terminated) |
| CHAR            | `i8`                    |
| BOOLEAN         | `i1`                    |
| DATE            | `i8*`                   |

---

## AST Node Code Generation

### `IdentifierAST`

Looks up a variable in the symbol table using `globalSymbolTable->lookupSymbol(name)` and returns a `load` instruction unless it’s a function argument (already a pointer).

### `DeclarationAST`

Declares a scalar variable:

- Get the amout of space need to allocate using `TypeAST::typeMap.at(type->type)(context)` in which type is passed
- Allocates memory using ` builder.CreateAlloca(varType, nullptr, identifier->name)`

- Registers the variable in the symbol table using ` globalSymbolTable->declareSymbol(identifier->name, alloca, varType)`

### `AssignmentAST`

Assigns a value to a variable:

- Looks up pointer using `globalSymbolTable->lookupSymbol(identifier->name)`

- Type checks value against the declared type `globalSymbolTable->getSymbolType(identifier->name)`

- Stores the result with ` builder.CreateStore(val, varPtr)`

### `ArrayAST`

Declares a static-sized array:

- Allocates space for `[N x Type]` using `ArrayType::get(elementType, lastIndex)`

- Uses `builder.CreateAlloca(arrayType, nullptr, identifier->name)` for storage

- Stores metadata in the symbol table using `globalSymbolTable->declareSymbol(identifier->name, alloca, arrayType, true, firstIndex, lastIndex - 1)`

### `ArrayAssignmentAST`

Assigns a value to an array element:

- Computes index with `CreateGEP(...)` in the symboltable.cpp

- Looks up base pointer with `globalSymbolTable->lookupSymbol(identifier->name, indexValue)`

- Type checks value against the declared type of the array using `globalSymbolTable->getSymbolType(identifier->name)`

- Stores the value using `CreateStore(...)`

### `OutputAST`

Generates a `printf` call:

- Determines format string based on expression types

- Uses `builder.CreateGlobalStringPtr(fmt, ".fmt")` for format

- Calls `printf` with `builder.CreateCall(printfFunc, args)`

### `InputAST`

Generates a `scanf` call:

- Determines format string using variable type

- Gets pointer via `globalSymbolTable->lookupSymbol(Identifier->name)`
- Get input type `globalSymbolTable->getSymbolType(Identifier->name)`
- - Uses `builder.CreateGlobalStringPtr(fmt, ".fmt")` for format

- Calls `scanf` with `builder.CreateCall(scanfFunc, {fmtPtr, varPtr});`

### `BinaryOpAST`

Generates binary operations (e.g., `+`, `-`, `AND`):

- Delegates logic to helper function `performBinaryOperation(lhs, rhs, op)`

### `ComparisonAST`

Generates comparison operations (e.g., `=`, `<`, `>=`):

- Delegates logic to helper function `performComparison(lhs, rhs, cmpOp)`

### `IfAST`

Implements branching logic:

- Creates `if.then`, `if.else`, and `if.end` blocks using `BasicBlock::Create(...)`

- Branching via `CreateCondBr(...)` and `CreateBr(...)`

### `ForAST`

Generates a for-loop structure:

- Initializes iterator with ` assignment->codegen()`

- Condition, body, and increment blocks created using `BasicBlock::Create(...)`

- loopBB has `forBlock->codegen();`

- Increments statement is added in bison to `step->codegen();`

### `WhileAST`

Generates a while-loop:

- Evaluates condition at top `builder.SetInsertPoint(condBB)`

- Uses conditional branch to enter or exit loop

- Emits loop using `BasicBlock` and `CreateCondBr(...)`

### `RepeatAST`

Generates a repeat-until loop:

- Executes body before condition check so `builder.SetInsertPoint(condBB);` is at the end

- Negates condition and branches back as needed

### `ProcedureAST`

Defines a void-returning function:

- Creates function with `Function::Create(funcType, Function::ExternalLinkage, Identifier->name, *module)` with function type as void using `FunctionType::get(Type::getVoidTy(context), paramTypes, false)`

- Allocates and stores parameters using `builder.CreateAlloca(varType, nullptr, param->name)` and `builder.CreateStore(paramVal, alloca)`

- Emits body and returns via `CreateRetVoid()`

### `FuncAST`

Defines a function that returns a value:

- Same setup as `ProcedureAST`

- Returns result via `CreateRet(...)` in the `statementsBlock->codegen()`

---

## Debugging

A `DEBUG_PRINT_FUNCTION()` macro logs the function entry and current IR insertion point to a file (`llvm_debug.output.txt`).

Useful for tracing code generation and symbol resolution steps during compilation.
