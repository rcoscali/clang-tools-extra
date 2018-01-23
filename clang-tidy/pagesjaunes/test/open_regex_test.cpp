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

        TEST_F(OpenRegexTest, RegexMatching)
        {
#define REQ                                     \
          " EXEC SQL OPEN crsCountInsEPJ0; " 
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_open_re().match(req0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 5);
          if (retbool0 && retsize0 == 5)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL OPEN crsCountInsEPJ0;");
              EXPECT_STREQ(matches0[1].str().c_str(), "OPEN");
              EXPECT_STREQ(matches0[2].str().c_str(), "crsCountInsEPJ0");
              EXPECT_STREQ(matches0[3].str().c_str(), "");
              EXPECT_STREQ(matches0[4].str().c_str(), "");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  OPEN crsCountInsEPJ1; "
          
          llvm::StringRef req1(REQ);
          SmallVector<StringRef, 8> matches1;
          bool retbool1 = get_open_re().match(req1, &matches1);
          EXPECT_TRUE(retbool1);
          int retsize1 = matches1.size();
          EXPECT_EQ(retsize1, 5);
          if (retbool1 && retsize1 == 5)
            {
              EXPECT_STREQ(matches1[0].str().c_str(), "EXEC SQL\n  OPEN crsCountInsEPJ1;");
              EXPECT_STREQ(matches1[1].str().c_str(), "OPEN");
              EXPECT_STREQ(matches1[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches1[3].str().c_str(), "");
              EXPECT_STREQ(matches1[4].str().c_str(), "");
            }
#undef REQ

#define REQ                                     \
          "EXEC SQL OPEN crsCountInsEPJ2 USING :nTab1,:nTab2; " 
          
          llvm::StringRef req2(REQ);
          SmallVector<StringRef, 8> matches2;
          bool retbool2 = get_open_re().match(req2, &matches2);
          EXPECT_TRUE(retbool2);
          int retsize2 = matches2.size();
          EXPECT_EQ(retsize2, 5);
          if (retbool2 && retsize2 == 5)
            {
              EXPECT_STREQ(matches2[0].str().c_str(), "EXEC SQL OPEN crsCountInsEPJ2 USING :nTab1,:nTab2;");
              EXPECT_STREQ(matches2[1].str().c_str(), "OPEN");
              EXPECT_STREQ(matches2[2].str().c_str(), "crsCountInsEPJ2");
              EXPECT_STREQ(matches2[3].str().c_str(), "USING");
              EXPECT_STREQ(matches2[4].str().c_str(), ":nTab1,:nTab2");
            }
#undef REQ

#define REQ                                                             \
          "EXEC SQL\n"                                                  \
            "  OPEN crsCountInsEPJ1 USING :nTab1,:nTab2,nTab3;"
          
          llvm::StringRef req3(REQ);
          SmallVector<StringRef, 8> matches3;
          bool retbool3 = get_open_re().match(req3, &matches3);
          EXPECT_TRUE(retbool3);
          int retsize3 = matches3.size();
          EXPECT_EQ(retsize3, 5);
          if (retbool3 && retsize3 == 5)
            {
              EXPECT_STREQ(matches3[0].str().c_str(), "EXEC SQL\n  OPEN crsCountInsEPJ1 USING :nTab1,:nTab2,nTab3;");
              EXPECT_STREQ(matches3[1].str().c_str(), "OPEN");
              EXPECT_STREQ(matches3[2].str().c_str(), "crsCountInsEPJ1");
              EXPECT_STREQ(matches3[3].str().c_str(), "USING");
              EXPECT_STREQ(matches3[4].str().c_str(), ":nTab1,:nTab2,nTab3");
            }
#undef REQ
          
#define REQ                                                             \
          "EXEC SQL \n"                                                 \
            "open ghhcrsLireVersionIeinsc \n"                           \
            "using :pcOraNumnat,\n"                                     \
            ":pcOraNumlo,\n"                                            \
            ":pcOraNumls;"
          
          llvm::StringRef req4(REQ);
          SmallVector<StringRef, 8> matches4;
          bool retbool4 = get_open_re().match(req4, &matches4);
          EXPECT_TRUE(retbool4);
          int retsize4 = matches4.size();
          EXPECT_EQ(retsize4, 5);
          if (retbool4 && retsize4 == 5)
            {
              EXPECT_STREQ(matches4[0].str().c_str(), "EXEC SQL \nopen ghhcrsLireVersionIeinsc \nusing :pcOraNumnat,\n:pcOraNumlo,\n:pcOraNumls;");
              EXPECT_STREQ(matches4[1].str().c_str(), "open");
              EXPECT_STREQ(matches4[2].str().c_str(), "ghhcrsLireVersionIeinsc");
              EXPECT_STREQ(matches4[3].str().c_str(), "using");
              EXPECT_STREQ(matches4[4].str().c_str(), ":pcOraNumnat,\n:pcOraNumlo,\n:pcOraNumls");
            }
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
          bool retbool2 = get_open_re().match(reqweird2, &matches2);
          EXPECT_TRUE(retbool2);
          int retsize2 = matches2.size();
          EXPECT_EQ(retsize2, 5);
          if (retbool2 && retsize2 == 5)
            {
              EXPECT_STREQ(matches2[0].str().c_str(), "EXEC SQL \n  OPEN crsCountIns_EPJ0\n  ;");
              EXPECT_STREQ(matches2[1].str().c_str(), "OPEN");
              EXPECT_STREQ(matches2[2].str().c_str(), "crsCountIns_EPJ0");
              EXPECT_STREQ(matches2[3].str().c_str(), "");
              EXPECT_STREQ(matches2[4].str().c_str(), "");
            }
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
            "  UsInG: emp1, : emp2 "                                     \
            "  ; " 
          
          llvm::StringRef reqweird4(REQWEIRD);
          SmallVector<StringRef, 8> matches4;
          bool retbool4 = get_open_re().match(reqweird4, &matches4);
          EXPECT_TRUE(retbool4);
          int retsize4 = matches4.size();
          EXPECT_EQ(retsize4, 5);
          if (retbool4 && retsize4 == 5)
            {
              EXPECT_STREQ(matches4[0].str().c_str(), "EXEC SQL \n  OPEN __crsCount_Ins_EPJ_0__\n  UsInG: emp1, : emp2   ;");
              EXPECT_STREQ(matches4[1].str().c_str(), "OPEN");
              EXPECT_STREQ(matches4[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
              EXPECT_STREQ(matches4[3].str().c_str(), "UsInG");
              EXPECT_STREQ(matches4[4].str().c_str(), ": emp1, : emp2   ");
            }
#undef REQWEIRD
          
#define REQWEIRD                                                        \
          "	  EXEC 		\n"                                     \
            "	   SQL   \n"                                            \
            "  open __crsCount_Ins_EPJ_0__\n"                           \
            " UsiNG:  \n  "                                              \
            "  _emp1 , :\n "                                            \
            "  _emp2 , :\n "                                            \
            "  _emp3 \n "                                               \
            "  ; " 
          
          llvm::StringRef reqweird5(REQWEIRD);
          SmallVector<StringRef, 8> matches5;
          bool retbool5 = get_open_re().match(reqweird5, &matches5);
          EXPECT_TRUE(retbool5);
          int retsize5 = matches5.size();
          EXPECT_EQ(retsize5, 5);
          if (retbool5 && retsize5 == 5)
            {
              EXPECT_STREQ(matches5[0].str().c_str(), "EXEC 		\n	   SQL   \n  open __crsCount_Ins_EPJ_0__\n UsiNG:  \n    _emp1 , :\n   _emp2 , :\n   _emp3 \n   ;");
              EXPECT_STREQ(matches5[1].str().c_str(), "open");
              EXPECT_STREQ(matches5[2].str().c_str(), "__crsCount_Ins_EPJ_0__");
              EXPECT_STREQ(matches5[3].str().c_str(), "UsiNG");
              EXPECT_STREQ(matches5[4].str().c_str(), ":  \n    _emp1 , :\n   _emp2 , :\n   _emp3 \n   ");
            }
#undef REQWEIRD
          
        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
