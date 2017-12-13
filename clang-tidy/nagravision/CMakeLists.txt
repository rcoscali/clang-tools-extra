set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyNagravisionModule
  ArgumentNamingCheck.cpp
  NagravisionTidyModule.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangTidy
  )
