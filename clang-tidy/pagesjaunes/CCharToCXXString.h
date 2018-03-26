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
      ///   * `Include-Comment-Regex`: The regular expression to use for parsing comment
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

	// Store check options
	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	// Register the AST matchers used for finding nodes
	void registerMatchers(ast_matchers::MatchFinder *Finder) override;
	// Callback calkled with each found node
	void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

      private:

	void checkStrcmp(SourceManager &,
			 DiagnosticsEngine &,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	void checkStrcpy(SourceManager &,
			 DiagnosticsEngine &,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	void checkStrcat(SourceManager &,
			 DiagnosticsEngine &,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	void checkStrlen(SourceManager &,
			 DiagnosticsEngine &,
			 CallExpr *,
			 const MatchFinder::MatchResult &);
	
	enum CCharToCXXStringCallKind
	  {
	    CCHAR_2_CXXSTRING_CALL_STRCMP = 0,
	    CCHAR_2_CXXSTRING_CALL_STRNCMP,
	    CCHAR_2_CXXSTRING_CALL_STRCPY,
	    CCHAR_2_CXXSTRING_CALL_STRNCPY,
	    CCHAR_2_CXXSTRING_CALL_STRCAT,
	    CCHAR_2_CXXSTRING_CALL_STRNCAT,
	    CCHAR_2_CXXSTRING_CALL_STRLEN,
	  };

        void emitDiagAndFix(DiagnosticsEngine &,
			    const SourceLocation&, const SourceLocation&,
			    enum CCharToCXXStringCallKind,
			    std::string&, std::string&, std::string&, 
			    const SourceLocation&, const SourceLocation&,
			    std::string&, std::string&, std::string&, std::string&);
	
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
		
	void emitError(DiagnosticsEngine &,
		       const SourceLocation& err_loc,
		       enum CCharToCXXStringErrorKind,
		       std::string *msg = nullptr);

	// Context instance
	ClangTidyContext *TidyContext;

	// Check options
	const unsigned handle_strcmp;
	const unsigned handle_strcpy;
	const unsigned handle_strcat;
	const unsigned handle_strlen;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARTOCXXSTRING_H
