//===--- PagesJaunesTidyModule.cpp - clang-tidy ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "CCharToCXXString.h"
#include "ExecSQLToFunctionCall.h"

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes 
    {

      class PagesJaunesModule : public ClangTidyModule 
      {
      public:
	void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override 
	{
	  CheckFactories.registerCheck<CCharToCXXString> ("pagesjaunes-C-char-to-CXX-string");
	  CheckFactories.registerCheck<ExecSQLToFunctionCall> ("pagesjaunes-exec-sql-to-function-call");
	}
      };

      // Register the PagesJaunesModule using this statically initialized variable.
      static ClangTidyModuleRegistry::Add<PagesJaunesModule> X("pagesjaunes-module", 
							       "Adds PagesJaunes refactoring rules related checks.");
      
    } // namespace pagesjaunes

    // This anchor is used to force the linker to link in the generated object file
    // and thus register the PagesJaunesModule.
    volatile int PagesJaunesModuleAnchorSource = 0;

  } // namespace tidy
} // namespace clang
