//===------- fetch_tmpl_repeat_members2_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "fetch_tmpl_repeat_members2_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FetchTmplRepeatMembers2Test::FetchTmplRepeatMembers2Test()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_ALL_TMPL_REPEAT_MEMBERS_RE2)
        {
        }

        FetchTmplRepeatMembers2Test::~FetchTmplRepeatMembers2Test()
        {
        }

        void
        FetchTmplRepeatMembers2Test::SetUp(void)
        {
        }
        
        void
        FetchTmplRepeatMembers2Test::TearDown(void)
        {
        }
        
        void
        FetchTmplRepeatMembers2Test::PrintTo(const FetchTmplRepeatMembers2Test& fetch_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(FetchTmplRepeatMembers2Test, TemplRepeatMembers2RegexMatching)
        {
          llvm::StringRef req0(",1as, 2dd, 3d, df4f, sdf5ssqd");
          SmallVector<StringRef, 8> matches0;
          EXPECT_FALSE(get_fetch_re().match(req0, &matches0));
          //EXPECT_EQ(matches0.size(), 3);
          //EXPECT_STREQ(matches0[0].str().c_str(), ",1as, 2dd, 3d, df4f, sdf5ssqd");
          //EXPECT_STREQ(matches0[1].str().c_str(), ", 2dd, 3d, df4f, sdf5ssqd");
          //EXPECT_STREQ(matches0[2].str().c_str(), "1as");

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
