//===------- open_regex_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "open_regex_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        OpenRegexTest::OpenRegexTest()
          : open_re_m(PAGESJAUNES_REGEX_EXEC_SQL_OPEN_REQ_RE)
        {
        }

        OpenRegexTest::~OpenRegexTest()
        {
        }

        void
        OpenRegexTest::SetUp(void)
        {
        }
        
        void
        OpenRegexTest::TearDown(void)
        {
        }
        
        void
        OpenRegexTest::PrintTo(const OpenRegexTest& open_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(OpenRegexTest, RegexMatchingIndicators)
        {
#define REQ                                     \
          "EXEC SQL OPEN crsCountInsEPJ0; " 
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_open_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 3);
          EXPECT_STREQ(matches0[1].str().c_str(), "OPEN");
          EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  OPEN crsCountInsEPJ1; "
          
          llvm::StringRef req1(REQ);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_open_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 3);
          EXPECT_STREQ(matches1[1].str().c_str(), "OPEN");
          EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
#undef REQ
        }

        TEST_F(OpenRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  OPEN crsCountIns_EPJ0\n"                                 \
            "  ; " 
          
          llvm::StringRef reqweird2(REQWEIRD);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_open_re().match(reqweird2, &matches2));
          EXPECT_EQ(matches2.size(), 3);
          EXPECT_STREQ(matches2[1].str().c_str(), "OPEN");
          EXPECT_STREQ(matches2[2].str().c_str(), "crsCountIns_EPJ0");
#undef REQWEIRD

#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  OPEN 1crsCountInsEPJ0\n"                                 \
            "  ; " 
          
          llvm::StringRef reqweird3(REQWEIRD);
          SmallVector<StringRef, 8> matches3;
          EXPECT_FALSE(get_open_re().match(reqweird3, &matches3));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  OPEN __crsCount_Ins_EPJ_0__\n"                           \
            "  ; " 
          
          llvm::StringRef reqweird4(REQWEIRD);
          SmallVector<StringRef, 8> matches4;
          EXPECT_TRUE(get_open_re().match(reqweird4, &matches4));
          EXPECT_EQ(matches4.size(), 3);
          EXPECT_STREQ(matches4[1].str().c_str(), "OPEN");
          EXPECT_STREQ(matches4[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC 		\n"                                     \
            "	   SQL   \n"                                            \
            "  open __crsCount_Ins_EPJ_0__\n"                           \
            "  ; " 
          
          llvm::StringRef reqweird5(REQWEIRD);
          SmallVector<StringRef, 8> matches5;
          EXPECT_TRUE(get_open_re().match(reqweird5, &matches5));
          EXPECT_EQ(matches5.size(), 3);
          EXPECT_STREQ(matches5[1].str().c_str(), "open");
          EXPECT_STREQ(matches5[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
#undef REQWEIRD
          
        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
