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
	/**
	 * Register all checks of PageJaunes module
	 */
	void
	addCheckFactories(ClangTidyCheckFactories &CheckFactories) override 
	{
	  CheckFactories.registerCheck<CCharToCXXString> ("pagesjaunes-C-char-to-CXX-string");
	  CheckFactories.registerCheck<ExecSQLToFunctionCall> ("pagesjaunes-exec-sql-to-function-call");
	  CheckFactories.registerCheck<ExecSQLToFunctionCall> ("pagesjaunes-de-include-preproc");
	}

	/**
	 * Register checks options
	 */
	ClangTidyOptions
	getModuleOptions() override
	{
	  ClangTidyOptions Options;
	  auto &Opts = Options.CheckOptions;

	  /**
	   * Options are available in order to enable(1)/disable(0) processing
	   * of each possible string manipulation functions.
	   */
	  Opts["pagesjaunes-C-char-to-CXX-string.Handle-strcpy"] = "1";
	  Opts["pagesjaunes-C-char-to-CXX-string.Handle-strcmp"] = "1";
	  Opts["pagesjaunes-C-char-to-CXX-string.Handle-strlen"] = "1";

	  return Options;
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
