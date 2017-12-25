//===------- fetch_regex_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "fetch_regex_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FetchRegexTest::FetchRegexTest()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE)
        {
        }

        FetchRegexTest::~FetchRegexTest()
        {
        }

        void
        FetchRegexTest::SetUp(void)
        {
        }
        
        void
        FetchRegexTest::TearDown(void)
        {
        }
        
        void
        FetchRegexTest::PrintTo(const FetchRegexTest& fetch_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(FetchRegexTest, RegexMatchingIndicators)
        {
#define REQ0                                                            \
          "EXEC SQL \n"                                                  \
            "  FETCH crsCountInsEPJ0\n"                                 \
            "  INTO :iNbIns:iNbInsI; " 
          
          llvm::StringRef req0(REQ0);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 5);
          EXPECT_STREQ(matches0[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
          EXPECT_STREQ(matches0[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches0[4].str().c_str(), ":iNbIns:iNbInsI");

#define REQ1                                                            \
          "EXEC SQL\n"                                                  \
            "  FETCH crsCountInsEPJ1\n"                                  \
            "  INTO :champStruct.champInt4:champStruct.champInt4I; "
          
          llvm::StringRef req1(REQ1);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 5);
          EXPECT_STREQ(matches1[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
          EXPECT_STREQ(matches1[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches1[4].str().c_str(), ":champStruct.champInt4:champStruct.champInt4I");

#define REQ2                                                            \
          "EXEC SQL\n"                                                  \
            "  FETCH crsCountInsEPJ2\n"                                  \
            "  INTO :pChampStruct->champInt4:pChampStruct->champInt4I; "
          
          llvm::StringRef req2(REQ2);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(req2, &matches2));
          EXPECT_EQ(matches1.size(), 5);
          EXPECT_STREQ(matches2[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches2[2].str().c_str(), "crsCountInsEPJ2");
          EXPECT_STREQ(matches2[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches2[4].str().c_str(), ":pChampStruct->champInt4:pChampStruct->champInt4I");

        }

        TEST_F(FetchRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQ_0                                                            \
          "EXEC SQL \n"                                                  \
            "  FETCH: crsCountInsEPJ0\n"                                 \
            "  INTO: iNbIns:iNbInsI; " 
          
          llvm::StringRef req0(REQ_0);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 5);
          EXPECT_STREQ(matches0[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches0[2].str().c_str(), ": crsCountInsEPJ0");
          EXPECT_STREQ(matches0[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches0[4].str().c_str(), ": iNbIns:iNbInsI");

#define REQ_1                                                            \
          "EXEC SQL\n"                                                  \
            "  FETCH: crsCountInsEPJ1\n"                                  \
            "  INTO: champStruct.champInt4: champStruct.champInt4I; "
          
          llvm::StringRef req1(REQ_1);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 5);
          EXPECT_STREQ(matches1[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches1[2].str().c_str(), ": crsCountInsEPJ1");
          EXPECT_STREQ(matches1[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches1[4].str().c_str(), ": champStruct.champInt4: champStruct.champInt4I");

#define REQ_2                                                            \
          "EXEC SQL\n"                                                  \
            "  FETCH: crsCountInsEPJ2\n"                                  \
            "  INTO: pChampStruct->champInt4 :pChampStruct->champInt4I; "
          
          llvm::StringRef req2(REQ_2);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(req2, &matches2));
          EXPECT_EQ(matches1.size(), 5);
          EXPECT_STREQ(matches2[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches2[2].str().c_str(), ": crsCountInsEPJ2");
          EXPECT_STREQ(matches2[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches2[4].str().c_str(), ": pChampStruct->champInt4 :pChampStruct->champInt4I");

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
