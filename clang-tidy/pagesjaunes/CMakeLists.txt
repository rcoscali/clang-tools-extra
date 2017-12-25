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
  FileManipulator.cpp
  PagesJaunesTidyModule.cpp
  
  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangTidy
  clangLex
  clangTidyUtils
  clangTooling
  clangFrontend
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
set_property(SOURCE FileManipulator.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE PagesJaunesTidyModule.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")

# Googletest directory
set(LLVM_GOOGLETEST_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../../utils/unittest/googletest")

# Add GoogleTest static library target
add_library(gtestall STATIC ${LLVM_GOOGLETEST_DIR}/src/gtest-all.cc)
target_compile_options(gtestall PUBLIC "-std=c++11")
target_include_directories(gtestall PRIVATE ${LLVM_GOOGLETEST_DIR})
target_include_directories(gtestall SYSTEM BEFORE PRIVATE ${LLVM_GOOGLETEST_DIR}/include)

# Enable testing for pagesjaunes
enable_testing()
add_test(
  NAME fetch_regex_test
  COMMAND fetch_regex_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test)
add_executable(fetch_regex_test test/test_main.cpp test/fetch_regex_test.cpp)
target_link_libraries(fetch_regex_test "clangAST;clangASTMatchers;clangBasic;clangTidy;clangLex;clangTidyUtils;clangTooling;clangFrontend;gtestall;pthread")
