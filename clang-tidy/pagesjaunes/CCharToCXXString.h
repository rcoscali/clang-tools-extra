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

using namespace llvm;
using namespace clang::ast_matchers;
using namespace clang;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      /// @brief Checks that argument name match parameter name rules.
      ///
      /// These options are supported:
      ///   * `Handle-strcpy`: actually process or not strcpy
      ///     default is 1 (process strcpy)
      ///   * `Handle-strcmp`: actually process or not strcpy
      ///     default is 1 (process strcmp)
      ///   * `Handle-strlen`: actually process or not strcpy
      ///     default is 1 (process strlen)
      ///
      class CCharToCXXString : public ClangTidyCheck 
      {
      public:
	CCharToCXXString(StringRef Name, ClangTidyContext *Context);

	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	void registerMatchers(ast_matchers::MatchFinder *Finder) override;
	void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

      private:

	void checkStrcmp(SourceManager &,
			 DiagnosticsEngine &,
			 std::string&,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	void checkStrcpy(SourceManager &,
			 DiagnosticsEngine &,
			 std::string&,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	void checkStrlen(SourceManager &,
			 DiagnosticsEngine &,
			 std::string&,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	enum CCharToCXXStringErrorKind
	  {
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

	// Context instance
	ClangTidyContext *TidyContext;
	// Diag ids
	const unsigned unexpected_diag_id;
	const unsigned no_error_diag_id;
	const unsigned array_type_not_found_diag_id;
	const unsigned record_decl_not_found_diag_id;
	const unsigned member_has_no_def_diag_id;
	const unsigned member_not_found_diag_id;
	const unsigned member2_not_found_diag_id;
	const unsigned unexpected_ast_node_kind_diag_id;
	// Check options
	const unsigned handle_strcmp;
	const unsigned handle_strcpy;
	const unsigned handle_strlen;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARTOCXXSTRING_H
