//===--- decode_host_vars.h - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __DECODE_HOST_VARS_H__
#define __DECODE_HOST_VARS_H__

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

        class DecodeHostVarsTest : public ::testing::Test
        {
        public:
          DecodeHostVarsTest();
          virtual ~DecodeHostVarsTest();
          
          virtual void SetUp(void);
          virtual void TearDown(void);
          
          // It's important that PrintTo() is defined in the SAME
          // namespace that defines Bar.  C++'s look-up rules rely on that.
          void PrintTo(const DecodeHostVarsTest&, ::std::ostream*);
          
        private:
          llvm::Regex fetch_re_m;
        };
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang

#endif /*! __DECODE_HOST_VARS_H__ */
