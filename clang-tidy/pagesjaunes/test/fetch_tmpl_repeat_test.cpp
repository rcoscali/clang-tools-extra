//===------- fetch_tmpl_repeat_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "fetch_tmpl_repeat_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FetchTmplRepeatTest::FetchTmplRepeatTest()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_ALL_TMPL_REPEAT_RE)
        {
        }

        FetchTmplRepeatTest::~FetchTmplRepeatTest()
        {
        }

        void
        FetchTmplRepeatTest::SetUp(void)
        {
        }
        
        void
        FetchTmplRepeatTest::TearDown(void)
        {
        }
        
        void
        FetchTmplRepeatTest::PrintTo(const FetchTmplRepeatTest& fetch_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(FetchTmplRepeatTest, TemplRepeatRegexMatching)
        {
#define REQ0 \
"   \n\
@repeat on AnonStructures{Name, Def}\n\
    \n\
"
          llvm::StringRef req0(REQ0);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_fetch_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 4);
          EXPECT_STREQ(matches0[0].str().c_str(), "@repeat on AnonStructures{Name, Def}");
          EXPECT_STREQ(matches0[1].str().c_str(), "AnonStructures");
          EXPECT_STREQ(matches0[2].str().c_str(), "Name");
          EXPECT_STREQ(matches0[3].str().c_str(), " Def");

#define REQ1 \
"\n\
@repeat on AnonStructures{ Name, Def, Tab}\n\
\n\
"
          llvm::StringRef req1(REQ1);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_fetch_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 4);
          EXPECT_STREQ(matches1[0].str().c_str(), "@repeat on AnonStructures{ Name, Def, Tab}");
          EXPECT_STREQ(matches1[1].str().c_str(), "AnonStructures");
          EXPECT_STREQ(matches1[2].str().c_str(), "Name");
          EXPECT_STREQ(matches1[3].str().c_str(), " Def, Tab");

#define REQ2 \
"\n\
@repeat on AnonStructures	{ Name, Def, Tab}\n\
\n\
"
          llvm::StringRef req2(REQ2);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_fetch_re().match(req2, &matches2));
          EXPECT_EQ(matches2.size(), 4);
          EXPECT_STREQ(matches2[0].str().c_str(), "@repeat on AnonStructures	{ Name, Def, Tab}");
          EXPECT_STREQ(matches2[1].str().c_str(), "AnonStructures");
          EXPECT_STREQ(matches2[2].str().c_str(), "Name");
          EXPECT_STREQ(matches2[3].str().c_str(), " Def, Tab");

#define REQ3 \
"\n\
@repeat on AnonStructures	{Name,Def,Tab  }\n\
\n\
"
          llvm::StringRef req3(REQ3);
          SmallVector<StringRef, 8> matches3;
          EXPECT_TRUE(get_fetch_re().match(req3, &matches3));
          EXPECT_EQ(matches3.size(), 4);
          EXPECT_STREQ(matches3[0].str().c_str(), "@repeat on AnonStructures	{Name,Def,Tab  }");
          EXPECT_STREQ(matches3[1].str().c_str(), "AnonStructures");
          EXPECT_STREQ(matches3[2].str().c_str(), "Name");
          EXPECT_STREQ(matches3[3].str().c_str(), "Def,Tab  ");

#define REQ4 \
"\n\
@repeat on AnonStructures	{ Name , Def,Tab  }\n\
\n\
"
          llvm::StringRef req4(REQ4);
          SmallVector<StringRef, 8> matches4;
          EXPECT_TRUE(get_fetch_re().match(req4, &matches4));
          EXPECT_EQ(matches4.size(), 4);
          EXPECT_STREQ(matches4[0].str().c_str(), "@repeat on AnonStructures	{ Name , Def,Tab  }");
          EXPECT_STREQ(matches4[1].str().c_str(), "AnonStructures");
          EXPECT_STREQ(matches4[2].str().c_str(), "Name");
          EXPECT_STREQ(matches4[3].str().c_str(), " Def,Tab  ");

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
