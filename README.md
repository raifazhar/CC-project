# SSC Compiler

A simple compiler for the SSC language using Flex, Bison, and LLVM.

## Project Achievements

The SSC compiler project has successfully implemented the following:

- **Custom Compiler for SSC Language**: A fully functional compiler for the SSC language using the Flex lexer, Bison parser, and LLVM for code generation.
- **LLVM IR Generation**: The compiler translates SSC code into LLVM Intermediate Representation (IR), enabling further optimizations and compilation into machine code.
- **Modular Compiler Architecture**: The project uses a modular approach with separate components for lexical analysis, syntax parsing, semantic analysis, optimization, and code generation.

## Goals

The key goals for the SSC compiler project include:

- **Extend Language Features**: Add new constructs and features to the SSC language to support more complex programs.
- **Optimization**: Improve the LLVM IR optimization techniques to enhance the performance of compiled programs.
- **Cross-Platform Compilation**: Ensure that the compiler can be used to generate executables for different platforms using LLVM's capabilities.
- **Documentation**: Provide comprehensive documentation for users and developers, covering both the architecture of the compiler and the grammar of the SSC language.

## Documentation

For detailed information on how to build, run, and optimize the SSC compiler, refer to:

- 📄 [SSC Compiler Documentation](SSC_Compiler_Documentation.md)
- 📄 [Bison Grammar](SSC_Bison_Grammar.md)
- 📄 [Build Table](BUILD_TABLE.md)
