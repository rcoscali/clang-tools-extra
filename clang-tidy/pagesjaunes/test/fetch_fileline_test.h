//===--- fetch_fileline_test.h - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __FETCH_FILELINE_TEST_H__
#define __FETCH_FILELINE_TEST_H__

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

        class FetchFilelineTest : public ::testing::Test
        {
        public:
          FetchFilelineTest();
          virtual ~FetchFilelineTest();
          
          virtual void SetUp(void);
          virtual void TearDown(void);
          
          // It's important that PrintTo() is defined in the SAME
          // namespace that defines Bar.  C++'s look-up rules rely on that.
          void PrintTo(const FetchFilelineTest&, ::std::ostream*);

          llvm::Regex& get_fetch_re()
            {
              return fetch_re_m;
            }
          
        private:
          llvm::Regex fetch_re_m;
        };
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang

#endif /*! __FETCH_FILELINE_TEST_H__ */
