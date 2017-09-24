//===--- DeIncludePreProC.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_UNINCLUDE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_UNINCLUDE_H

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

      // Checks that argument name match parameter name rules.
      class DeIncludePreProC : public ClangTidyCheck 
      {
      public:
	// Constructor
	DeIncludePreProC(StringRef, ClangTidyContext *);

	// Register matrchers
	void registerMatchers(ast_matchers::MatchFinder *) override;
	// Check a matched node
	void check(const ast_matchers::MatchFinder::MatchResult &) override;

      private:

	enum DeIncludePreProCErrorKind
	  {
	    //
	    DE_INCLUDE_PRE_PROC_ERROR_NO_ERROR = 0,
	    //
	    DE_INCLUDE_PRE_PROC_ERROR_ACCESS_CHAR_DATA,
	    //
	    DE_INCLUDE_PRE_PROC_ERROR_CANT_FIND_COMMENT_START,
	    //
	    DE_INCLUDE_PRE_PROC_ERROR_COMMENT_DONT_MATCH,
	  };

	// Emit diagnostic and eventually fix it
        void emitDiagAndFix(const SourceLocation&,
			    const SourceLocation&,
			    const std::string&);

	// Emit error
	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum DeIncludePreProCErrorKind);

	// AST Context instance
	ClangTidyContext *TidyContext;
	// Diag ids
	const unsigned unexpected_diag_id;
	// Diag ids
	const unsigned no_error_diag_id;
	// Diag ids
	const unsigned access_char_data_diag_id;
	// Diag ids
	const unsigned cant_find_comment_diag_id;
	// Diag ids
	const unsigned comment_dont_match_diag_id;

      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_UNINCLUDE_H
