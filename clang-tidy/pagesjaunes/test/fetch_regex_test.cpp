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
          EXPECT_EQ(matches2.size(), 5);
          EXPECT_STREQ(matches2[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches2[2].str().c_str(), "crsCountInsEPJ2");
          EXPECT_STREQ(matches2[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches2[4].str().c_str(), ":pChampStruct->champInt4:pChampStruct->champInt4I");

        }

        TEST_F(FetchRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD_0                                                            \
          "EXEC SQL \n"                                                  \
            "  FETCH: crsCountInsEPJ0\n"                                 \
            "  INTO: iNbIns:iNbInsI; " 
          
          llvm::StringRef reqweird0(REQWEIRD_0);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(reqweird0, &matches0));
          EXPECT_EQ(matches0.size(), 5);
          EXPECT_STREQ(matches0[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches0[2].str().c_str(), ": crsCountInsEPJ0");
          EXPECT_STREQ(matches0[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches0[4].str().c_str(), ": iNbIns:iNbInsI");

#define REQWEIRD_1                                                            \
          "EXEC SQL\n"                                                  \
            "  FETCH: crsCountInsEPJ1\n"                                  \
            "  INTO: champStruct.champInt4: champStruct.champInt4I; "
          
          llvm::StringRef reqweird1(REQWEIRD_1);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(reqweird1, &matches1));
          EXPECT_EQ(matches1.size(), 5);
          EXPECT_STREQ(matches1[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches1[2].str().c_str(), ": crsCountInsEPJ1");
          EXPECT_STREQ(matches1[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches1[4].str().c_str(), ": champStruct.champInt4: champStruct.champInt4I");

#define REQWEIRD_2                                                            \
          "EXEC SQL\n"                                                  \
            "  FETCH: crsCountInsEPJ2\n"                                  \
            "  INTO: pChampStruct->champInt4 :pChampStruct->champInt4I; "
          
          llvm::StringRef reqweird2(REQWEIRD_2);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(reqweird2, &matches2));
          EXPECT_EQ(matches2.size(), 5);
          EXPECT_STREQ(matches2[1].str().c_str(), "FETCH");
          EXPECT_STREQ(matches2[2].str().c_str(), ": crsCountInsEPJ2");
          EXPECT_STREQ(matches2[3].str().c_str(), "INTO");
          EXPECT_STREQ(matches2[4].str().c_str(), ": pChampStruct->champInt4 :pChampStruct->champInt4I");

#define REQWEIRD_3                                                            \
          "EXEC SQL\n"                                                  \
            "  FeTCH: __crs_Count_Ins_EPJ2_\n"                                  \
            "  INtO: _pChamp_1Struct->_champ_Int4 :_p_Champ4Struct->_champ_Int4I; "
          
          llvm::StringRef reqweird3(REQWEIRD_3);
          SmallVector<StringRef, 8> matches3;
          EXPECT_TRUE(get_fetch_re().match(reqweird3, &matches3));
          EXPECT_EQ(matches3.size(), 5);
          EXPECT_STREQ(matches3[1].str().c_str(), "FeTCH");
          EXPECT_STREQ(matches3[2].str().c_str(), ": __crs_Count_Ins_EPJ2_");
          EXPECT_STREQ(matches3[3].str().c_str(), "INtO");
          EXPECT_STREQ(matches3[4].str().c_str(), ": _pChamp_1Struct->_champ_Int4 :_p_Champ4Struct->_champ_Int4I");

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
