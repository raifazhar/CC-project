@echo off
echo Building SSC compiler...

REM Compile IR.cpp
clang++ -IC:/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -c IR.cpp -o IR.o

REM Compile Symbol_Table.cpp
clang++ -IC:/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -c Symbol_Table.cpp -o Symbol_Table.o

REM Compile AST.cpp
clang++ -IC:/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -c AST.cpp -o AST.o

REM Compile ssc.tab.c
clang++ -IC:/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -c ssc.tab.c -o ssc.tab.o

REM Compile lex.yy.c
clang++ -IC:/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -c lex.yy.c -o lex.yy.o

REM Link everything together
clang++ -IC:/msys64/mingw64/include -std=c++17 -fno-exceptions -funwind-tables -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -o ssc_compiler ssc.tab.o lex.yy.o IR.o Symbol_Table.o AST.o -LC:/msys64/mingw64/lib -lLLVM-19

echo Build complete! 