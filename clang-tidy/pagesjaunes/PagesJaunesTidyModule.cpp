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
#include "ExecSQLOpenToFunctionCall.h"
#include "ExecSQLPrepareToFunctionCall.h"
#include "ExecSQLPrepareFmtdToFunctionCall.h"
#include "DeIncludePreProC.h"

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
	  CheckFactories.registerCheck<ExecSQLOpenToFunctionCall> ("pagesjaunes-exec-sql-open-to-function-call");
	  CheckFactories.registerCheck<ExecSQLPrepareToFunctionCall> ("pagesjaunes-exec-sql-prepare-to-function-call");
	  CheckFactories.registerCheck<ExecSQLPrepareFmtdToFunctionCall> ("pagesjaunes-exec-sql-prepare-fmtd-to-function-call");
	  CheckFactories.registerCheck<DeIncludePreProC> ("pagesjaunes-de-include-preproc");
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

	  /*
	   * Options are available in order to select processed headers
	   * and indicated their location.
	   */
	  Opts["pagesjaunes-de-include-preproc.Comment-regex"] = "^.*EXEC SQL[ \t]+include[ \t]+\"([_A-Za-z.]+)\".*$";
	  Opts["pagesjaunes-de-include-preproc.Headers-to-include-in"] = "";
	  Opts["pagesjaunes-de-include-preproc.Headers-to-exclude-from"] = "GYBstruct_Pro_C.h,GYBgestion_pro_c.h";
	  Opts["pagesjaunes-de-include-preproc.Headers-directories"] = "./Include/";

	  /*
	   * 
	   */
	  Opts["pagesjaunes-exec-sql-to-function-call.Generate-requests-headers"] = "1";
	  Opts["pagesjaunes-exec-sql-to-function-call.Generate-requests-sources"] = "1";
	  Opts["pagesjaunes-exec-sql-to-function-call.Generation-directory"] = "./REQSQL/src";
	  Opts["pagesjaunes-exec-sql-to-function-call.Generation-header-template"] = "./pagesjaunes.h.tmpl";
	  Opts["pagesjaunes-exec-sql-to-function-call.Generation-source-template"] = "./pagesjaunes.pc.tmpl";
	  Opts["pagesjaunes-exec-sql-to-function-call.Generation-request-groups"] = "request_groups.json";

	  /*
	   * 
	   */
	  Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generate-requests-headers"] = "1";
	  Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generate-requests-sources"] = "1";
	  Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-directory"] = "./REQSQL/src";
	  Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-header-template"] = "./pagesjaunes_prepare.h.tmpl";
	  Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-source-template"] = "./pagesjaunes_prepare.pc.tmpl";
	  Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-request-groups"] = "request_groups.json";

	  /*
	   * 
	   */
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generate-requests-headers"] = "1";
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generate-requests-sources"] = "1";
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-directory"] = "./REQSQL/src";
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-header-template"] = "./pagesjaunes_prepare_fmt.h.tmpl";
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-source-template"] = "./pagesjaunes_prepare_fmt.pc.tmpl";
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-request-groups"] = "request_groups.json";
	  Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-simplify-function-args"] = "0";

	  /*
	   * 
	   */
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generate-requests-headers"] = "1";
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generate-requests-sources"] = "1";
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-directory"] = "./REQSQL/src";
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-header-template"] = "./pagesjaunes_open.h.tmpl";
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-source-template"] = "./pagesjaunes_open.pc.tmpl";
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-request-groups"] = "request_groups.json";
	  Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-simplify-function-args"] = "0";

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
