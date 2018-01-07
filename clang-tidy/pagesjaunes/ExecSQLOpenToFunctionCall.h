//===--- ExecSQLOpenToFunctionCall.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLOPENTOFUNCTIONCALL_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLOPENTOFUNCTIONCALL_H

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
      class ExecSQLOpenToFunctionCall : public ClangTidyCheck 
      {
      public:

	enum ExecSQLOpenToFunctionCallErrorKind
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
	    // Error kind for source generation failure (already exists)
	    EXEC_SQL_2_FUNC_ERROR_SOURCE_EXISTS,
	    // Error kind for source generation failure (dir creation)
	    EXEC_SQL_2_FUNC_ERROR_SOURCE_CREATE_DIR,
	    // Error kind for header generation failure
	    EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION,
	    // Error kind for header generation failure
	    EXEC_SQL_2_FUNC_ERROR_HEADER_EXISTS,
	    // Error kind for header generation failure (dir creation)
	    EXEC_SQL_2_FUNC_ERROR_HEADER_CREATE_DIR,
	    // Error kind for unsupported string literal charset
	    EXEC_SQL_2_FUNC_ERROR_UNSUPPORTED_STRING_CHARSET,
	    // Error kind for invalid group file
	    EXEC_SQL_2_FUNC_ERROR_INVALID_GROUPS_FILE,
	    // Error kind for assignment not found
	    EXEC_SQL_2_FUNC_ERROR_ASSIGNMENT_NOT_FOUND,
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
	
	// Constructor
	ExecSQLOpenToFunctionCall(StringRef, ClangTidyContext *);

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
                                   const std::string&);

	// Emit error
	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum ExecSQLOpenToFunctionCallErrorKind,
		       const std::string* msgptr = nullptr);

      protected:
	/*
	 * sprintf finder collector in case of a prepare request
	 * Literal must be directly copied in the :reqName var
	 */
	struct VarDeclMatchRecord
	{
	  const VarDecl *varDecl;
	  unsigned linenum;
	  char dummy1[16];
	  char dummy2[16];
	};

	// Collector for possible sprintf calls
	std::vector<struct VarDeclMatchRecord *> m_req_var_decl_collector;
	
      private:

	/**
	 * VarDeclMatcher
	 *
	 */
	class VarDeclMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit VarDeclMatcher(ExecSQLOpenToFunctionCall *parent)
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
	  // Parent ExecSQLOpenToFunctionCall instance
	  ExecSQLOpenToFunctionCall *m_parent;
	};
	
      private:

	source_range_set_t m_macrosStringLiterals;
	
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
	
	// Find a macro string literal defined at a line
	bool findMacroStringLiteralDefAtLine(SourceManager &,
					     unsigned,
					     std::string&, std::string&,
					     SourceRangeForStringLiterals **);
	
	const VarDecl *findSymbolInFunction(ClangTool *,
					    std::string&,
					    const FunctionDecl *);
	  
        // Replace the EXEC SQL statement by the function call in the .pc file
        void replaceExecSQLinPC(void);        

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
	// Request header template (default: "./pagesjaunes_open.h.tmpl")
	const std::string generation_header_template;
	// Request source template (default: "./pagesjaunes_open.pc.tmpl")
	const std::string generation_source_template;
	// Request grouping
	const std::string generation_request_groups;
	// Simplify request args list simplification
	const bool generation_simplify_function_args;
        // boolean for reporting modifications in original .pc
        const bool generation_do_report_modification_in_pc;
        // Directory of the .pc file in which to report modifications
        const std::string generation_report_modification_in_dir;

        // Map containing comments and code to replace
        map_comment_map_replacement_values replacement_per_comment;
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_EXECSQLOPENTOFUNCTIONCALL_H
