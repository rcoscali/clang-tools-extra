//===------- close_regex_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "close_regex_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        CloseRegexTest::CloseRegexTest()
          : close_re_m(PAGESJAUNES_REGEX_EXEC_SQL_CLOSE_REQ_RE)
        {
        }

        CloseRegexTest::~CloseRegexTest()
        {
        }

        void
        CloseRegexTest::SetUp(void)
        {
        }
        
        void
        CloseRegexTest::TearDown(void)
        {
        }
        
        void
        CloseRegexTest::PrintTo(const CloseRegexTest& close_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(CloseRegexTest, RegexMatchingIndicators)
        {
#define REQ                                                             \
          "EXEC SQL CLOSE crsCountInsEPJ0; " 
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_close_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 3);
          EXPECT_STREQ(matches0[1].str().c_str(), "CLOSE");
          EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  CLOSE crsCountInsEPJ1; "
          
          llvm::StringRef req1(REQ);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_close_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 3);
          EXPECT_STREQ(matches1[1].str().c_str(), "CLOSE");
          EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
#undef REQ
        }

        TEST_F(CloseRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  CLOSE crsCountIns_EPJ0\n"                                \
            "  ; " 
          
          llvm::StringRef reqweird0(REQWEIRD);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_close_re().match(reqweird0, &matches0));
          EXPECT_EQ(matches0.size(), 3);
          EXPECT_STREQ(matches0[1].str().c_str(), "CLOSE");
          EXPECT_STREQ(matches0[2].str().c_str(), "crsCountIns_EPJ0");
#undef REQWEIRD

#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  CLOSE 1crsCountInsEPJ0\n"                                \
            "  ; " 
          
          llvm::StringRef reqweird1(REQWEIRD);
          SmallVector<StringRef, 8> matches1;
          EXPECT_FALSE(get_close_re().match(reqweird1, &matches1));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  CLOSE __crsCount_Ins_EPJ_0__\n"                          \
            "  ; " 
          
          llvm::StringRef reqweird2(REQWEIRD);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_close_re().match(reqweird2, &matches2));
          EXPECT_EQ(matches2.size(), 3);
          EXPECT_STREQ(matches2[1].str().c_str(), "CLOSE");
          EXPECT_STREQ(matches2[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC 		\n"                                     \
            "	   SQL   \n"                                            \
            "  cLOsE __crsCount_Ins_EPJ_0__\n"                          \
            "  ; " 
          
          llvm::StringRef reqweird3(REQWEIRD);
          SmallVector<StringRef, 8> matches3;
          EXPECT_TRUE(get_close_re().match(reqweird3, &matches3));
          EXPECT_EQ(matches3.size(), 3);
          EXPECT_STREQ(matches3[1].str().c_str(), "cLOsE");
          EXPECT_STREQ(matches3[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
#undef REQWEIRD
          
        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
