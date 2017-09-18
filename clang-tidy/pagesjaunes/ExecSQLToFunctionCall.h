//===--- ExecSQLToFunctionCall.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLTOFUNCTIONCALL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLTOFUNCTIONCALL_H

#include "../ClangTidy.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Regex.h"
#include "clang/Basic/Diagnostic.h"

using namespace clang;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      /// \brief Checks that argument name match parameter name rules.
      class ExecSQLToFunctionCall : public ClangTidyCheck 
      {
      public:
	ExecSQLToFunctionCall(StringRef, ClangTidyContext *);

	void registerMatchers(ast_matchers::MatchFinder *) override;
	void check(const ast_matchers::MatchFinder::MatchResult &) override;

      private:

	enum ExecSQLToFunctionCallErrorKind {
	  EXEC_SQL_2_FUNC_ERROR_NO_ERROR = 0,
	  EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA,
	  EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START,
	  EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH,
	};
	
        void emitDiagAndFix(const SourceLocation&,
			    const SourceLocation&,
			    const std::string&);

	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum ExecSQLToFunctionCallErrorKind);

	ClangTidyContext *TidyContext;
	const unsigned unexpected_diag_id;
	const unsigned no_error_diag_id;
	const unsigned access_char_data_diag_id;
	const unsigned cant_find_comment_diag_id;
	const unsigned comment_dont_match_diag_id;

      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLTOFUNCTIONCALL_H
