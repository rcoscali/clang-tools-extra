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

        TEST_F(DeclareRegexTest, RegexMatching)
        {
#define REQ                                                             \
          "EXEC SQL DECLARE crsCountInsEPJ0 cursor for reqCountInsEPJ0; " 
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_declare_re().match(req0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 6);
          if (retbool0 && retsize0 == 6)
            {
              EXPECT_STREQ(matches0[1].str().c_str(), "DECLARE");
              EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
              EXPECT_STREQ(matches0[3].str().c_str(), "cursor");
              EXPECT_STREQ(matches0[4].str().c_str(), "for");
              EXPECT_STREQ(matches0[5].str().c_str(), "reqCountInsEPJ0");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  DECLARE crsCountInsEPJ1 cursor \n"                       \
            "  for  reqCountInsEPJ1;"
          
          llvm::StringRef req1(REQ);
          SmallVector<StringRef, 8> matches1;
          bool retbool1 = get_declare_re().match(req1, &matches1);
          EXPECT_TRUE(retbool1);
          int retsize1 = matches1.size();
          EXPECT_EQ(retsize1, 6);
          if (retbool1 && retsize1 == 6)
            {
              EXPECT_STREQ(matches1[1].str().c_str(), "DECLARE");
              EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches1[3].str().c_str(), "cursor");
              EXPECT_STREQ(matches1[4].str().c_str(), "for");
              EXPECT_STREQ(matches1[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  DECLARE \n"                                              \
            "           crsCountInsEPJ1    \n"                          \
            "    cursor 	\n"                                     \
            "  for \n"                                                  \
            "    reqCountInsEPJ1;"
          
          llvm::StringRef req2(REQ);
          SmallVector<StringRef, 8> matches2;
          bool retbool2 = get_declare_re().match(req2, &matches2);
          EXPECT_TRUE(retbool2);
          int retsize2 = matches2.size();
          EXPECT_EQ(retsize2, 6);
          if (retbool2 && retsize2 == 6)
            {
              EXPECT_STREQ(matches2[1].str().c_str(), "DECLARE");
              EXPECT_STREQ(matches2[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches2[3].str().c_str(), "cursor");
              EXPECT_STREQ(matches2[4].str().c_str(), "for");
              EXPECT_STREQ(matches2[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  DECLARE \n"                                              \
            "           crsCountInsEPJ1    \n"                          \
            "    cursor 	\n"                                     \
            "  for \n"                                                  \
            "    reqCountInsEPJ1;"
          
          llvm::StringRef req3(REQ);
          SmallVector<StringRef, 8> matches3;
          bool retbool3 = get_declare_re().match(req3, &matches3);
          EXPECT_TRUE(retbool3);
          int retsize3 = matches3.size();
          EXPECT_EQ(retsize3, 6);
          if (retbool3 && retsize3 == 6)
            {
              EXPECT_STREQ(matches3[1].str().c_str(), "DECLARE");
              EXPECT_STREQ(matches3[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches3[3].str().c_str(), "cursor");
              EXPECT_STREQ(matches3[4].str().c_str(), "for");
              EXPECT_STREQ(matches3[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQ
        }

        TEST_F(DeclareRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DEcLARE _crsCountIns_EPJ0 cURsOr\n"                      \
            "   FoR _req_Count1_InsEPJ0\n"                              \
            "  ; " 
          
          llvm::StringRef reqweird4(REQWEIRD);
          SmallVector<StringRef, 8> matches4;
          bool retbool4 = get_declare_re().match(reqweird4, &matches4);
          EXPECT_TRUE(retbool4);
          int retsize4 = matches4.size();
          EXPECT_EQ(retsize4, 6);
          if (retbool4 && retsize4 == 6)
            {
              EXPECT_STREQ(matches4[1].str().c_str(), "DEcLARE");
              EXPECT_STREQ(matches4[2].str().c_str(), "_crsCountIns_EPJ0");
              EXPECT_STREQ(matches4[3].str().c_str(), "cURsOr");
              EXPECT_STREQ(matches4[4].str().c_str(), "FoR");
              EXPECT_STREQ(matches4[5].str().c_str(), "_req_Count1_InsEPJ0");
            }
#undef REQWEIRD

#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE 1crsCountInsEPJ0 cursor\n"                       \
            "  for reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird5(REQWEIRD);
          SmallVector<StringRef, 8> matches5;
          EXPECT_FALSE(get_declare_re().match(reqweird5, &matches5));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE crsCountInsEPJ0 cursor\n"                        \
            "  for 1reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird6(REQWEIRD);
          SmallVector<StringRef, 8> matches6;
          EXPECT_FALSE(get_declare_re().match(reqweird6, &matches6));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE -crsCountInsEPJ0 cursor\n"                       \
            "  for reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird7(REQWEIRD);
          SmallVector<StringRef, 8> matches7;
          EXPECT_FALSE(get_declare_re().match(reqweird7, &matches7));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE crsCountInsEPJ0 cursor\n"                        \
            "  for req-CountInsEPJ0; " 
          
          llvm::StringRef reqweird8(REQWEIRD);
          SmallVector<StringRef, 8> matches8;
          EXPECT_FALSE(get_declare_re().match(reqweird8, &matches8));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  DECLARE __crsCount_Ins_EPJ_0__\n"                        \
            "  cUrsor  fOr __req_CountInsEPJ_0__; " 
          
          llvm::StringRef reqweird9(REQWEIRD);
          SmallVector<StringRef, 8> matches9;
          bool retbool9 = get_declare_re().match(reqweird9, &matches9);
          EXPECT_TRUE(retbool9);
          int retsize9 = matches9.size();
          EXPECT_EQ(retsize9, 6);
          if (retbool9 && retsize9 == 6)
            {
              EXPECT_STREQ(matches9[1].str().c_str(), "DECLARE");
              EXPECT_STREQ(matches9[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
              EXPECT_STREQ(matches9[3].str().c_str(), "cUrsor");
              EXPECT_STREQ(matches9[4].str().c_str(), "fOr");
              EXPECT_STREQ(matches9[5].str().c_str(), "__req_CountInsEPJ_0__");
            }
#undef REQWEIRD
          
        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
