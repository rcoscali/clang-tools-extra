//===--- ExecSQLLOBCloseToFunctionCall.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLLOBCLOSETOFUNCTIONCALL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLLOBCLOSETOFUNCTIONCALL_H

#include "../ClangTidy.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Regex.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/json/json.hpp"

#define GENERATION_SOURCE_FILENAME_EXTENSION ".pc"
#define GENERATION_HEADER_FILENAME_EXTENSION ".h"

using namespace clang;
using namespace clang::ast_matchers;
using string2_map = std::map<std::string, std::string>;
using map_vector_string = std::map<std::string, std::vector<std::string>>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      // Checks that argument name match parameter name rules.
      class ExecSQLLOBCloseToFunctionCall : public ClangTidyCheck 
      {
      public:
	// AST Context instance
	ClangTidyContext *TidyContext;

	class SourceRangeForStringLiterals
	{
	public:
	  SourceRangeForStringLiterals() {};
	  SourceRangeForStringLiterals(SourceRange urange, SourceRange mrange, StringRef name)
	    : m_usage_range(urange),
	      m_macro_range(mrange),
	      m_macro_name(name) {};
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals& to_copy)
	  : m_usage_range(to_copy.m_usage_range),
	    m_macro_range(to_copy.m_macro_range),
	    m_macro_name(to_copy.m_macro_name) {};	  
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals const& to_copy)
	  : m_usage_range(to_copy.m_usage_range),
	    m_macro_range(to_copy.m_macro_range),
	    m_macro_name(to_copy.m_macro_name) {};
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals *to_copy)
	  : m_usage_range(to_copy->m_usage_range),
	    m_macro_range(to_copy->m_macro_range),
	    m_macro_name(to_copy->m_macro_name) {};
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals const*to_copy)
	  : m_usage_range(to_copy->m_usage_range),
	    m_macro_range(to_copy->m_macro_range),
	    m_macro_name(to_copy->m_macro_name) {};
	  SourceRangeForStringLiterals& operator =(const SourceRangeForStringLiterals & to_copy)
	  {
	    m_usage_range = to_copy.m_usage_range;
	    m_macro_range = to_copy.m_macro_range;
	    m_macro_name= to_copy.m_macro_name;
	    return *this;
	  }
	  SourceRangeForStringLiterals& operator =(SourceRangeForStringLiterals & to_copy)
	  {
	    m_usage_range = to_copy.m_usage_range;
	    m_macro_range = to_copy.m_macro_range;
	    m_macro_name= to_copy.m_macro_name;
	    return *this;
	  }
	  
	  SourceRange m_usage_range;
	  SourceRange m_macro_range;
	  StringRef m_macro_name;
	};
	class SourceRangeBefore
	{
	public:
	  bool operator()(const SourceRangeForStringLiterals &l, const SourceRangeForStringLiterals &r) const
	  {
	    if (l.m_macro_range.getBegin() == r.m_macro_range.getBegin())
	      return (l.m_macro_range.getEnd() < r.m_macro_range.getEnd());
	    
	    return (l.m_macro_range.getBegin() < r.m_macro_range.getBegin());
	  }
	};
	using source_range_set_t = std::multiset<SourceRangeForStringLiterals, SourceRangeBefore>;
	
	// Constructor
	ExecSQLLOBCloseToFunctionCall(StringRef, ClangTidyContext *);

	// Store check Options
	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	// Register matrchers
	void registerMatchers(ast_matchers::MatchFinder *) override;
	// Register our PreProcessor callback 
	void registerPPCallbacks(CompilerInstance &Compiler) override;
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
	  const VarDecl *varDecl;
	  unsigned vardecl_linenum;
	};

	// Collector for possible sprintf calls
	std::vector<struct StringLiteralRecord *> m_req_copy_collector;
	
      private:

	/**
	 * CopyRequestMatcher
	 *
	 */
	class CopyRequestMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit CopyRequestMatcher(ExecSQLLOBCloseToFunctionCall *parent)
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
	    record->varDecl = result.Nodes.getNodeAs<VarDecl>("vardecl");
	    record->vardecl_linenum =
	      result.Context->getSourceManager()
	      .getSpellingLineNumber(result.Context->getSourceManager().getSpellingLoc(record->varDecl->getLocStart()));
	    m_parent->m_req_copy_collector.push_back(record);
	  }

	private:
	  // Parent ExecSQLLOBCloseToFunctionCall instance
	  ExecSQLLOBCloseToFunctionCall *m_parent;
	};

      protected:
	/*
	 * assignment finder collector in case of a prepare request
	 * Literal is created from params and formatted with sprint 
	 * then assigned to the from :reqName.
	 */
	struct AssignmentRecord
	{
	  const BinaryOperator *binop;
	  const DeclRefExpr *lhs;
	  const DeclRefExpr *rhs;
	  unsigned binop_linenum;
	};

	// Collector for possible assignments
	std::vector<struct AssignmentRecord *> m_req_assign_collector;
	
      private:

	/**
	 * FindAssignMatcher
	 *
	 */
	class FindAssignMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit FindAssignMatcher(ExecSQLLOBCloseToFunctionCall *parent)
	    : m_parent(parent)
	  {}

	  /// The run method adding all calls in the collection vector
	  virtual void
	  run(const MatchFinder::MatchResult &result)
	  {
	    struct AssignmentRecord *record = new(struct AssignmentRecord);
	    record->lhs = result.Nodes.getNodeAs<DeclRefExpr>("lhs");
	    record->rhs = result.Nodes.getNodeAs<DeclRefExpr>("rhs");
	    record->binop = result.Nodes.getNodeAs<BinaryOperator>("binop");
	    record->binop_linenum =
	      result.Context->getSourceManager()
	      .getSpellingLineNumber(result.Context->getSourceManager().getSpellingLoc(record->binop->getLocStart()));
	    m_parent->m_req_assign_collector.push_back(record);
	  }

	private:
	  // Parent ExecSQLLOBCloseToFunctionCall instance
	  ExecSQLLOBCloseToFunctionCall *m_parent;
	};

      protected:
	/*
	 * assignment finder collector in case of a prepare request
	 * Literal is created from params and formatted with sprint 
	 * then assigned to the from :reqName.
	 */
	struct ReqFmtRecord
	{
	  const CallExpr *callExpr;
	  const DeclRefExpr *arg0;
	  unsigned callexpr_linenum;
	};

	// Collector for possible assignments
	std::vector<struct ReqFmtRecord *> m_req_fmt_collector;
	
      private:

	source_range_set_t m_macrosStringLiterals;
	
	/**
	 * FindReqFmtMatcher
	 *
	 */
	class FindReqFmtMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit FindReqFmtMatcher(ExecSQLLOBCloseToFunctionCall *parent)
	    : m_parent(parent)
	  {}

	  /// The run method adding all calls in the collection vector
	  virtual void
	  run(const MatchFinder::MatchResult &result)
	  {
	    struct ReqFmtRecord *record = new(struct ReqFmtRecord);
	    record->callExpr = result.Nodes.getNodeAs<CallExpr>("callExpr");
	    record->arg0 = result.Nodes.getNodeAs<DeclRefExpr>("arg0");
	    record->callexpr_linenum =
	      result.Context->getSourceManager()
	      .getSpellingLineNumber(result.Context->getSourceManager().getSpellingLoc(record->callExpr->getLocStart()));
	    m_parent->m_req_fmt_collector.push_back(record);
	  }

	private:
	  // Parent ExecSQLLOBCloseToFunctionCall instance
	  ExecSQLLOBCloseToFunctionCall *m_parent;
	};

	enum ExecSQLLOBCloseToFunctionCallErrorKind
	  {
	    // Error kind for no error
	    EXEC_SQL_2_FUNC_ERROR_NO_ERROR = 0,
	    // Error kind for access char data
	    EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA,
	    // Error kind for cannot find comment start
	    EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START,
	    // Error kind for comment do not match
	    EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH,
	    // Error kind for source generation failure
	    EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION,
	    // Error kind for header generation failure
	    EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION,
	  };

	// Emit diagnostic and eventually fix it
        void emitDiagAndFix(const SourceLocation&,
			    const SourceLocation&,
			    const std::string&);

	// Process a template file with values in map
	bool processTemplate(const std::string&,
			     const std::string&,
			     string2_map&);
	
	// Generate source file for request
	void doRequestSourceGeneration(DiagnosticsEngine&,
				       const std::string&,
				       string2_map&);

	// Generate header file for request
	void doRequestHeaderGeneration(DiagnosticsEngine&,
				       const std::string&,
				       string2_map&);
	
	// Emit error
	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum ExecSQLLOBCloseToFunctionCallErrorKind,
		       const std::string* msgptr = nullptr);

	// Diag id for unexpected error
	const unsigned unexpected_diag_id;
	// Diag id for no error
	const unsigned no_error_diag_id;
	// Diag id for access char data error
	const unsigned access_char_data_diag_id;
	// Diag id for cannot find comment error
	const unsigned cant_find_comment_diag_id;
	// Diag id for comment do not match error
	const unsigned comment_dont_match_diag_id;
	// Diag id for source generation failure
	const unsigned source_generation_failure_diag_id;
	// Diag id for header generation failure
	const unsigned header_generation_failure_diag_id;

	// Json for request grouping
	nlohmann::json request_groups;
	// Group structure created from json and used for
	// initializing generation groups
	map_vector_string req_groups;

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
	// Request header template for prepare requests (default: "./pagesjaunes_prepare.h.tmpl")
	const std::string generation_prepare_header_template;
	// Request source template for prepare requests (default: "./pagesjaunes_prepare.pc.tmpl")
	const std::string generation_prepare_source_template;
	// Request header template for prepare fmt requests (default: "./pagesjaunes_prepare_fmt.h.tmpl")
	const std::string generation_prepare_fmt_header_template;
	// Request source template for prepare fmt requests (default: "./pagesjaunes_prepare_fmt.pc.tmpl")
	const std::string generation_prepare_fmt_source_template;
	// Request grouping
	const std::string generation_request_groups;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLLOBCLOSETOFUNCTIONCALL_H
