set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyPagesJaunesModule
  ExecSQLToFunctionCall.cpp
  CCharToCXXString.cpp
  PagesJaunesTidyModule.cpp
  
  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangTidy
  )
