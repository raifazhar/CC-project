
# 📘 Bison Grammar Overview (parser.y)

The SSC language uses a Bison-based parser to build an Abstract Syntax Tree (AST) from source code. This AST is then used for code generation via LLVM. Below is a breakdown of the grammar definitions and how they map to the language's syntax and features.

---

## 🔧 File Structure

- **Includes**:
  - `AST.h`: Contains definitions for all AST node classes (e.g., expressions, statements, functions).
  - `llvm/IR/Value.h`: For LLVM IR value representation.

- **Externs**:
  - `yyparse()`: Starts parsing.
  - `yylex()`: Gets the next token from the lexer.
  - `yyin`, `yylineno`: Used for file input and error tracking.

- **Debugging**:
  - If `DEBUGBISON` is defined, additional debug output is printed during parsing.

---

## 📦 `%union` Declarations

Bison’s `%union` specifies the types of semantic values that can be used in grammar rules:

```c
%union {
  ASTNode *node;
  TypeAST *type;
  BinaryOpAST *binary;
  std::vector<ASTNode*> *block;
  int intValue;
  double floatValue;
  char *stringValue;
  char charValue;
  ...
}
```

These correspond to nodes in the AST and literal values.

---

## 🧱 Tokens

Tokens represent the basic units of the language and are produced by the Flex lexer. They include:

- **Literals**: `tok_Integer_Literal`, `tok_Float_Literal`, `tok_String_Literal`, `tok_Char_Literal`
- **Identifiers**: `tok_Identifier`
- **Operators**: `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>=`, etc.
- **Keywords**: `DECLARE`, `IF`, `WHILE`, `FOR`, `REPEAT`, `FUNCTION`, `PROCEDURE`, `RETURN`, `CALL`, `INPUT`, `OUTPUT`, `END_IF`, `NEXT`, `UNTIL`, etc.

---

## 📜 Grammar Rules

### `root`

The top-level rule:
```bison
root: statements
```
This initializes a new symbol table scope and translates the parsed statements into LLVM IR.

---

### `statements`, `statement_line`

Handles multiple lines of statements. Example:
```ssc
DECLARE x: INTEGER
x = 5
OUTPUT x
```

---

### `statement`

Defines a single executable action:
- Assignments: `x = 5`
- Control Flow: `IF`, `WHILE`, `FOR`, `REPEAT`
- I/O: `INPUT`, `OUTPUT`
- Declarations
- Function/Procedure definitions and calls

---

### `type`

Specifies SSC data types:
```ssc
INTEGER, REAL, CHAR, STRING, BOOLEAN
```

Maps to `TypeAST` nodes for type checking and code generation.

---

### `declaration`

Variable declaration:
```ssc
DECLARE x: INTEGER
DECLARE arr: ARRAY[1:10] OF INTEGER
```

---

### `assignment` / `array_assignment`

Assignment rules:
```ssc
x = 10
arr[2] = x + 5
```

---

### `expression`, `term`, `factor`

Arithmetic and logical expressions:
```ssc
a + b * 2
(a + b) / c
NOT flag
```

Also supports:
- Unary operations (`-a`, `+a`)
- Function calls: `CALL foo(1, 2)`
- Literals and variables

---

### `comparison`

Handles relational operators:
```ssc
a < b
x != y
a AND b
NOT condition
```

---

### `statement_block`

Used for blocks within control structures. A block is a sequence of indented statements.

---

### Control Flow

- **If Statement**:
  ```ssc
  IF condition THEN
    ...
  ELSE
    ...
  END_IF
  ```

- **While Loop**:
  ```ssc
  WHILE condition
    ...
  END_WHILE
  ```

- **For Loop**:
  ```ssc
  FOR i = 1 TO 10 STEP 2
    ...
  NEXT i
  ```

- **Repeat Loop**:
  ```ssc
  REPEAT
    ...
  UNTIL condition
  ```

---

### Functions and Procedures

- **Procedure**:
  ```ssc
  PROCEDURE PrintMessage(message: STRING)
    OUTPUT message
  ENDPROCEDURE
  ```

- **Function**:
  ```ssc
  FUNCTION Add(a: INTEGER, b: INTEGER): INTEGER
    RETURN a + b
  ENDFUNCTION
  ```

- **Return Statement**:
  ```ssc
  RETURN result
  ```

---

### `parameter_list`, `argument_list`

Handle function/procedure parameters:
```ssc
FUNCTION Sum(a: INTEGER, b: INTEGER): INTEGER
```
And arguments:
```ssc
CALL Sum(1, 2)
```

---

## ⚙️ Code Generation

After parsing, each AST node calls `.codegen()` to emit LLVM IR. The final IR is then compiled or interpreted.

---

## ❌ Error Handling

Errors (like using the wrong type, or mismatched loop variables) are reported via `yyerror()`. A `YYERROR` call prevents further parsing of invalid input.
