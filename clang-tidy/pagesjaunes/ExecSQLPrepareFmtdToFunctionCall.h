//===--- ExecSQLPrepareFmtdToFunctionCall.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLPREPAREFMTDTOFUNCTIONCALL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLPREPAREFMTDTOFUNCTIONCALL_H

#include "ExecSQLCommon.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Regex.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/json/json.hpp"

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      // Checks that argument name match parameter name rules.
      class ExecSQLPrepareFmtdToFunctionCall : public ClangTidyCheck 
      {
      public:

	enum ExecSQLPrepareFmtdToFunctionCallErrorKind
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
	    // Error kind for unsupported string literal charset
	    EXEC_SQL_2_FUNC_ERROR_UNSUPPORTED_STRING_CHARSET,
	    // Error kind for invalid group file
	    EXEC_SQL_2_FUNC_ERROR_INVALID_GROUPS_FILE,
	    // Error kind for assignment not found
	    EXEC_SQL_2_FUNC_ERROR_ASSIGNMENT_NOT_FOUND,
	    // Error kind for source generation failure (already exists)
	    EXEC_SQL_2_FUNC_ERROR_SOURCE_EXISTS,
	    // Error kind for header generation failure (already exists)
	    EXEC_SQL_2_FUNC_ERROR_HEADER_EXISTS,
	    // Error kind for source generation failure (dir creation)
	    EXEC_SQL_2_FUNC_ERROR_SOURCE_CREATE_DIR,
	    // Error kind for header generation failure (dir creation)
	    EXEC_SQL_2_FUNC_ERROR_HEADER_CREATE_DIR,
	  };

	// AST Context instance
	ClangTidyContext *TidyContext;

	/**
	 * SourceRangeForStringLiterals
	 *
	 * @brief Collect data about macro expansion for string literals
	 *
	 * Instances of this class are created for each occurence of string literal 
	 * expansions in macro. Each occurence allows to keep two source ranges:
	 *  - one for the macro usage in source code (expansion usage location)
	 *  - one for the macro definition used for expansion (macro definition location)
	 * This class also keep the original macro identifier (identifier token for the 
	 * name of the macro).
	 */
	class SourceRangeForStringLiterals
	{
	public:
	  /* Default constructor */ 
	  SourceRangeForStringLiterals() {};
	  /* Explicit constructor */ 
	  SourceRangeForStringLiterals(SourceRange urange, SourceRange mrange, StringRef name)
	    : m_usage_range(urange),
	      m_macro_range(mrange),
	      m_macro_name(name) {};
	  /* Instance copy constructor */ 
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals& to_copy)
	  : m_usage_range(to_copy.m_usage_range),
	    m_macro_range(to_copy.m_macro_range),
	    m_macro_name(to_copy.m_macro_name) {};	  
	  /* Const instance copy constructor */ 
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals const& to_copy)
	  : m_usage_range(to_copy.m_usage_range),
	    m_macro_range(to_copy.m_macro_range),
	    m_macro_name(to_copy.m_macro_name) {};
	  /* Instance pointer constructor */ 
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals *to_copy)
	  : m_usage_range(to_copy->m_usage_range),
	    m_macro_range(to_copy->m_macro_range),
	    m_macro_name(to_copy->m_macro_name) {};
	  /* Const instance pointer constructor */ 
	  SourceRangeForStringLiterals(SourceRangeForStringLiterals const *to_copy)
	  : m_usage_range(to_copy->m_usage_range),
	    m_macro_range(to_copy->m_macro_range),
	    m_macro_name(to_copy->m_macro_name) {};
	  /* Const instance assignment operator */
	  SourceRangeForStringLiterals& operator =(const SourceRangeForStringLiterals & to_copy)
	  {
	    m_usage_range = to_copy.m_usage_range;
	    m_macro_range = to_copy.m_macro_range;
	    m_macro_name= to_copy.m_macro_name;
	    return *this;
	  }
	  /* Instance assignment operator */
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

	/**
	 * SourceRangeBefore
	 */
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
	
      private:

	source_range_set_t m_macrosStringLiterals;

      public:
	
	// Constructor
	ExecSQLPrepareFmtdToFunctionCall(StringRef, ClangTidyContext *);

	// Store check Options
	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	// Register matrchers
	void registerMatchers(ast_matchers::MatchFinder *) override;
	// Register our PreProcessor callback 
	void registerPPCallbacks(CompilerInstance &Compiler) override;
	// Check a matched node
	void check(const ast_matchers::MatchFinder::MatchResult &) override;

	// Emit diagnostic and eventually fix it
        std::string emitDiagAndFix(const SourceLocation&,
                                   const SourceLocation&,
                                   const std::string&,
                                   const std::string&);

	// Emit error
	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum ExecSQLPrepareFmtdToFunctionCallErrorKind,
		       const std::string* msgptr = nullptr);

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
	  explicit FindAssignMatcher(ExecSQLPrepareFmtdToFunctionCall *parent)
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
	  // Parent ExecSQLPrepareFmtdToFunctionCall instance
	  ExecSQLPrepareFmtdToFunctionCall *m_parent;
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

	/**
	 * FindReqFmtMatcher
	 *
	 */
	class FindReqFmtMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit FindReqFmtMatcher(ExecSQLPrepareFmtdToFunctionCall *parent)
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
	  // Parent ExecSQLPrepareFmtdToFunctionCall instance
	  ExecSQLPrepareFmtdToFunctionCall *m_parent;
	};

      private:

	/**
	 * VarDeclMatcher
	 *
	 */
	class VarDeclMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit VarDeclMatcher(ExecSQLPrepareFmtdToFunctionCall *parent)
	    : m_parent(parent)
	  {}

	  /// The run method adding all calls in the collection vector
	  virtual void
	  run(const MatchFinder::MatchResult &result)
	  {
	    struct VarDeclMatchRecord *record = new(struct VarDeclMatchRecord);
	    record->varDecl = result.Nodes.getNodeAs<VarDecl>("varDecl");
	    record->linenum =
	      result.Context->getSourceManager()
	      .getSpellingLineNumber(result.Context->getSourceManager().getSpellingLoc(record->varDecl->getLocStart()));
	    m_parent->m_req_var_decl_collector.push_back(record);
	  }

	private:
	  // Parent ExecSQLPrepareFmtdToFunctionCall instance
	  ExecSQLPrepareFmtdToFunctionCall *m_parent;
	};
	
      protected:

	// Collector for possible sprintf calls. struct VarDeclMatchRecord defined in ExecSQLCommon.h
	std::vector<struct clang::tidy::pagesjaunes::VarDeclMatchRecord *> m_req_var_decl_collector;

      private:
	
        // Override to be called at start of translation unit
        void onStartOfTranslationUnit();

        // Override to be called at end of translation unit
        void onEndOfTranslationUnit();

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
	
	// Find macro string literal at a line
	bool findMacroStringLiteralAtLine(SourceManager&,
					  unsigned,
					  std::string&, std::string&,
					  SourceRangeForStringLiterals**);

        // Format a string for dumping a params definition
        std::string
        createParamsDef(const std::string&,
                        const std::string&,
                        const std::string&,
                        const std::string&);

        // Format a string for dumping a params/host vars declare section
        std::string
        createParamsDeclareSection(const std::string&,
                                   const std::string&,
                                   const std::string&,
                                   const std::string&,
                                   const std::string&);

        // Format a string for dumping a params declaration
        std::string
        createParamsDecl(const std::string&,
                         const std::string&,
                         const std::string&);

        // Format a string for dumping a params declaration
        std::string
        createParamsCall(const std::string&);

        // Format a string for dumping a params declaration
        std::string
        createHostVarList(const std::string&, bool);

        // Find a symbol definition in a function, with type, line number etc
	const VarDecl *findSymbolInFunction(std::string&,
					    const FunctionDecl *);

        // Find a declaration for a named symbol in a function
        string2_map findDeclInFunction(const FunctionDecl *,
                                       const std::string&);
        
        string2_map findCXXRecordMemberInTranslationUnit(const TranslationUnitDecl *,
                                                         const std::string&,
                                                         const std::string&);
	  
        // Replace the EXEC SQL statement by the function call in the .pc file
        map_host_vars decodeHostVars(const std::string &);

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
	// Generate allow overwrite (default: true)
	const bool generate_req_allow_overwrite;
	// Generation directory (default: "./")
	const std::string generation_directory;
	// Request header template (default: "./pagesjaunes_prepare_fmt.h.tmpl")
	const std::string generation_header_template;
	// Request source template (default: "./pagesjaunes_prepare_fmt.pc.tmpl")
	const std::string generation_source_template;
	// Request grouping
	const std::string generation_request_groups;
	// Simplify request args list simplification
	const bool generation_simplify_function_args;
        // boolean for reporting modifications in original .pc
        const bool generation_do_report_modification_in_pc;
        // Directory of the .pc file in which to report modifications
        const std::string generation_report_modification_in_dir;
        // Keep EXEC SQL coments
        const bool generation_do_keep_commented_out_exec_sql;

        // Map containing comments and code to replace
        map_comment_map_replacement_values replacement_per_comment;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLPREPAREFMTDTOFUNCTIONCALL_H
