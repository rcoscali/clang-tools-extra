//===------- fetch_decode_host_var.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "fetch_decode_host_var.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        FetchDecodeHostVar::FetchDecodeHostVar()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE)
        {
        }

        FetchDecodeHostVar::~FetchDecodeHostVar()
        {
        }

        void
        FetchDecodeHostVar::SetUp(void)
        {
        }
        
        void
        FetchDecodeHostVar::TearDown(void)
        {
        }
        
        void
        FetchDecodeHostVar::PrintTo(const FetchDecodeHostVar& fetch_decode_host_var, ::std::ostream* os)
        {
        }

        TEST_F(FetchDecodeHostVar, RegexMatchingIndicators)
        {
#define REQ0                                                            \
            "    EXEC SQL\n"                                            \
            "    fetch crsStandard\n"                                   \
            "    into :prOraInscr->acDenom\n"                           \
            "    :prIndInscr->sDenomI,\n"                               \
            "    :prOraInscr->acCompln:prIndInscr->sComplnI,\n"         \
            "    :prOraInscr->acDesign:prIndInscr->sDesignI,\n"         \
            "    :prOraInscr->acPrenom:prIndInscr->sPrenomI,\n"         \
            "    :prOraInscr->acLaQualite:prIndInscr->sLaQualiteI;\n"
          
          llvm::StringRef req0(REQ0);
          SmallVector<StringRef, 8> matches0;
          bool retbool0 = get_fetch_re().match(req0, &matches0);
          EXPECT_TRUE(retbool0);
          size_t retsize0 = matches0.size();
          EXPECT_EQ(retsize0, 5);
          if (retbool0 && retsize0 == 5)
            {
              EXPECT_STREQ(matches0[0].str().c_str(), "EXEC SQL\n    fetch crsStandard\n    into :prOraInscr->acDenom\n    :prIndInscr->sDenomI,\n    :prOraInscr->acCompln:prIndInscr->sComplnI,\n    :prOraInscr->acDesign:prIndInscr->sDesignI,\n    :prOraInscr->acPrenom:prIndInscr->sPrenomI,\n    :prOraInscr->acLaQualite:prIndInscr->sLaQualiteI;");
              EXPECT_STREQ(matches0[1].str().c_str(), "fetch");
              EXPECT_STREQ(matches0[2].str().c_str(), "crsStandard");
              EXPECT_STREQ(matches0[3].str().c_str(), "into");
              EXPECT_STREQ(matches0[4].str().c_str(), ":prOraInscr->acDenom\n    :prIndInscr->sDenomI,\n    :prOraInscr->acCompln:prIndInscr->sComplnI,\n    :prOraInscr->acDesign:prIndInscr->sDesignI,\n    :prOraInscr->acPrenom:prIndInscr->sPrenomI,\n    :prOraInscr->acLaQualite:prIndInscr->sLaQualiteI");

              map_host_vars hv;
              hv = clang::tidy::pagesjaunes::decodeHostVars(matches0[4]);

              // Total vars = 5
              EXPECT_EQ(hv.size(), 5);
              
              // var #1
              //     full = ':prOraInscr->acDenom'
              //     fulli = ':prIndInscr->sDenomI,'
              //     hostvar = 'prOraInscr->acDenom'
              //     hostvari = 'prIndInscr->sDenomI'
              //     hostrecord = 'prOraInscr'
              //     hostrecordi = 'prIndInscr'
              //     hostmember = 'acDenom'
              //     hostmemberi = 'sDenomI'
              //     deref = '->'
              //     derefi = '->'
              EXPECT_STREQ(hv[1]["full"].c_str(), ":prOraInscr->acDenom");
              EXPECT_STREQ(hv[1]["hostvar"].c_str(), "prOraInscr->acDenom");
              EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "prOraInscr");
              EXPECT_STREQ(hv[1]["hostmember"].c_str(), "acDenom");
              EXPECT_STREQ(hv[1]["deref"].c_str(), "->");
              
              EXPECT_STREQ(hv[1]["fulli"].c_str(), ":prIndInscr->sDenomI,");
              EXPECT_STREQ(hv[1]["hostvari"].c_str(), "prIndInscr->sDenomI");
              EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "prIndInscr");
              EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "sDenomI");
              EXPECT_STREQ(hv[1]["derefi"].c_str(), "->");

              // var #2
              //     full = ':prOraInscr->acCompln'
              //     fulli = ':prIndInscr->sComplnI,'
              //     hostvar = 'prOraInscr->acCompln'
              //     hostvari = 'prIndInscr->sComplnI'
              //     hostrecord = 'prOraInscr'
              //     hostrecordi = 'prIndInscr'
              //     hostmember = 'acCompln'
              //     hostmemberi = 'sComplnI'
              //     deref = '->'
              //     derefi = '->'
              EXPECT_STREQ(hv[2]["full"].c_str(), ":prOraInscr->acCompln");
              EXPECT_STREQ(hv[2]["hostvar"].c_str(), "prOraInscr->acCompln");
              EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "prOraInscr");
              EXPECT_STREQ(hv[2]["hostmember"].c_str(), "acCompln");
              EXPECT_STREQ(hv[2]["deref"].c_str(), "->");
              
              EXPECT_STREQ(hv[2]["fulli"].c_str(), ":prIndInscr->sComplnI,");
              EXPECT_STREQ(hv[2]["hostvari"].c_str(), "prIndInscr->sComplnI");
              EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "prIndInscr");
              EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "sComplnI");
              EXPECT_STREQ(hv[2]["derefi"].c_str(), "->");

              // var #3
              //     full = ':prOraInscr->acDesign'
              //     fulli = ':prIndInscr->sDesignI,'
              //     hostvar = 'prOraInscr->acDesign'
              //     hostvari = 'prIndInscr->sDesignI'
              //     hostrecord = 'prOraInscr'
              //     hostrecordi = 'prIndInscr'
              //     hostmember = 'acDesign'
              //     hostmemberi = 'sDesignI'
              //     deref = '->'
              //     derefi = '->'
              EXPECT_STREQ(hv[3]["full"].c_str(), ":prOraInscr->acDesign");
              EXPECT_STREQ(hv[3]["hostvar"].c_str(), "prOraInscr->acDesign");
              EXPECT_STREQ(hv[3]["hostrecord"].c_str(), "prOraInscr");
              EXPECT_STREQ(hv[3]["hostmember"].c_str(), "acDesign");
              EXPECT_STREQ(hv[3]["deref"].c_str(), "->");
              
              EXPECT_STREQ(hv[3]["fulli"].c_str(), ":prIndInscr->sDesignI,");
              EXPECT_STREQ(hv[3]["hostvari"].c_str(), "prIndInscr->sDesignI");
              EXPECT_STREQ(hv[3]["hostrecordi"].c_str(), "prIndInscr");
              EXPECT_STREQ(hv[3]["hostmemberi"].c_str(), "sDesignI");
              EXPECT_STREQ(hv[3]["derefi"].c_str(), "->");

              // var #4
              //     full = ':prOraInscr->acPrenom'
              //     fulli = ':prIndInscr->sPrenomI,'
              //     hostvar = 'prOraInscr->acPrenom'
              //     hostvari = 'prIndInscr->sPrenomI'
              //     hostrecord = 'prOraInscr'
              //     hostrecordi = 'prIndInscr'
              //     hostmember = 'acPrenom'
              //     hostmemberi = 'sPrenomI'
              //     deref = '->'
              //     derefi = '->'
              EXPECT_STREQ(hv[4]["full"].c_str(), ":prOraInscr->acPrenom");
              EXPECT_STREQ(hv[4]["hostvar"].c_str(), "prOraInscr->acPrenom");
              EXPECT_STREQ(hv[4]["hostrecord"].c_str(), "prOraInscr");
              EXPECT_STREQ(hv[4]["hostmember"].c_str(), "acPrenom");
              EXPECT_STREQ(hv[4]["deref"].c_str(), "->");
              
              EXPECT_STREQ(hv[4]["fulli"].c_str(), ":prIndInscr->sPrenomI,");
              EXPECT_STREQ(hv[4]["hostvari"].c_str(), "prIndInscr->sPrenomI");
              EXPECT_STREQ(hv[4]["hostrecordi"].c_str(), "prIndInscr");
              EXPECT_STREQ(hv[4]["hostmemberi"].c_str(), "sPrenomI");
              EXPECT_STREQ(hv[4]["derefi"].c_str(), "->");

              // var #5
              //     full = ':prOraInscr->acLaQualite'
              //     fulli = ':prIndInscr->sLaQualiteI'
              //     hostvar = 'prOraInscr->acLaQualite'
              //     hostvari = 'prIndInscr->sLaQualiteI'
              //     hostrecord = 'prOraInscr'
              //     hostrecordi = 'prIndInscr'
              //     hostmember = 'acLaQualite'
              //     hostmemberi = 'sLaQualiteI'
              //     deref = '->'
              //     derefi = '->'
              EXPECT_STREQ(hv[5]["full"].c_str(), ":prOraInscr->acLaQualite");
              EXPECT_STREQ(hv[5]["hostvar"].c_str(), "prOraInscr->acLaQualite");
              EXPECT_STREQ(hv[5]["hostrecord"].c_str(), "prOraInscr");
              EXPECT_STREQ(hv[5]["hostmember"].c_str(), "acLaQualite");
              EXPECT_STREQ(hv[5]["deref"].c_str(), "->");
              
              EXPECT_STREQ(hv[5]["fulli"].c_str(), ":prIndInscr->sLaQualiteI");
              EXPECT_STREQ(hv[5]["hostvari"].c_str(), "prIndInscr->sLaQualiteI");
              EXPECT_STREQ(hv[5]["hostrecordi"].c_str(), "prIndInscr");
              EXPECT_STREQ(hv[5]["hostmemberi"].c_str(), "sLaQualiteI");
              EXPECT_STREQ(hv[5]["derefi"].c_str(), "->");
     
            }
        }

      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
