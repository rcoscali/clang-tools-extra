//===------- fetch_tmpl_repeat_members_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "fetch_tmpl_repeat_members_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FetchTmplRepeatMembersTest::FetchTmplRepeatMembersTest()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_ALL_TMPL_REPEAT_MEMBERS_RE)
        {
        }

        FetchTmplRepeatMembersTest::~FetchTmplRepeatMembersTest()
        {
        }

        void
        FetchTmplRepeatMembersTest::SetUp(void)
        {
        }
        
        void
        FetchTmplRepeatMembersTest::TearDown(void)
        {
        }
        
        void
        FetchTmplRepeatMembersTest::PrintTo(const FetchTmplRepeatMembersTest& fetch_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(FetchTmplRepeatMembersTest, TemplRepeatMembersRegexMatching)
        {
          llvm::StringRef req0(", arg1, arg2, arg3");
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 3);
          EXPECT_STREQ(matches0[0].str().c_str(), ", arg1, arg2, arg3");
          EXPECT_STREQ(matches0[1].str().c_str(), ", arg3");
          EXPECT_STREQ(matches0[2].str().c_str(), "arg3");

          llvm::StringRef req1(", arg1, arg2");
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 3);
          EXPECT_STREQ(matches1[0].str().c_str(), ", arg1, arg2");
          EXPECT_STREQ(matches1[1].str().c_str(), ", arg2");
          EXPECT_STREQ(matches1[2].str().c_str(), "arg2");

          llvm::StringRef req2(", arg1");
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(req2, &matches2));
          EXPECT_EQ(matches2.size(), 3);
          EXPECT_STREQ(matches2[0].str().c_str(), ", arg1");
          EXPECT_STREQ(matches2[1].str().c_str(), ", arg1");
          EXPECT_STREQ(matches2[2].str().c_str(), "arg1");

        }

        TEST_F(FetchTmplRepeatMembersTest, TemplRepeatMembersRegexMoreBlankMatching)
        {
          llvm::StringRef req0(", arg1  ,  arg2  , 	arg3  ");
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 3);
          EXPECT_STREQ(matches0[0].str().c_str(), ", arg1  ,  arg2  , 	arg3  ");
          EXPECT_STREQ(matches0[1].str().c_str(), ", 	arg3  ");
          EXPECT_STREQ(matches0[2].str().c_str(), "arg3");

          llvm::StringRef req1(", arg1	, 	arg2	");
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 3);
          EXPECT_STREQ(matches1[0].str().c_str(), ", arg1	, 	arg2	");
          EXPECT_STREQ(matches1[1].str().c_str(), ", 	arg2	");
          EXPECT_STREQ(matches1[2].str().c_str(), "arg2");

          llvm::StringRef req2(",  arg1  ");
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(req2, &matches2));
          EXPECT_EQ(matches2.size(), 3);
          EXPECT_STREQ(matches2[0].str().c_str(), ",  arg1  ");
          EXPECT_STREQ(matches2[1].str().c_str(), ",  arg1  ");
          EXPECT_STREQ(matches2[2].str().c_str(), "arg1");

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
