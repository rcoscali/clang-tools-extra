//===--- free_regex_test.h - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __FREE_REGEX_TEST_H__
#define __FREE_REGEX_TEST_H__

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

        class FreeRegexTest : public ::testing::Test
        {
        public:
          FreeRegexTest();
          virtual ~FreeRegexTest();
          
          virtual void SetUp(void);
          virtual void TearDown(void);
          
          // It's important that PrintTo() is defined in the SAME
          // namespace that defines Bar.  C++'s look-up rules rely on that.
          void PrintTo(const FreeRegexTest&, ::std::ostream*);

          llvm::Regex& get_free_re()
            {
              return free_re_m;
            }
          
        private:
          llvm::Regex free_re_m;
        };
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang

#endif /*! __FREE_REGEX_TEST_H__ */
