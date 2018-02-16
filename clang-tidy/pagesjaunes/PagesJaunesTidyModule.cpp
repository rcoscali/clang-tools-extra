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
#include "DeIncludePreProC.h"
#include "ExecSQLAllocateToFunctionCall.h"
#include "ExecSQLForToFunctionCall.h"
#include "ExecSQLFreeToFunctionCall.h"
#include "ExecSQLLOBCreateToFunctionCall.h"
#include "ExecSQLLOBOpenToFunctionCall.h"
#include "ExecSQLLOBReadToFunctionCall.h"
#include "ExecSQLLOBCloseToFunctionCall.h"
#include "ExecSQLLOBFreeToFunctionCall.h"
#include "ExecSQLFetchToFunctionCall.h"
#include "ExecSQLOpenToFunctionCall.h"
#include "ExecSQLCloseToFunctionCall.h"
#include "ExecSQLPrepareToFunctionCall.h"
#include "ExecSQLPrepareFmtdToFunctionCall.h"


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
          CheckFactories.registerCheck<ExecSQLAllocateToFunctionCall> ("pagesjaunes-exec-sql-allocate-to-function-call");
          CheckFactories.registerCheck<ExecSQLFetchToFunctionCall> ("pagesjaunes-exec-sql-fetch-to-function-call");
          CheckFactories.registerCheck<ExecSQLForToFunctionCall> ("pagesjaunes-exec-sql-for-to-function-call");
          CheckFactories.registerCheck<ExecSQLFreeToFunctionCall> ("pagesjaunes-exec-sql-free-to-function-call");
          CheckFactories.registerCheck<ExecSQLLOBCloseToFunctionCall> ("pagesjaunes-exec-sql-lob-close-to-function-call");
          CheckFactories.registerCheck<ExecSQLLOBCreateToFunctionCall> ("pagesjaunes-exec-sql-lob-create-temporary-to-function-call");
          CheckFactories.registerCheck<ExecSQLLOBFreeToFunctionCall> ("pagesjaunes-exec-sql-lob-free-temporary-to-function-call");
          CheckFactories.registerCheck<ExecSQLLOBOpenToFunctionCall> ("pagesjaunes-exec-sql-lob-open-to-function-call");
          CheckFactories.registerCheck<ExecSQLLOBReadToFunctionCall> ("pagesjaunes-exec-sql-lob-read-to-function-call");
          CheckFactories.registerCheck<ExecSQLOpenToFunctionCall> ("pagesjaunes-exec-sql-open-to-function-call");
          CheckFactories.registerCheck<ExecSQLCloseToFunctionCall> ("pagesjaunes-exec-sql-close-to-function-call");
          CheckFactories.registerCheck<ExecSQLPrepareFmtdToFunctionCall> ("pagesjaunes-exec-sql-prepare-fmtd-to-function-call");
          CheckFactories.registerCheck<ExecSQLPrepareToFunctionCall> ("pagesjaunes-exec-sql-prepare-to-function-call");
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
          Opts["pagesjaunes-de-include-preproc.Comment-regex"] = "EXEC[[:space:]]+SQL[[:space:]]+([Ii][Nn][Cc][Ll][Uu][Dd][Ee])[[:space:]]+\"([[:alnum:]]+)\"";
          Opts["pagesjaunes-de-include-preproc.Headers-to-include-in"] = "";
          Opts["pagesjaunes-de-include-preproc.Headers-to-exclude-from"] = "GYBstruct_Pro_C.h,GYBgestion_pro_c.h";
          Opts["pagesjaunes-de-include-preproc.Headers-directories"] = "./Include/";

          /*
           * Allocate requests
           */
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-header-template"] = "./pagesjaunes_allocate.h.tmpl";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-source-template"] = "./pagesjaunes_allocate.pc.tmpl";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-allocate-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * For requests
           */
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-header-template"] = "./pagesjaunes_for.h.tmpl";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-source-template"] = "./pagesjaunes_for.pc.tmpl";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-for-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * Free requests
           */
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-header-template"] = "./pagesjaunes_free.h.tmpl";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-source-template"] = "./pagesjaunes_free.pc.tmpl";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-free-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * LOB CLOSE requests
           */
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-header-template"] = "./pagesjaunes_lob_close.h.tmpl";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-source-template"] = "./pagesjaunes_lob_close.pc.tmpl";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-lob-close-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * LOB CREATE TEMPORARY requests
           */
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-directory"] = "./";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generate-requests-allow-overwrite"] = "1";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-header-template"] = "./pagesjaunes_lob_create_temporary.h.tmpl";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-source-template"] = "./pagesjaunes_lob_create_temporary.pc.tmpl";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-lob-create-temporary-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * LOB FREE TEMPORARY requests
           */
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-header-template"] = "./pagesjaunes_lob_free_temporary.h.tmpl";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-source-template"] = "./pagesjaunes_lob_free_temporary.pc.tmpl";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-lob-free-temporary-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * LOB OPEN requests
           */
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-header-template"] = "./pagesjaunes_lob_open.h.tmpl";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-source-template"] = "./pagesjaunes_lob_open.pc.tmpl";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-lob-open-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * LOB READ requests
           */
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-header-template"] = "./pagesjaunes_lob_read.h.tmpl";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-source-template"] = "./pagesjaunes_lob_read.pc.tmpl";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-lob-read-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * Fetch requests
           */
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-header-template"] = "./pagesjaunes_fetch.h.tmpl";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-source-template"] = "./pagesjaunes_fetch.pc.tmpl";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-simplify-function-args"] = "0";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-fetch-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * Open requests
           */
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generate-requests-allow-overwrite"] = "1";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-header-template"] = "./pagesjaunes_open.h.tmpl";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-source-template"] = "./pagesjaunes_open.pc.tmpl";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-simplify-function-args"] = "0";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-open-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * Close requests
           */
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generate-requests-allow-overwrite"] = "1";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-header-template"] = "./pagesjaunes_close.h.tmpl";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-source-template"] = "./pagesjaunes_close.pc.tmpl";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-simplify-function-args"] = "0";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-close-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * Prepare requests (sprintf from define with no params)
           */
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-header-template"] = "./pagesjaunes_prepare.h.tmpl";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-source-template"] = "./pagesjaunes_prepare.pc.tmpl";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-prepare-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

          /*
           * Sprintf formatted prepare requests (sprintf from define with params)
           */
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generate-requests-headers"] = "1";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generate-requests-sources"] = "1";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-directory"] = ".";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-header-template"] = "./pagesjaunes_prepare_fmt.h.tmpl";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-source-template"] = "./pagesjaunes_prepare_fmt.pc.tmpl";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-request-groups"] = "request_groups.json";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-simplify-function-args"] = "0";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-do-report-modification-in-PC"] = "1";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-report-modification-in-dir"] = "./";
          Opts["pagesjaunes-exec-sql-prepare-fmtd-to-function-call.Generation-keep-commented-out-exec-sql-in-PC"] = "0";

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
