set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyPagesJaunesModule
  ExecSQLToFunctionCall.cpp
  CCharToCXXString.cpp
  DeIncludePreProC.cpp
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
set_property(SOURCE ExecSQLToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE DeIncludePreProC.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive ")
set_property(SOURCE PagesJaunesTidyModule.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
