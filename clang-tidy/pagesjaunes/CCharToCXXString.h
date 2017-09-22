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
	  CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND,
	  CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND,
	  CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF,
	  CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND,
	  CCHAR_2_CXXSTRING_ERROR_MEMBER2_NOT_FOUND,
	  CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND
	};
		
        void emitDiagAndFix(DiagnosticsEngine &,
			    const SourceLocation&, const SourceLocation&,
			    std::string&, std::string&, std::string&, 
			    const SourceLocation&, const SourceLocation&,
			    std::string&, std::string&, std::string&, std::string&);
	
	void emitError(DiagnosticsEngine &,
		       const SourceLocation& err_loc,
		       enum CCharToCXXStringErrorKind,
		       std::string *msg = nullptr);

	ClangTidyContext *TidyContext;
	const unsigned unexpected_diag_id;
	const unsigned no_error_diag_id;
	const unsigned array_type_not_found_diag_id;
	const unsigned record_decl_not_found_diag_id;
	const unsigned member_has_no_def_diag_id;
	const unsigned member_not_found_diag_id;
	const unsigned member2_not_found_diag_id;
	const unsigned unexpected_ast_node_kind_diag_id;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARTOCXXSTRING_H
