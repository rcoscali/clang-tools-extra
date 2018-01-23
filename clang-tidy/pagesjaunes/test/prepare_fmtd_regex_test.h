//===--- prepare_fmtd_regex_test.h - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __PREPARE_FMTD_REGEX_TEST_H__
#define __PREPARE_FMTD_REGEX_TEST_H__

#include "gtest/gtest.h"

#include "llvm/Support/Regex.h"
#include "ExecSQLCommon.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        class PrepareFmtdRegexTest : public ::testing::Test
        {
        public:
          PrepareFmtdRegexTest();
          virtual ~PrepareFmtdRegexTest();
          
          virtual void SetUp(void);
          virtual void TearDown(void);
          
          // It's important that PrintTo() is defined in the SAME
          // namespace that defines Bar.  C++'s look-up rules rely on that.
          void PrintTo(const PrepareFmtdRegexTest&, ::std::ostream*);

          llvm::Regex& get_prepare_fmtd_re()
            {
              return prepare_fmtd_re_m;
            }
          
        private:
          llvm::Regex prepare_fmtd_re_m;
        };
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang

#endif /*! __PREPARE_FMTD_REGEX_TEST_H__ */
