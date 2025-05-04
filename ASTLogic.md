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

Looks up a variable in the symbol table and returns a `load` instruction unless it’s a function argument.

### `DeclarationAST`

Declares a scalar variable:

- Allocates memory using `alloca`
- Registers the variable in the symbol table

### `AssignmentAST`

Assigns a value to a variable:

- Type checks value against the declared type
- Stores the result with `store`

### `ArrayAST`

Declares a static-sized array:

- Allocates space for `[N x Type]`
- Stores the metadata in the symbol table

### `ArrayAssignmentAST`

Assigns a value to an array element:

- Computes index and gets pointer via symbol table
- Type checks element type
- Stores the value

### `OutputAST`

Generates a `printf` call:

- Determines format string based on expression types
- Emits global format string and call to `printf`

### `InputAST`

Generates a `scanf` call:

- Uses variable’s LLVM type to pick correct format string

### `BinaryOpAST`

Generates binary operations (e.g., `+`, `-`, `AND`):

- Delegates to `performBinaryOperation(lhs, rhs, op)`

### `ComparisonAST`

Generates comparison operations (e.g., `=`, `<`, `>=`):

- Delegates to `performComparison(lhs, rhs, cmpOp)`

### `IfAST`

Implements branching logic:

- Creates `if.then`, `if.else`, and `if.end` blocks
- Generates code for both blocks and merges control flow

### `ForAST`

Generates a for-loop structure:

- Has blocks for condition, loop body, and end
- Emits increment via `step` expression

### `WhileAST`

Generates a while-loop:

- Evaluates condition at top
- Branches back to loop body if condition is true

### `RepeatAST`

Generates a repeat-until loop:

- Body executes before condition
- Negates condition and branches appropriately

### `ProcedureAST`

Defines a void-returning function:

- Creates function with parameters
- Allocates and stores arguments into `alloca`
- Generates body code and ends with `ret void`

### `FuncAST`

Defines a function that returns a value:

- Same setup as `ProcedureAST`
- Returns a typed value via `ret <value>`

---

## Debugging

A `DEBUG_PRINT_FUNCTION()` macro logs the function entry and current IR insertion point to a file (`llvm_debug.output.txt`).
