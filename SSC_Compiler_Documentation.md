
# SSC Compiler

A simple compiler for the SSC language using Flex, Bison, and LLVM.

## Build Structure

The build process is organized into the following directories:

- `build/`: Main build directory
  - `bin/`: Binary executables
  - `debug/`: Debug output files
  - `ir/`: LLVM IR files
  - `obj/`: Object files

## Building the Compiler

To build the compiler, run:

```bash
make
```

This will compile the compiler and place the executable in the `build/bin/` directory.

## Generating LLVM IR

To generate LLVM IR from an input file, run:

```bash
make ir
```

This will generate the LLVM IR in `build/ir/output.ll`.

## Optimizing LLVM IR

To optimize the LLVM IR, run:

```bash
make opt
```

This will generate the optimized LLVM IR in `build/ir/output_opt.ll`.

## Running the Compiler

To run the compiler on an input file, compile the IR, and run the resulting executable, run:

```bash
make run
```

This will:
1. Generate LLVM IR from the input file
2. Compile the IR to an executable
3. Run the executable

## Debugging

To run the compiler without redirecting output (for debugging), run:

```bash
make debug
```

## Cleaning Up

To clean up all generated files, run:

```bash
make clean
```

To clean up all build files and output, run:

```bash
make distclean
```

## Help

To display help information, run:

```bash
make help
```

## Build Table

| File/Directory         | Description                          |
|------------------------|--------------------------------------|
| `build/`               | Main build directory                 |
| `build/bin/`           | Output binary executable             |
| `build/debug/`         | Debug output files                   |
| `build/ir/output.ll`   | Generated LLVM IR                    |
| `build/ir/output_opt.ll`| Optimized LLVM IR                   |
| `build/obj/`           | Intermediate object files            |
| `Makefile`             | Build automation file                |
| `src/`                 | Source files (Flex, Bison, C++)      |

