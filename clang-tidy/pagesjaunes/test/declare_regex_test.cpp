//===------- declare_regex_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "declare_regex_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        DeclareRegexTest::DeclareRegexTest()
          : declare_re_m(PAGESJAUNES_REGEX_EXEC_SQL_DECLARE_REQ_RE)
        {
        }

        DeclareRegexTest::~DeclareRegexTest()
        {
        }

        void
        DeclareRegexTest::SetUp(void)
        {
        }
        
        void
        DeclareRegexTest::TearDown(void)
        {
        }
        
        void
        DeclareRegexTest::PrintTo(const DeclareRegexTest& declare_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(DeclareRegexTest, RegexMatchingIndicators)
        {
#define REQ                                                             \
          "EXEC SQL DECLARE crsCountInsEPJ0 cursor for reqCountInsEPJ0; " 
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          EXPECT_TRUE(get_declare_re().match(req0, &matches0));
          EXPECT_EQ(matches0.size(), 6);
          EXPECT_STREQ(matches0[1].str().c_str(), "DECLARE");
          EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
          EXPECT_STREQ(matches0[3].str().c_str(), "cursor");
          EXPECT_STREQ(matches0[4].str().c_str(), "for");
          EXPECT_STREQ(matches0[5].str().c_str(), "reqCountInsEPJ0");
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  DECLARE cursor crsCountInsEPJ1 \n"                       \
            "  for  reqCountInsEPJ1;"
          
          llvm::StringRef req1(REQ);
          SmallVector<StringRef, 8> matches1;
          EXPECT_TRUE(get_declare_re().match(req1, &matches1));
          EXPECT_EQ(matches1.size(), 6);
          EXPECT_STREQ(matches1[1].str().c_str(), "DECLARE");
          EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
          EXPECT_STREQ(matches1[3].str().c_str(), "cursor");
          EXPECT_STREQ(matches1[4].str().c_str(), "for");
          EXPECT_STREQ(matches1[5].str().c_str(), "reqCountInsEPJ1");
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  DECLARE \n"                                              \
            "    cursor 	\n"                                     \
            "           crsCountInsEPJ1    \n"                          \
            "  for \n"                                                  \
            "    reqCountInsEPJ1;"
          
          llvm::StringRef req2(REQ);
          SmallVector<StringRef, 8> matches2;
          EXPECT_TRUE(get_declare_re().match(req2, &matches2));
          EXPECT_EQ(matches2.size(), 6);
          EXPECT_STREQ(matches2[1].str().c_str(), "DECLARE");
          EXPECT_STREQ(matches2[2].str().c_str(), "crsCountInsEPJ1");
          EXPECT_STREQ(matches2[3].str().c_str(), "cursor");
          EXPECT_STREQ(matches2[4].str().c_str(), "for");
          EXPECT_STREQ(matches2[5].str().c_str(), "reqCountInsEPJ1");
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  DECLARE \n"                                              \
            "    cursor 	\n"                                     \
            "           crsCountInsEPJ1    \n"                          \
            "  for \n"                                                  \
            "    reqCountInsEPJ1;"
          
          llvm::StringRef req3(REQ);
          SmallVector<StringRef, 8> matches3;
          EXPECT_TRUE(get_declare_re().match(req3, &matches3));
          EXPECT_EQ(matches3.size(), 6);
          EXPECT_STREQ(matches3[1].str().c_str(), "DECLARE");
          EXPECT_STREQ(matches3[2].str().c_str(), "crsCountInsEPJ1");
          EXPECT_STREQ(matches3[3].str().c_str(), "cursor");
          EXPECT_STREQ(matches3[4].str().c_str(), "for");
          EXPECT_STREQ(matches3[5].str().c_str(), "reqCountInsEPJ1");
#undef REQ
        }

        TEST_F(DeclareRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DEcLARE cURsOr _crsCountIns_EPJ0\n"                      \
            "   FoR _req_Count1_InsEPJ0\n"                              \
            "  ; " 
          
          llvm::StringRef reqweird4(REQWEIRD);
          SmallVector<StringRef, 8> matches4;
          EXPECT_TRUE(get_declare_re().match(reqweird4, &matches4));
          EXPECT_EQ(matches4.size(), 3);
          EXPECT_STREQ(matches4[1].str().c_str(), "DEcLARE");
          EXPECT_STREQ(matches4[2].str().c_str(), "_crsCountIns_EPJ0");
          EXPECT_STREQ(matches4[3].str().c_str(), "cURsOr");
          EXPECT_STREQ(matches4[4].str().c_str(), "FoR");
          EXPECT_STREQ(matches4[5].str().c_str(), "_req_Count1_InsEPJ0");
#undef REQWEIRD

#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE cursor 1crsCountInsEPJ0\n"                       \
            "  for reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird5(REQWEIRD);
          SmallVector<StringRef, 8> matches5;
          EXPECT_FALSE(get_declare_re().match(reqweird5, &matches5));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE cursor crsCountInsEPJ0\n"                        \
            "  for 1reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird6(REQWEIRD);
          SmallVector<StringRef, 8> matches6;
          EXPECT_FALSE(get_declare_re().match(reqweird6, &matches6));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE cursor -crsCountInsEPJ0\n"                       \
            "  for reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird7(REQWEIRD);
          SmallVector<StringRef, 8> matches7;
          EXPECT_FALSE(get_declare_re().match(reqweird7, &matches7));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE cursor crsCountInsEPJ0\n"                        \
            "  for req-CountInsEPJ0; " 
          
          llvm::StringRef reqweird8(REQWEIRD);
          SmallVector<StringRef, 8> matches8;
          EXPECT_FALSE(get_declare_re().match(reqweird8, &matches8));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE cUrsor __crsCount_Ins_EPJ_0__\n"                 \
            "  fOr __req_CountInsEPJ_0__; " 
          
          llvm::StringRef reqweird9(REQWEIRD);
          SmallVector<StringRef, 8> matches9;
          EXPECT_TRUE(get_declare_re().match(reqweird9, &matches9));
          EXPECT_EQ(matches9.size(), 6);
          EXPECT_STREQ(matches9[1].str().c_str(), "DECLARE");
          EXPECT_STREQ(matches9[2].str().c_str(), "cUrsor");
          EXPECT_STREQ(matches9[3].str().c_str(), "__crsCount_Ins_EPJ_0__");
          EXPECT_STREQ(matches9[4].str().c_str(), "fOr");
          EXPECT_STREQ(matches9[5].str().c_str(), "__req_CountInsEPJ_0__");
#undef REQWEIRD
          
        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
