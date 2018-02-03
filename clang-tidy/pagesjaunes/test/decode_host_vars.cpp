//===------- decode_host_vars.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "decode_host_vars.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        DecodeHostVarsTest::DecodeHostVarsTest()
          : fetch_re_m(PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE)
        {
        }

        DecodeHostVarsTest::~DecodeHostVarsTest()
        {
        }

        void
        DecodeHostVarsTest::SetUp(void)
        {
        }
        
        void
        DecodeHostVarsTest::TearDown(void)
        {
        }
        
        void
        DecodeHostVarsTest::PrintTo(const DecodeHostVarsTest& decode_host_vars, ::std::ostream* os)
        {
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsBasic)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":var1, :var2");

          // Total vars = 2
          EXPECT_EQ(hv.size(), 2);
          
          // var #1
          //     full = ':var1,'
          //     fulli = ''
          //     hostvar = 'var1'
          //     hostvari = ''
          //     hostrecord = 'var1'
          //     hostrecordi = ''
          //     hostmember = 'var1'
          //     hostmemberi = ''
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[1]["full"].c_str(), ":var1,");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "");

          // var #2
          //     full = ':var2'
          //     fulli = ''
          //     hostvar = 'var2'
          //     hostvari = ''
          //     hostrecord = 'var2'
          //     hostrecordi = ''
          //     hostmember = 'var2'
          //     hostmemberi = ''
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[2]["full"].c_str(), ":var2");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), "");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsBasic2)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":var1, :var2,:var3");

          // Total vars = 3
          EXPECT_EQ(hv.size(), 3);
          
          // var #1
          //     full = ':var1, '
          //     fulli = ''
          //     hostvar = 'var1'
          //     hostvari = ''
          //     hostrecord = 'var1'
          //     hostrecordi = ''
          //     hostmember = 'var1'
          //     hostmemberi = ''
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[1]["full"].c_str(), ":var1,");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "");

          // var #2
          //     full = ':var2,'
          //     fulli = ''
          //     hostvar = 'var2'
          //     hostvari = ''
          //     hostrecord = 'var2'
          //     hostrecordi = ''
          //     hostmember = 'var2'
          //     hostmemberi = ''
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[2]["full"].c_str(), ":var2,");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), "");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "");

          // var #3
          //     full = ':var3'
          //     fulli = ''
          //     hostvar = 'var3'
          //     hostvari = ''
          //     hostrecord = 'var3'
          //     hostrecordi = ''
          //     hostmember = 'var3'
          //     hostmemberi = ''
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[3]["full"].c_str(), ":var3");
          EXPECT_STREQ(hv[3]["hostvar"].c_str(), "var3");
          EXPECT_STREQ(hv[3]["hostrecord"].c_str(), "var3");
          EXPECT_STREQ(hv[3]["hostmember"].c_str(), "var3");
          EXPECT_STREQ(hv[3]["deref"].c_str(), "");

          EXPECT_STREQ(hv[3]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[3]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[3]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[3]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[3]["derefi"].c_str(), "");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsLimit1)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":var1");

          // Total vars = 1
          EXPECT_EQ(hv.size(), 1);
          
          // var #1
          //     full = ':var1'
          //     fulli = ''
          //     hostvar = 'var1'
          //     hostvari = ''
          //     hostrecord = 'var1'
          //     hostrecordi = ''
          //     hostmember = 'var1'
          //     hostmemberi = ''
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[1]["full"].c_str(), ":var1");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "");
          
          EXPECT_STREQ(hv[1]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsLimit0)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(" ");

          // Total vars = 0
          EXPECT_EQ(hv.size(), 0);          
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsPointers)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":var1->member1, :var2->member2");

          // Total vars = 2
          EXPECT_EQ(hv.size(), 2);
          
          // var #1
          //     full = ':var1->member1,'
          //     fulli = ''
          //     hostvar = 'var1->member1'
          //     hostvari = ''
          //     hostrecord = 'var1'
          //     hostrecordi = ''
          //     hostmember = 'member1'
          //     hostmemberi = ''
          //     deref = '->'
          //     derefi = ''
          EXPECT_STREQ(hv[1]["full"].c_str(), ":var1->member1,");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "var1->member1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "member1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "->");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "");

          // var #2
          //     full = ':var2->member2'
          //     fulli = ''
          //     hostvar = 'var2->member2'
          //     hostvari = ''
          //     hostrecord = 'var2'
          //     hostrecordi = ''
          //     hostmember = 'member2'
          //     hostmemberi = ''
          //     deref = '->'
          //     derefi = ''
          EXPECT_STREQ(hv[2]["full"].c_str(), ":var2->member2");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "var2->member2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "member2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), "->");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsStruct)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":var1.member1, :var2.member2");

          // Total vars = 2
          EXPECT_EQ(hv.size(), 2);
          
          // var #1
          //     full = ':var1.member1,'
          //     fulli = ''
          //     hostvar = 'var1.member1'
          //     hostvari = ''
          //     hostrecord = 'var1'
          //     hostrecordi = ''
          //     hostmember = 'member1'
          //     hostmemberi = ''
          //     deref = '.'
          //     derefi = ''
          EXPECT_STREQ(hv[1]["full"].c_str(), ":var1.member1,");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "var1.member1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "member1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), ".");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "");

          // var #2
          //     full = ':var2.member2'
          //     fulli = ''
          //     hostvar = 'var2.member2'
          //     hostvari = ''
          //     hostrecord = 'var2'
          //     hostrecordi = ''
          //     hostmember = 'member2'
          //     hostmemberi = ''
          //     deref = '.'
          //     derefi = ''
          EXPECT_STREQ(hv[2]["full"].c_str(), ":var2.member2");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "var2.member2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "member2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), ".");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsBasicWithIndicators)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":var1:Ivar1, :var2:Ivar2");

          // Total vars = 2
          EXPECT_EQ(hv.size(), 2);
          
          // var #1
          //     full = ':var1'
          //     fulli = ':Ivar1,'
          //     hostvar = 'var1'
          //     hostvari = 'Ivar1'
          //     hostrecord = 'var1'
          //     hostrecordi = 'Ivar1'
          //     hostmember = 'var1'
          //     hostmemberi = 'Ivar1'
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[1]["full"].c_str(), ":var1");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), ":Ivar1,");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "Ivar1");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "Ivar1");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "Ivar1");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "");

          // var #2
          //     full = ':var2'
          //     fulli = ':Ivar2'
          //     hostvar = 'var2'
          //     hostvari = 'Ivar2'
          //     hostrecord = 'var2'
          //     hostrecordi = 'Ivar2'
          //     hostmember = 'var2'
          //     hostmemberi = 'Ivar2'
          //     deref = ''
          //     derefi = ''
          EXPECT_STREQ(hv[2]["full"].c_str(), ":var2");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), "");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), ":Ivar2");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "Ivar2");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "Ivar2");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "Ivar2");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsPointerWithIndicators)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":p->var1:Ip->Ivar1, :p->var2:Ip->Ivar2");

          // Total vars = 2
          EXPECT_EQ(hv.size(), 2);
          
          // var #1
          //     full = ':p->var1'
          //     fulli = ':Ip->Ivar1,'
          //     hostvar = 'p->var1'
          //     hostvari = 'Ip->Ivar1'
          //     hostrecord = 'p'
          //     hostrecordi = 'Ip'
          //     hostmember = 'var1'
          //     hostmemberi = 'Ivar1'
          //     deref = '->'
          //     derefi = '->'
          EXPECT_STREQ(hv[1]["full"].c_str(), ":p->var1");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "p->var1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "p");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), "->");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), ":Ip->Ivar1,");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "Ip->Ivar1");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "Ip");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "Ivar1");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), "->");

          // var #2
          //     full = ':p->var2'
          //     fulli = ':Ip->Ivar2'
          //     hostvar = 'p->var2'
          //     hostvari = 'Ip->Ivar2'
          //     hostrecord = 'p'
          //     hostrecordi = 'Ip'
          //     hostmember = 'var2'
          //     hostmemberi = 'Ivar2'
          //     deref = '->'
          //     derefi = '->'
          EXPECT_STREQ(hv[2]["full"].c_str(), ":p->var2");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "p->var2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "p");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), "->");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), ":Ip->Ivar2");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "Ip->Ivar2");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "Ip");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "Ivar2");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), "->");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsStructWithIndicators)
        {
          map_host_vars hv;
          hv = clang::tidy::pagesjaunes::decodeHostVars(":s.var1:Is.Ivar1, :s.var2:Is.Ivar2");

          // Total vars = 2
          EXPECT_EQ(hv.size(), 2);
          
          // var #1
          //     full = ':s.var1'
          //     fulli = ':Is.Ivar1,'
          //     hostvar = 's.var1'
          //     hostvari = 'Is.Ivar1'
          //     hostrecord = 's'
          //     hostrecordi = 'Is'
          //     hostmember = 'var1'
          //     hostmemberi = 'Ivar1'
          //     deref = '.'
          //     derefi = '.'
          EXPECT_STREQ(hv[1]["full"].c_str(), ":s.var1");
          EXPECT_STREQ(hv[1]["hostvar"].c_str(), "s.var1");
          EXPECT_STREQ(hv[1]["hostrecord"].c_str(), "s");
          EXPECT_STREQ(hv[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv[1]["deref"].c_str(), ".");

          EXPECT_STREQ(hv[1]["fulli"].c_str(), ":Is.Ivar1,");
          EXPECT_STREQ(hv[1]["hostvari"].c_str(), "Is.Ivar1");
          EXPECT_STREQ(hv[1]["hostrecordi"].c_str(), "Is");
          EXPECT_STREQ(hv[1]["hostmemberi"].c_str(), "Ivar1");
          EXPECT_STREQ(hv[1]["derefi"].c_str(), ".");

          // var #2
          //     full = ':s.var2'
          //     fulli = ':Is.Ivar2'
          //     hostvar = 's.var2'
          //     hostvari = 'Is.Ivar2'
          //     hostrecord = 's'
          //     hostrecordi = 'Is'
          //     hostmember = 'var2'
          //     hostmemberi = 'Ivar2'
          //     deref = '.'
          //     derefi = '.'
          EXPECT_STREQ(hv[2]["full"].c_str(), ":s.var2");
          EXPECT_STREQ(hv[2]["hostvar"].c_str(), "s.var2");
          EXPECT_STREQ(hv[2]["hostrecord"].c_str(), "s");
          EXPECT_STREQ(hv[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv[2]["deref"].c_str(), ".");

          EXPECT_STREQ(hv[2]["fulli"].c_str(), ":Is.Ivar2");
          EXPECT_STREQ(hv[2]["hostvari"].c_str(), "Is.Ivar2");
          EXPECT_STREQ(hv[2]["hostrecordi"].c_str(), "Is");
          EXPECT_STREQ(hv[2]["hostmemberi"].c_str(), "Ivar2");
          EXPECT_STREQ(hv[2]["derefi"].c_str(), ".");
        }

        TEST_F(DecodeHostVarsTest, DecodeHostVarsMixedWithIndicators)
        {
          map_host_vars hv1;
          hv1 = clang::tidy::pagesjaunes::decodeHostVars(":p->var1:Is.Ivar1, :p->var2:Is.Ivar2");

          // Total vars = 2
          EXPECT_EQ(hv1.size(), 2);
          
          // var #1
          //     full = ':p->var1'
          //     fulli = ':Is.Ivar1,'
          //     hostvar = 'p->var1'
          //     hostvari = 'Is.Ivar1'
          //     hostrecord = 'p'
          //     hostrecordi = 'Is'
          //     hostmember = 'var1'
          //     hostmemberi = 'Ivar1'
          //     deref = '->'
          //     derefi = '.'
          EXPECT_STREQ(hv1[1]["full"].c_str(), ":p->var1");
          EXPECT_STREQ(hv1[1]["hostvar"].c_str(), "p->var1");
          EXPECT_STREQ(hv1[1]["hostrecord"].c_str(), "p");
          EXPECT_STREQ(hv1[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv1[1]["deref"].c_str(), "->");

          EXPECT_STREQ(hv1[1]["fulli"].c_str(), ":Is.Ivar1,");
          EXPECT_STREQ(hv1[1]["hostvari"].c_str(), "Is.Ivar1");
          EXPECT_STREQ(hv1[1]["hostrecordi"].c_str(), "Is");
          EXPECT_STREQ(hv1[1]["hostmemberi"].c_str(), "Ivar1");
          EXPECT_STREQ(hv1[1]["derefi"].c_str(), ".");
          
          // var #2
          //     full = ':p->var2'
          //     fulli = ':Is.Ivar2'
          //     hostvar = 'p->var2'
          //     hostvari = 'Is.Ivar2'
          //     hostrecord = 'p'
          //     hostrecordi = 'Is'
          //     hostmember = 'var2'
          //     hostmemberi = 'Ivar2'
          //     deref = '->'
          //     derefi = '.'
          EXPECT_STREQ(hv1[2]["full"].c_str(), ":p->var2");
          EXPECT_STREQ(hv1[2]["hostvar"].c_str(), "p->var2");
          EXPECT_STREQ(hv1[2]["hostrecord"].c_str(), "p");
          EXPECT_STREQ(hv1[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv1[2]["deref"].c_str(), "->");

          EXPECT_STREQ(hv1[2]["fulli"].c_str(), ":Is.Ivar2");
          EXPECT_STREQ(hv1[2]["hostvari"].c_str(), "Is.Ivar2");
          EXPECT_STREQ(hv1[2]["hostrecordi"].c_str(), "Is");
          EXPECT_STREQ(hv1[2]["hostmemberi"].c_str(), "Ivar2");
          EXPECT_STREQ(hv1[2]["derefi"].c_str(), ".");

          map_host_vars hv2;
          hv2 = clang::tidy::pagesjaunes::decodeHostVars(":p->var1:Is.Ivar1, :p->var2:Is.Ivar2, :s.var3:Ip->Ivar3, :s.var4:Ip->Ivar4");

          // Total vars = 4
          EXPECT_EQ(hv2.size(), 4);
          
          // var #1
          //     full = ':p->var1'
          //     fulli = ':Is.Ivar1,'
          //     hostvar = 'p->var1'
          //     hostvari = 'Is.Ivar1'
          //     hostrecord = 'p'
          //     hostrecordi = 'Is'
          //     hostmember = 'var1'
          //     hostmemberi = 'Ivar1'
          //     deref = '->'
          //     derefi = '.'
          EXPECT_STREQ(hv2[1]["full"].c_str(), ":p->var1");
          EXPECT_STREQ(hv2[1]["hostvar"].c_str(), "p->var1");
          EXPECT_STREQ(hv2[1]["hostrecord"].c_str(), "p");
          EXPECT_STREQ(hv2[1]["hostmember"].c_str(), "var1");
          EXPECT_STREQ(hv2[1]["deref"].c_str(), "->");

          EXPECT_STREQ(hv2[1]["fulli"].c_str(), ":Is.Ivar1,");
          EXPECT_STREQ(hv2[1]["hostvari"].c_str(), "Is.Ivar1");
          EXPECT_STREQ(hv2[1]["hostrecordi"].c_str(), "Is");
          EXPECT_STREQ(hv2[1]["hostmemberi"].c_str(), "Ivar1");
          EXPECT_STREQ(hv2[1]["derefi"].c_str(), ".");

          // var #2
          //     full = ':p->var2'
          //     fulli = ':Is.Ivar2,'
          //     hostvar = 'p->var2'
          //     hostvari = 'Is.Ivar2'
          //     hostrecord = 'p'
          //     hostrecordi = 'Is'
          //     hostmember = 'var2'
          //     hostmemberi = 'Ivar2'
          //     deref = '->'
          //     derefi = '.'
          EXPECT_STREQ(hv2[2]["full"].c_str(), ":p->var2");
          EXPECT_STREQ(hv2[2]["hostvar"].c_str(), "p->var2");
          EXPECT_STREQ(hv2[2]["hostrecord"].c_str(), "p");
          EXPECT_STREQ(hv2[2]["hostmember"].c_str(), "var2");
          EXPECT_STREQ(hv2[2]["deref"].c_str(), "->");

          EXPECT_STREQ(hv2[2]["fulli"].c_str(), ":Is.Ivar2,");
          EXPECT_STREQ(hv2[2]["hostvari"].c_str(), "Is.Ivar2");
          EXPECT_STREQ(hv2[2]["hostrecordi"].c_str(), "Is");
          EXPECT_STREQ(hv2[2]["hostmemberi"].c_str(), "Ivar2");
          EXPECT_STREQ(hv2[2]["derefi"].c_str(), ".");

          // var #3
          //     full = ':s.var3'
          //     fulli = ':Ip->Ivar3,'
          //     hostvar = 's.var3'
          //     hostvari = 'Ip->Ivar3'
          //     hostrecord = 's'
          //     hostrecordi = 'Ip'
          //     hostmember = 'var3'
          //     hostmemberi = 'Ivar3'
          //     deref = '.'
          //     derefi = '->'
          EXPECT_STREQ(hv2[3]["full"].c_str(), ":s.var3");
          EXPECT_STREQ(hv2[3]["hostvar"].c_str(), "s.var3");
          EXPECT_STREQ(hv2[3]["hostrecord"].c_str(), "s");
          EXPECT_STREQ(hv2[3]["hostmember"].c_str(), "var3");
          EXPECT_STREQ(hv2[3]["deref"].c_str(), ".");

          EXPECT_STREQ(hv2[3]["fulli"].c_str(), ":Ip->Ivar3,");
          EXPECT_STREQ(hv2[3]["hostvari"].c_str(), "Ip->Ivar3");
          EXPECT_STREQ(hv2[3]["hostrecordi"].c_str(), "Ip");
          EXPECT_STREQ(hv2[3]["hostmemberi"].c_str(), "Ivar3");
          EXPECT_STREQ(hv2[3]["derefi"].c_str(), "->");

          // var #4
          //     full = ':s.var4'
          //     fulli = ':Ip->Ivar4'
          //     hostvar = 's.var4'
          //     hostvari = 'Ip->Ivar4'
          //     hostrecord = 's'
          //     hostrecordi = 'Ip'
          //     hostmember = 'var4'
          //     hostmemberi = 'Ivar4'
          //     deref = '.'
          //     derefi = '->'
          EXPECT_STREQ(hv2[4]["full"].c_str(), ":s.var4");
          EXPECT_STREQ(hv2[4]["hostvar"].c_str(), "s.var4");
          EXPECT_STREQ(hv2[4]["hostrecord"].c_str(), "s");
          EXPECT_STREQ(hv2[4]["hostmember"].c_str(), "var4");
          EXPECT_STREQ(hv2[4]["deref"].c_str(), ".");

          EXPECT_STREQ(hv2[4]["fulli"].c_str(), ":Ip->Ivar4");
          EXPECT_STREQ(hv2[4]["hostvari"].c_str(), "Ip->Ivar4");
          EXPECT_STREQ(hv2[4]["hostrecordi"].c_str(), "Ip");
          EXPECT_STREQ(hv2[4]["hostmemberi"].c_str(), "Ivar4");
          EXPECT_STREQ(hv2[4]["derefi"].c_str(), "->");
        }


        TEST_F(DecodeHostVarsTest, DecodeHostVarsInvalid)
        {
          map_host_vars hv1;
          hv1 = clang::tidy::pagesjaunes::decodeHostVars(":2a.3bs");
          // Total vars = 0
          EXPECT_EQ(hv1.size(), 0);
          
          map_host_vars hv2;
          hv2 = clang::tidy::pagesjaunes::decodeHostVars(":2asdjkhfkku-0dfsdkhkgzejkfg->3bs.7gdf,:5djkjdsgui->2odjfkhfzh");
          // Total vars = 0
          EXPECT_EQ(hv2.size(), 0);
          
          map_host_vars hv3;
          hv3 = clang::tidy::pagesjaunes::decodeHostVars(":->:.,:.:->");
          // Total vars = 0
          EXPECT_EQ(hv3.size(), 0);

          map_host_vars hv4;
          hv4 = clang::tidy::pagesjaunes::decodeHostVars(":1");
          // Total vars = 0
          EXPECT_EQ(hv4.size(), 0);
        }
        
        TEST_F(DecodeHostVarsTest, DecodeHostVarsWeird)
        {
          map_host_vars hv1;
          hv1 = clang::tidy::pagesjaunes::decodeHostVars(":ptr ->	member :  \n ptr2 \n -> \n	member2");

          // Total vars = 1
          EXPECT_EQ(hv1.size(), 1);
          
          // var #1
          //     full = ':ptr ->	member'
          //     fulli = ':  \n ptr2 \n -> \n	member2'
          //     hostvar = 'ptr ->	member'
          //     hostvari = 'ptr2 \n -> \n	member2'
          //     hostrecord = 'ptr'
          //     hostrecordi = 'ptr2'
          //     hostmember = 'member'
          //     hostmemberi = 'member2'
          //     deref = '->'
          //     derefi = '->'
          EXPECT_STREQ(hv1[1]["full"].c_str(), ":ptr ->\tmember");
          EXPECT_STREQ(hv1[1]["hostvar"].c_str(), "ptr ->\tmember");
          EXPECT_STREQ(hv1[1]["hostrecord"].c_str(), "ptr");
          EXPECT_STREQ(hv1[1]["hostmember"].c_str(), "member");
          EXPECT_STREQ(hv1[1]["deref"].c_str(), "->");

          EXPECT_STREQ(hv1[1]["fulli"].c_str(), ":  \n ptr2 \n -> \n	member2");
          EXPECT_STREQ(hv1[1]["hostvari"].c_str(), "ptr2 \n -> \n\tmember2");
          EXPECT_STREQ(hv1[1]["hostrecordi"].c_str(), "ptr2");
          EXPECT_STREQ(hv1[1]["hostmemberi"].c_str(), "member2");
          EXPECT_STREQ(hv1[1]["derefi"].c_str(), "->");
        }
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
