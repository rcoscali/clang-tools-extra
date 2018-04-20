set(LLVM_LINK_COMPONENTS support)

## Removed unused rules
add_clang_library(clangTidyPagesJaunesModule

  CCharToCXXString.cpp
  CCharList.cpp
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
  ExecSQLCloseToFunctionCall.cpp
  ExecSQLPrepareFmtdToFunctionCall.cpp
  ExecSQLPrepareToFunctionCall.cpp
  ExecSQLToFunctionCall.cpp
  ExecSQLCommon.cpp
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
set_property(SOURCE CCharToCXXString.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
set_property(SOURCE CCharList.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
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
set_property(SOURCE ExecSQLCloseToFunctionCall.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-cast-qual -fpermissive -fexceptions ")
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
set(PAGESJAUNES_CPPTEST_COMMAND pagesjaunes_test CACHE FILEPATH "Executable generated for C++ tests run")
add_test(
  NAME pagesjaunes_test
  COMMAND pagesjaunes_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/test)
add_executable(pagesjaunes_test
  test/test_main.cpp
  ExecSQLCommon.cpp
  test/backup_file.cpp
  test/buffer_split.cpp
  test/decode_host_vars.cpp
  test/open_regex_test.cpp
  test/open_request_test.cpp
  test/close_regex_test.cpp
  test/allocate_regex_test.cpp
  test/free_regex_test.cpp
  test/declare_regex_test.cpp
  test/fetch_regex_test.cpp
  test/fetch_decode_host_var.cpp
  test/fetch_tmpl_repeat_test.cpp
  test/fetch_tmpl_repeat_members_test.cpp
  test/fetch_tmpl_repeat_members2_test.cpp
  test/fetch_fileline_test.cpp)
target_link_libraries(pagesjaunes_test
  "clangTidy;clangTidyUtils;clangAST;clangASTMatchers;clangBasic;clangFormat;clangFrontend;clangLex;clangRewrite;clangSema;clangStaticAnalyzerCore;clangStaticAnalyzerFrontend;clangTooling;clangToolingCore;clangToolingASTDiff;clangToolingRefactor;LLVMSupport;gtestall;pthread")
set_target_properties(pagesjaunes_test PROPERTIES
  COMPILE_FLAGS "-std=c++11 -fno-rtti -DACTIVATE_TRACES_DEFINED"
  LINK_FLAGS "-std=c++11 -fno-rtti")
