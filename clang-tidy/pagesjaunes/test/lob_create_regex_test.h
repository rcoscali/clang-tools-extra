//===--- lob_create_regex_test.h - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __LOB_CREATE_REGEX_TEST_H__
#define __LOB_CREATE_REGEX_TEST_H__

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

        class LobCreateRegexTest : public ::testing::Test
        {
        public:
          LobCreateRegexTest();
          virtual ~LobCreateRegexTest();
          
          virtual void SetUp(void);
          virtual void TearDown(void);
          
          // It's important that PrintTo() is defined in the SAME
          // namespace that defines Bar.  C++'s look-up rules rely on that.
          void PrintTo(const LobCreateRegexTest&, ::std::ostream*);

          llvm::Regex& get_lob_create_re()
            {
              return lob_create_re_m;
            }
          
        private:
          llvm::Regex lob_create_re_m;
        };
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang

#endif /*! __LOB_CREATE_REGEX_TEST_H__ */
