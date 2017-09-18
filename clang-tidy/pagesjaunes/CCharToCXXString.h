//===--- CCharToCXXString.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARTOCXXSTRING_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARTOCXXSTRING_H

#include "../ClangTidy.h"
#include "llvm/Support/Regex.h"

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      /// \brief Checks that argument name match parameter name rules.
      class CCharToCXXString : public ClangTidyCheck 
      {
      public:
	CCharToCXXString(StringRef Name, ClangTidyContext *Context);

	void registerMatchers(ast_matchers::MatchFinder *Finder) override;
	void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

      private:

	enum CCharToCXXStringErrorKind {
	  CCHAR_2_CXXSTRING_ERROR_NO_ERROR = 0,
	};
		
        void emitDiagAndFix(const SourceLocation& call_start, const SourceLocation& call_end,
			    const SourceLocation& def_start, const SourceLocation& def_end);
	
	void emitError(DiagnosticsEngine &,
		       const SourceLocation& err_loc,
		       enum CCharToCXXStringErrorKind);

	ClangTidyContext *TidyContext;
	const unsigned unexpected_diag_id;
	const unsigned no_error_diag_id;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARTOCXXSTRING_H
