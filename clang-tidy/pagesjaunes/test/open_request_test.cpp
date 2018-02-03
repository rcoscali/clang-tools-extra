//===------- open_request_test.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "open_request_test.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        OpenRequestTest::OpenRequestTest()
          : open_re_m(PAGESJAUNES_REGEX_EXEC_SQL_OPEN_REQ_RE)
        {
        }

        OpenRequestTest::~OpenRequestTest()
        {
        }

        void
        OpenRequestTest::SetUp(void)
        {
        }
        
        void
        OpenRequestTest::TearDown(void)
        {
        }
        
        void
        OpenRequestTest::PrintTo(const OpenRequestTest& open_request_test, ::std::ostream* os)
        {
        }

        TEST_F(OpenRequestTest, RequestDecode)
        {
#define REQ                                             \
          "        EXEC SQL\n"                          \
            "          open ghhcrsLireVersionIeinsc\n"  \
            "          using :pcOraNumnat,\n"           \
            "          :pcOraNumlo,\n"                  \
            "          :pcOraNumls;"
          
          llvm::StringRef req0(REQ);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_open_re().match(req0, &matches0);
          EXPECT_TRUE(retbool0);
          int retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 5);
          if (retbool0 && retsize0 == 5)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL\n          open ghhcrsLireVersionIeinsc\n          using :pcOraNumnat,\n          :pcOraNumlo,\n          :pcOraNumls;");
              EXPECT_STREQ(matches0[1].str().c_str(), "open");
              EXPECT_STREQ(matches0[2].str().c_str(), "ghhcrsLireVersionIeinsc");
              EXPECT_STREQ(matches0[3].str().c_str(), "using");
              EXPECT_STREQ(matches0[4].str().c_str(), ":pcOraNumnat,\n          :pcOraNumlo,\n          :pcOraNumls");
            }

          map_host_vars hv = clang::tidy::pagesjaunes::decodeHostVars(matches0[4]);

          EXPECT_STREQ(hv[1]["full"].c_str(), ":pcOraNumnat,");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "pcOraNumnat");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "pcOraNumnat");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "pcOraNumnat");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "");
          
          EXPECT_STREQ(hv[2]["full"].c_str(), ":pcOraNumlo,");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "pcOraNumlo");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "pcOraNumlo");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "pcOraNumlo");
          EXPECT_STREQ(hv[2]["deref"].c_str(), "");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "");
          
          EXPECT_STREQ(hv[3]["full"].c_str(), ":pcOraNumls");
          EXPECT_STREQ(hv[3]["hostvar"].c_str(), "pcOraNumls");
          EXPECT_STREQ(hv[3]["hostrecord"].c_str(), "pcOraNumls");
          EXPECT_STREQ(hv[3]["hostmember"].c_str(), "pcOraNumls");
          EXPECT_STREQ(hv[3]["deref"].c_str(), "");

          EXPECT_STREQ(hv[3]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[3]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[3]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[3]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[3]["derefi"].c_str(), "");
          
#undef REQ

        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
