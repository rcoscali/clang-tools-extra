//===------- fetch_fileline_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "fetch_fileline_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FetchFilelineTest::FetchFilelineTest()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_ALL_FILELINE)
        {
        }

        FetchFilelineTest::~FetchFilelineTest()
        {
        }

        void
        FetchFilelineTest::SetUp(void)
        {
        }
        
        void
        FetchFilelineTest::TearDown(void)
        {
        }
        
        void
        FetchFilelineTest::PrintTo(const FetchFilelineTest& fetch_fileline_test, ::std::ostream* os)
        {
        }

        TEST_F(FetchFilelineTest, FilelineMatching)
        {
          llvm::StringRef req0("/usr/local/src/data2/misc/LLVM/llvm-6.0.0/tools#12345");
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 3);
          EXPECT_STREQ(matches0[1].str().c_str(), "/usr/local/src/data2/misc/LLVM/llvm-6.0.0/tools");
          EXPECT_STREQ(matches0[2].str().c_str(), "12345");

          llvm::StringRef req1("./local/src/data2/misc/LLVM/llvm-6.0.0/tools#12345");
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 3);
          EXPECT_STREQ(matches1[1].str().c_str(), "./local/src/data2/misc/LLVM/llvm-6.0.0/tools");
          EXPECT_STREQ(matches1[2].str().c_str(), "12345");

          llvm::StringRef req2("./local/src/data2/misc/LLVM/llvm-6.0.0/tools#12345");
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(req2, &matches2));
          EXPECT_EQ(matches2.size(), 3);
          EXPECT_STREQ(matches2[1].str().c_str(), "./local/src/data2/misc/LLVM/llvm-6.0.0/tools");
          EXPECT_STREQ(matches2[2].str().c_str(), "12345");

          llvm::StringRef req3("   ./local/src/data2/misc/LLVM/llvm-6.0.0/tools#  12345");
          SmallVector<StringRef, 8> matches3;
          EXPECT_FALSE(get_fetch_re().match(req3, &matches3));

          llvm::StringRef req4("./local/src/data2/misc/LLVM/llvm-6.0.0/tools  #12345");
          SmallVector<StringRef, 8> matches4;
          EXPECT_TRUE(get_fetch_re().match(req4, &matches4));
          EXPECT_EQ(matches4.size(), 3);
          EXPECT_STREQ(matches4[1].str().c_str(), "./local/src/data2/misc/LLVM/llvm-6.0.0/tools  ");
          EXPECT_STREQ(matches4[2].str().c_str(), "12345");

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
