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
using namespace clang::ast_matchers;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      // Checks that argument name match parameter name rules.
      class ExecSQLToFunctionCall : public ClangTidyCheck 
      {
      public:
	// Constructor
	ExecSQLToFunctionCall(StringRef, ClangTidyContext *);

	// Store check Options
	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	// Register matrchers
	void registerMatchers(ast_matchers::MatchFinder *) override;
	// Check a matched node
	void check(const ast_matchers::MatchFinder::MatchResult &) override;

      protected:
	/*
	 * sprintf finder collector in case of a prepare request
	 * Literal must be directly copied in the :reqName var
	 */
	struct StringLiteralRecord
	{
	  const CallExpr *callExpr;
	  unsigned call_linenum;
	  const StringLiteral *literal;
	  unsigned linenum;
	};

	// Collector for possible sprintf calls
	std::vector<struct StringLiteralRecord *> m_req_copy_collector;
	
      private:

	/**
	 * FindCopyRequestCall
	 *
	 */
	class CopyRequestMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit CopyRequestMatcher(ExecSQLToFunctionCall *parent)
	    : m_parent(parent)
	  {}

	  /// The run method adding all calls in the collection vector
	  virtual void
	  run(const MatchFinder::MatchResult &result)
	  {
	    struct StringLiteralRecord *record = new(struct StringLiteralRecord);
	    record->callExpr = result.Nodes.getNodeAs<CallExpr>("callExpr");
	    record->call_linenum =
	      result.Context->getSourceManager()
	      .getSpellingLineNumber(result.Context->getSourceManager().getSpellingLoc(record->callExpr->getLocStart()));
	    record->literal = result.Nodes.getNodeAs<StringLiteral>("reqLiteral");
	    record->linenum =
	      result.Context->getSourceManager()
	      .getSpellingLineNumber(result.Context->getSourceManager().getSpellingLoc(record->literal->getLocStart()));
	    m_parent->m_req_copy_collector.push_back(record);
	  }

	private:
	  // Parent ExecSQLToFunctionCall instance
	  ExecSQLToFunctionCall *m_parent;
	};

	enum ExecSQLToFunctionCallErrorKind
	  {
	    //
	    EXEC_SQL_2_FUNC_ERROR_NO_ERROR = 0,
	    //
	    EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA,
	    //
	    EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START,
	    //
	    EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH,
	  };

	// Emit diagnostic and eventually fix it
        void emitDiagAndFix(const SourceLocation&,
			    const SourceLocation&,
			    const std::string&);

	// Generate source file for request
	void doRequestSourceGeneration(std::map<std::string, std::string>);

	// Generate header file for request
	void doRequestHeaderGeneration(std::map<std::string, std::string>);
	
	// Emit error
	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum ExecSQLToFunctionCallErrorKind);

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

	/*
	 * Check Options
	 */
	// Generate headers option (default: false)
	const bool generate_req_headers;
	// Generate sources option (default: false)
	const bool generate_req_sources;
	// Generation directory (default: "./")
	const std::string generation_directory;
	// Request header template (default: "./pagesjaunes.h.tmpl")
	const std::string generation_header_template;
	// Request source template (default: "./pagesjaunes.pc.tmpl")
	const std::string generation_source_template;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLTOFUNCTIONCALL_H
