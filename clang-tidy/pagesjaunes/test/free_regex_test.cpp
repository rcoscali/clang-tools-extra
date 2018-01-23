//===------- free_regex_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "free_regex_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FreeRegexTest::FreeRegexTest()
          : free_re_m(PAGESJAUNES_REGEX_EXEC_SQL_FREE_REQ_RE)
        {
        }

        FreeRegexTest::~FreeRegexTest()
        {
        }

        void
        FreeRegexTest::SetUp(void)
        {
        }
        
        void
        FreeRegexTest::TearDown(void)
        {
        }
        
        void
        FreeRegexTest::PrintTo(const FreeRegexTest& free_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(FreeRegexTest, RegexMatchingIndicators)
        {
#define REQ0                                                            \
          "EXEC SQL FREE :emp_cv;" 
          
          llvm::StringRef req0(REQ0);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_free_re().match(req0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 4);
          if (retbool0 && retsize0 == 4)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL FREE :emp_cv;");
              EXPECT_STREQ(matches0[1].str().c_str(), "FREE");
              EXPECT_STREQ(matches0[2].str().c_str(), ":emp_cv");
              EXPECT_STREQ(matches0[3].str().c_str(), "emp_cv");
            }


#define REQ1                                                            \
          "EXEC SQL \n   FREE	 :emp_cv     ;" 
          
          llvm::StringRef req1(REQ1);
          SmallVector<StringRef, 8> matches1;
          bool retbool1 = get_free_re().match(req1, &matches1);
          EXPECT_TRUE(retbool1);
          int retsize1 = matches1.size();
          EXPECT_EQ(retsize1, 4);
          if (retbool1 && retsize1 == 4)
            {
              EXPECT_STREQ(matches1[0].str().c_str(), "EXEC SQL \n   FREE	 :emp_cv     ;");
              EXPECT_STREQ(matches1[1].str().c_str(), "FREE");
              EXPECT_STREQ(matches1[2].str().c_str(), ":emp_cv");
              EXPECT_STREQ(matches1[3].str().c_str(), "emp_cv");
            }
        }

        TEST_F(FreeRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD_0                                                            \
          "EXEC SQL \n  FREE : emp_cv ;"
          
          llvm::StringRef reqweird0(REQWEIRD_0);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_free_re().match(reqweird0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 4);
          if (retbool0 && retsize0 == 4)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL \n  FREE : emp_cv ;");
              EXPECT_STREQ(matches0[1].str().c_str(), "FREE");
              EXPECT_STREQ(matches0[2].str().c_str(), ": emp_cv");
              EXPECT_STREQ(matches0[3].str().c_str(), "emp_cv");
            }

#define REQWEIRD_1                                                            \
          "EXEC SQL \n  Free : _emp_cv ;"
          
          llvm::StringRef reqweird1(REQWEIRD_1);
          SmallVector<StringRef, 8> matches1;
          bool retbool1 = get_free_re().match(reqweird1, &matches1);
          EXPECT_TRUE(retbool1);
          int retsize1 = matches1.size();
          EXPECT_EQ(retsize1, 4);
          if (retbool1 && retsize1 == 4)
            {
              EXPECT_STREQ(matches1[0].str().c_str(), "EXEC SQL \n  Free : _emp_cv ;");
              EXPECT_STREQ(matches1[1].str().c_str(), "Free");
              EXPECT_STREQ(matches1[2].str().c_str(), ": _emp_cv");
              EXPECT_STREQ(matches1[3].str().c_str(), "_emp_cv");
            }

#define REQWEIRD_2                                                            \
          "EXEC SQL \n  Free : 1emp_cv ;"
          
          llvm::StringRef reqweird2(REQWEIRD_2);
          SmallVector<StringRef, 8> matches2;
          bool retbool2 = get_free_re().match(reqweird2, &matches2);
          EXPECT_FALSE(retbool2);
          int retsize2 = matches2.size();
          EXPECT_EQ(retsize2, 0);

#define REQWEIRD_3                                                            \
          "EXEC SQL \n  Free : \n emp_cv ;"
          
          llvm::StringRef reqweird3(REQWEIRD_3);
          SmallVector<StringRef, 8> matches3;
          bool retbool3 = get_free_re().match(reqweird3, &matches3);
          EXPECT_TRUE(retbool3);
          int retsize3 = matches3.size();
          EXPECT_EQ(retsize3, 4);
          if (retbool3 && retsize3 == 4)
            {
              EXPECT_STREQ(matches3[0].str().c_str(), "EXEC SQL \n  Free : \n emp_cv ;");
              EXPECT_STREQ(matches3[1].str().c_str(), "Free");
              EXPECT_STREQ(matches3[2].str().c_str(), ": \n emp_cv");
              EXPECT_STREQ(matches3[3].str().c_str(), "emp_cv");
            }

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
