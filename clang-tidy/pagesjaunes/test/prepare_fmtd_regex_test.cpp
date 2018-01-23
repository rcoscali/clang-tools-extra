//===------- prepare_fmtd_regex_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "prepare_fmtd_regex_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        PrepareFmtdRegexTest::PrepareFmtdRegexTest()
          : prepare_fmtd_re_m(PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_FMTD_REQ_RE)
        {
        }

        PrepareFmtdRegexTest::~PrepareFmtdRegexTest()
        {
        }

        void
        PrepareFmtdRegexTest::SetUp(void)
        {
        }
        
        void
        PrepareFmtdRegexTest::TearDown(void)
        {
        }
        
        void
        PrepareFmtdRegexTest::PrintTo(const PrepareFmtdRegexTest& prepare_fmtd_regex_test, ::std::ostream* os)
        {
        }

        TEST_F(PrepareFmtdRegexTest, RegexMatching)
        {
#define REQ                                                             \
          "EXEC SQL PREPARE my_statement FROM :my_string;" 
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_prepare_fmtd_re().match(req0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 6);
          if (retbool0 && retsize0 == 6)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL PREPARE my_statement FROM :my_string;");
              EXPECT_STREQ(matches0[1].str().c_str(), "PREPARE");
              EXPECT_STREQ(matches0[2].str().c_str(), "my_statement");
              EXPECT_STREQ(matches0[3].str().c_str(), "FROM");
              EXPECT_STREQ(matches0[4].str().c_str(), ":my_string");
              EXPECT_STREQ(matches0[5].str().c_str(), "my_string");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  PREPARE crsCountInsEPJ1 \n"                       \
            "  FROM  reqCountInsEPJ1;"
          
          llvm::StringRef req1(REQ);
          SmallVector<StringRef, 8> matches1;
          bool retbool1 = get_prepare_fmtd_re().match(req1, &matches1);
          EXPECT_TRUE(retbool1);
          int retsize1 = matches1.size();
          EXPECT_EQ(retsize1, 6);
          if (retbool1 && retsize1 == 6)
            {
              EXPECT_STREQ(matches1[0].str().c_str(), "EXEC SQL\n  PREPARE crsCountInsEPJ1 \n  FROM  reqCountInsEPJ1;");
              EXPECT_STREQ(matches1[1].str().c_str(), "PREPARE");
              EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches1[3].str().c_str(), "FROM");
              EXPECT_STREQ(matches1[4].str().c_str(), "reqCountInsEPJ1");
              EXPECT_STREQ(matches1[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  PREPARE \n"                                              \
            "           crsCountInsEPJ1    \n"                          \
            "  from \n"                                                  \
            "    :reqCountInsEPJ1;"
          
          llvm::StringRef req2(REQ);
          SmallVector<StringRef, 8> matches2;
          bool retbool2 = get_prepare_fmtd_re().match(req2, &matches2);
          EXPECT_TRUE(retbool2);
          int retsize2 = matches2.size();
          EXPECT_EQ(retsize2, 6);
          if (retbool2 && retsize2 == 6)
            {
              EXPECT_STREQ(matches2[0].str().c_str(), "EXEC SQL\n  PREPARE \n           crsCountInsEPJ1    \n  from \n    :reqCountInsEPJ1;");
              EXPECT_STREQ(matches2[1].str().c_str(), "PREPARE");
              EXPECT_STREQ(matches2[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches2[3].str().c_str(), "from");
              EXPECT_STREQ(matches2[4].str().c_str(), ":reqCountInsEPJ1");
              EXPECT_STREQ(matches2[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  PREPARE \n"                                              \
            "           crsCountInsEPJ1    \n"                          \
            "  FROM \n"                                                 \
            "    :reqCountInsEPJ1;"
          
          llvm::StringRef req3(REQ);
          SmallVector<StringRef, 8> matches3;
          bool retbool3 = get_prepare_fmtd_re().match(req3, &matches3);
          EXPECT_TRUE(retbool3);
          int retsize3 = matches3.size();
          EXPECT_EQ(retsize3, 6);
          if (retbool3 && retsize3 == 6)
            {
              EXPECT_STREQ(matches3[0].str().c_str(), "EXEC SQL\n  PREPARE \n           crsCountInsEPJ1    \n  FROM \n    :reqCountInsEPJ1;");
              EXPECT_STREQ(matches3[1].str().c_str(), "PREPARE");
              EXPECT_STREQ(matches3[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches3[3].str().c_str(), "FROM");
              EXPECT_STREQ(matches3[4].str().c_str(), ":reqCountInsEPJ1");
              EXPECT_STREQ(matches3[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQ
        }

        TEST_F(PrepareFmtdRegexTest, RegexMatchingWeirdSyntax)
        {
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  Prepare _crsCountIns_EPJ0 \n"                            \
            "   FRom :_req_Count1_InsEPJ0\n"                            \
            "  ; " 
          
          llvm::StringRef reqweird4(REQWEIRD);
          SmallVector<StringRef, 8> matches4;
          bool retbool4 = get_prepare_fmtd_re().match(reqweird4, &matches4);
          EXPECT_TRUE(retbool4);
          int retsize4 = matches4.size();
          EXPECT_EQ(retsize4, 6);
          if (retbool4 && retsize4 == 6)
            {
              EXPECT_STREQ(matches4[0].str().c_str(), "EXEC SQL \n  Prepare _crsCountIns_EPJ0 \n   FRom :_req_Count1_InsEPJ0\n  ;");
              EXPECT_STREQ(matches4[1].str().c_str(), "Prepare");
              EXPECT_STREQ(matches4[2].str().c_str(), "_crsCountIns_EPJ0");
              EXPECT_STREQ(matches4[3].str().c_str(), "FRom");
              EXPECT_STREQ(matches4[4].str().c_str(), ":_req_Count1_InsEPJ0\n  ");
              EXPECT_STREQ(matches4[5].str().c_str(), "_req_Count1_InsEPJ0");
            }
#undef REQWEIRD

#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  PREPARE_FMTD 1crsCountInsEPJ0 cursor\n"                  \
            "  for reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird5(REQWEIRD);
          SmallVector<StringRef, 8> matches5;
          EXPECT_FALSE(get_prepare_fmtd_re().match(reqweird5, &matches5));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  PREPARE_FMTD crsCountInsEPJ0 cursor\n"                   \
            "  for 1reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird6(REQWEIRD);
          SmallVector<StringRef, 8> matches6;
          EXPECT_FALSE(get_prepare_fmtd_re().match(reqweird6, &matches6));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  PREPARE_FMTD -crsCountInsEPJ0 cursor\n"                       \
            "  for reqCountInsEPJ0; " 
          
          llvm::StringRef reqweird7(REQWEIRD);
          SmallVector<StringRef, 8> matches7;
          EXPECT_FALSE(get_prepare_fmtd_re().match(reqweird7, &matches7));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  PREPARE_FMTD crsCountInsEPJ0 cursor\n"                        \
            "  for req-CountInsEPJ0; " 
          
          llvm::StringRef reqweird8(REQWEIRD);
          SmallVector<StringRef, 8> matches8;
          EXPECT_FALSE(get_prepare_fmtd_re().match(reqweird8, &matches8));
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC SQL \n"                                          \
            "  PrePARE __crsCount_Ins_EPJ_0__\n"                        \
            "  fRoM :__req_CountInsEPJ_0__; " 
          
          llvm::StringRef reqweird9(REQWEIRD);
          SmallVector<StringRef, 8> matches9;
          bool retbool9 = get_prepare_fmtd_re().match(reqweird9, &matches9);
          EXPECT_TRUE(retbool9);
          int retsize9 = matches9.size();
          EXPECT_EQ(retsize9, 6);
          if (retbool9 && retsize9 == 6)
            {
              EXPECT_STREQ(matches9[0].str().c_str(), "EXEC SQL \n  PrePARE __crsCount_Ins_EPJ_0__\n  fRoM :__req_CountInsEPJ_0__;");
              EXPECT_STREQ(matches9[1].str().c_str(), "PrePARE");
              EXPECT_STREQ(matches9[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
              EXPECT_STREQ(matches9[3].str().c_str(), "fRoM");
              EXPECT_STREQ(matches9[4].str().c_str(), ":__req_CountInsEPJ_0__");
              EXPECT_STREQ(matches9[5].str().c_str(), "__req_CountInsEPJ_0__");
            }
#undef REQWEIRD
          
        }

        TEST_F(PrepareFmtdRegexTest, RegexMatchingBadColonSyntax)
        {
#define REQCOLON                                                        \
          "EXEC SQL \n"                                                 \
            " PREPARE crsCountInsEPJ0 \n"                               \
            " FROM: reqCountInsEPJ0;" 
          
          llvm::StringRef reqcolon0(REQCOLON);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_prepare_fmtd_re().match(reqcolon0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 6);
          if (retbool0 && retsize0 == 6)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL \n PREPARE crsCountInsEPJ0 \n FROM: reqCountInsEPJ0;");
              EXPECT_STREQ(matches0[1].str().c_str(), "PREPARE");
              EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
              EXPECT_STREQ(matches0[3].str().c_str(), "FROM");
              EXPECT_STREQ(matches0[4].str().c_str(), ": reqCountInsEPJ0");
              EXPECT_STREQ(matches0[5].str().c_str(), "reqCountInsEPJ0");
            }
#undef REQCOLON

#define REQCOLON                                                        \
          "EXEC SQL \n"                                                 \
            " PREPARE crsCountInsEPJ1 \n"                               \
            " FROM: reqCountInsEPJ1 \n ;" 
          
          llvm::StringRef reqcolon1(REQCOLON);
          SmallVector<StringRef, 8> matches1;
          bool retbool1 = get_prepare_fmtd_re().match(reqcolon1, &matches1);
          EXPECT_TRUE(retbool1);
          int retsize1 = matches1.size();
          EXPECT_EQ(retsize1, 6);
          if (retbool1 && retsize1 == 6)
            {
              EXPECT_STREQ(matches1[0].str().c_str(), "EXEC SQL \n PREPARE crsCountInsEPJ1 \n FROM: reqCountInsEPJ1 \n ;");
              EXPECT_STREQ(matches1[1].str().c_str(), "PREPARE");
              EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches1[3].str().c_str(), "FROM");
              EXPECT_STREQ(matches1[4].str().c_str(), ": reqCountInsEPJ1 \n ");
              EXPECT_STREQ(matches1[5].str().c_str(), "reqCountInsEPJ1");
            }
#undef REQCOLON
        }
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
