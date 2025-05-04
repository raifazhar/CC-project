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

- Allocates memory using `builder.CreateAlloca(...)`

- Registers the variable in the symbol table using `addSymbol(name, value)`

### `AssignmentAST`

Assigns a value to a variable:

- Looks up pointer using `lookupSymbol(name)`

- Type checks value against the declared type

- Stores the result with `builder.CreateStore(...)`

### `ArrayAST`

Declares a static-sized array:

- Allocates space for `[N x Type]` using `ArrayType::get(...)`

- Uses `CreateAlloca(...)` for storage

- Stores metadata in the symbol table using `addSymbol(...)`

### `ArrayAssignmentAST`

Assigns a value to an array element:

- Computes index with `CreateGEP(...)`

- Looks up base pointer with `lookupSymbol(...)`

- Stores the value using `CreateStore(...)`

### `OutputAST`

Generates a `printf` call:

- Determines format string based on expression types

- Uses `builder.CreateGlobalStringPtr(...)` for format

- Calls `printf` with `builder.CreateCall(...)`

### `InputAST`

Generates a `scanf` call:

- Determines format string using variable type

- Gets pointer via `lookupSymbol(...)`

- Calls `scanf` with `CreateCall(...)`

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

- Initializes iterator with `CreateStore(...)`

- Condition, body, and increment blocks created using `BasicBlock::Create(...)`

- Increments via `CreateAdd(...)` or `CreateFAdd(...)`

### `WhileAST`

Generates a while-loop:

- Evaluates condition at top

- Uses conditional branch to enter or exit loop

- Emits loop using `BasicBlock` and `CreateCondBr(...)`

### `RepeatAST`

Generates a repeat-until loop:

- Executes body before condition check

- Negates condition and branches back as needed

- **Functions used**: `CreateBr(...)`, `CreateICmpEQ(...)`/`CreateFCmpOEQ(...)`

### `ProcedureAST`

Defines a void-returning function:

- Creates function with `Function::Create(...)`

- Allocates and stores arguments using `CreateAlloca(...)` and `CreateStore(...)`

- Emits body and returns via `CreateRetVoid()`

### `FuncAST`

Defines a function that returns a value:

- Same setup as `ProcedureAST`

- Returns result via `CreateRet(...)`

---

## Debugging

A `DEBUG_PRINT_FUNCTION()` macro logs the function entry and current IR insertion point to a file (`llvm_debug.output.txt`).

Useful for tracing code generation and symbol resolution steps during compilation.
