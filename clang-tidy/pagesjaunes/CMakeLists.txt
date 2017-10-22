set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyPagesJaunesModule

  CCharToCXXString.cpp
  DeIncludePreProC.cpp
  ExecSQLAllocateToFunctionCall.cpp
  ExecSQLFetchToFunctionCall.cpp
  ExecSQLForToFunctionCall.cpp
  ExecSQLFreeToFunctionCall.cpp
  ExecSQLLOBCreateToFunctionCall.cpp
  ExecSQLLOBOpenToFunctionCall.cpp
  ExecSQLLOBReadToFunctionCall.cpp
  ExecSQLLOBCloseToFunctionCall.cpp
  ExecSQLLOBFreeToFunctionCall.cpp
  ExecSQLOpenToFunctionCall.cpp
  ExecSQLPrepareFmtdToFunctionCall.cpp
  ExecSQLPrepareToFunctionCall.cpp
  ExecSQLToFunctionCall.cpp
  PagesJaunesTidyModule.cpp
  
  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangTidy
  )

# Remove the -Wcast-qual because of some AST nodes access through
# Result.Node.getNodeAs returning a const ptr
set_property(SOURCE CCharToCXXString.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive ")
set_property(SOURCE DeIncludePreProC.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive ")
set_property(SOURCE ExecSQLAllocateToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLFetchToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLForToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLFreeToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLLOBCreateToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLLOBOpenToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLLOBReadToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLLOBCloseToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLLOBFreeToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLOpenToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLPrepareFmtdToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLPrepareToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE ExecSQLToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE PagesJaunesTidyModule.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
