//===--- ExecSQLPrepareFmtdToFunctionCall.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <sstream>
#include <string>

#include "ExecSQLPrepareFmtdToFunctionCall.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/Regex.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using emplace_ret_t = std::pair<std::set<std::string>::iterator, bool>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      namespace {
	// Keep track of macro expansions that defines a StringLiteral
	// then keep the associated source_range_set_t to be able to get the
	// original define (for keeping indentation)
	class GetStringLiteralsDefines : public PPCallbacks {
	public:
	  explicit GetStringLiteralsDefines(ExecSQLPrepareFmtdToFunctionCall *parent,
					    ExecSQLPrepareFmtdToFunctionCall::source_range_set_t *srset)
	    : m_parent(parent),
	      m_src_mgr(m_parent->TidyContext->getASTContext()->getSourceManager()),
	      m_macrosStringLiteralsPtr(srset)
	  {
	  }
	  
	  void
	  MacroExpands(const Token &macroNameTok,
		       const MacroDefinition &md, SourceRange range,
		       const MacroArgs *args) override
	  {
	    FileID main_fid = m_src_mgr.getMainFileID();
	    SourceLocation defloc = md.getMacroInfo()->getDefinitionLoc();
	    StringRef macroName = macroNameTok.getIdentifierInfo()->getName();
	    std::string macroLiteral;
	    FileID name_fid = m_src_mgr.getFileID(defloc);

	    if (name_fid == main_fid)
	      {
		SourceRange sr;
		bool have_literal = false;
		for (const auto& t : md.getMacroInfo()->tokens())
		  {
		    if (t.is(tok::string_literal))
		      {
			//unsigned tokenLength = t.getLength();
			SourceLocation sloc = t.getLocation();
			SourceLocation eloc = t.getEndLoc();
			sr = SourceRange(sloc, eloc);
			//outs() << "Token Length: '" << tokenLength  << "'\n";
			//outs() << "    value: '" << litdata << "'\n";
			//macroLiteral = litdata.str();
			have_literal = true;
		      }
		    else if (t.is(tok::wide_string_literal) ||
			     t.is(tok::angle_string_literal) ||
			     t.is(tok::utf8_string_literal) ||
			     t.is(tok::utf16_string_literal) ||
			     t.is(tok::utf32_string_literal))
		      {
			outs() << "*** Token for weird string (wide, utf etc) found\n";
		      }
		  }
		
		if (have_literal)
		  {
		    //StringRef extrait(macroLiteral.c_str(), (25 < macroLiteral.length() ? 25 : macroLiteral.length()));
		    outs() << "Adding macro '" << macroName << "' expansion at " << range.getBegin().printToString(m_src_mgr) << "...'\n";
		    ExecSQLPrepareFmtdToFunctionCall::SourceRangeForStringLiterals *ent
		      = new ExecSQLPrepareFmtdToFunctionCall::SourceRangeForStringLiterals(range, sr, macroName, StringRef(""));
		    m_macrosStringLiteralsPtr->insert(ent);
		  }
	      }
	  }

	  void
	  EndOfMainFile() override
	  {
	    outs() << "!!!***> END OF MAIN FILE !\n";
	  }
	  
	private:
	  ExecSQLPrepareFmtdToFunctionCall *m_parent;
	  const SourceManager &m_src_mgr; 
	  ExecSQLPrepareFmtdToFunctionCall::source_range_set_t *m_macrosStringLiteralsPtr;
	};
      } // namespace
      
      /**
       * ExecSQLPrepareFmtdToFunctionCall constructor
       *
       * @brief Constructor for the ExecSQLPrepareFmtdToFunctionCall rewriting check
       *
       * The rule is created a new check using its \c ClangTidyCheck base class.
       * Name and context are provided and stored locally.
       * Some diag ids corresponding to errors handled by rule are created:
       * - unexpected_diag_id: Unexpected error
       * - no_error_diag_id: No error
       * - access_char_data_diag_id: Couldn't access memory buffer for comment (unexpected)
       * - cant_find_comment_diag_id: Comment not available (unexpected)
       * - comment_dont_match_diag_id: Invalid comment structure (unexpected)
       * - source_generation_failure_diag_id: Request source file generation failed (unexpected)
       * - header_generation_failure_diag_id: Request header file generation failed (unexpected)
       *
       * @param Name    A StringRef for the new check name
       * @param Context The ClangTidyContext allowing to access other contexts
       */
      ExecSQLPrepareFmtdToFunctionCall::ExecSQLPrepareFmtdToFunctionCall(StringRef Name,
						   ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),	/** Init check (super class) */
	  TidyContext(Context),			/** Init our TidyContext instance */
	  /** Unexpected error occured */
	  unexpected_diag_id(Context->getASTContext()->getDiagnostics().
			     getCustomDiagID(DiagnosticsEngine::Warning,
					     "Unexpected error occured?!")),
	  /** No error: never thrown */
	  no_error_diag_id(Context->getASTContext()->getDiagnostics().
			   getCustomDiagID(DiagnosticsEngine::Ignored,
					   "No error")),
	  /** Access char data error occured */
	  access_char_data_diag_id(Context->getASTContext()->getDiagnostics().
				   getCustomDiagID(DiagnosticsEngine::Error,
						   "Couldn't access character data in file cache memory buffers!")),
	  /** Cannot find comment error */
	  cant_find_comment_diag_id(Context->getASTContext()->getDiagnostics().
				    getCustomDiagID(DiagnosticsEngine::Error,
						    "Couldn't find ProC comment start! This result has been discarded!")),
	  /** Cannot parse comment as a ProC SQL rqt statement */
	  comment_dont_match_diag_id(Context->getASTContext()->getDiagnostics().
				     getCustomDiagID(DiagnosticsEngine::Error,
						     "Couldn't match ProC comment for function name creation!")),
	  /** Cannot generate one request source file */
	  source_generation_failure_diag_id(Context->getASTContext()->getDiagnostics().
					    getCustomDiagID(DiagnosticsEngine::Error,
							    "Couldn't generate request source file %0!")),
	  /** Cannot generate one request header file */
	  header_generation_failure_diag_id(Context->getASTContext()->getDiagnostics().
					    getCustomDiagID(DiagnosticsEngine::Error,
							    "Couldn't generate request header file %0!")),
	  /*
	   * Options
	   */
	  /// Generate requests header files (bool)
	  generate_req_headers(Options.get("Generate-requests-headers",
					   false)),
	  /// Generate requests source files (bool)
	  generate_req_sources(Options.get("Generate-requests-sources",
					   false)),
	  /// Generation directory (string)
	  generation_directory(Options.get("Generation-directory",
					   "./")),
	  /// Generation header default template (string)
	  generation_header_template(Options.get("Generation-header-template",
						 "./pagesjaunes_prepare_fmt.h.tmpl")),
	  /// Generation source default template (string)
	  generation_source_template(Options.get("Generation-source-template",
						 "./pagesjaunes_prepare_fmt.pc.tmpl")),
	  /// Request grouping option: Filename containing a json map for
	  /// a group name indexing a vector of requests name
	  generation_request_groups(Options.get("Generation-request-groups",
						"./request_groups.json"))
      {
	//outs() << "Opening request group file: '" << generation_request_groups << "'\n";

	req_groups.clear();
	std::filebuf fb;
	if (fb.open(generation_request_groups, std::ios::in))
	  {
	    std::istream is(&fb);
	    is >> request_groups;
	    std::map<std::string, nlohmann::json> reqgroups = request_groups.get<std::map<std::string, nlohmann::json>>();
	    
	    for (auto it = reqgroups.begin();
		 it != reqgroups.end();
		 ++it)
	      {
		//outs() << "Request group: " << it->first << "\n";
		std::vector<std::string> agroup = it->second.get<std::vector<std::string>>();
		req_groups.emplace(it->first, agroup);
		// for (auto it2 = agroup.begin();
		//      it2 != agroup.end();
		//      ++it2)
		//   {
		//     outs() << "    " << (*it2) << "\n";
		//   }
	      }
	  }
	else
	  {
	    errs() << "Cannot load groups file: '" << generation_request_groups << "'\n";
	  }
      }
      
      /**
       * storeOptions
       *
       * @brief Store options for this check
       *
       * This check support one option for customizing comment regex 
       * - Generate-requests-headers
       * - Generate-requests-sources
       * - Generation-directory
       * - Generation-header-template
       * - Generation-source-template
       * - Generation-prepare-header-template
       * - Generation-prepare-source-template
       *
       * @param Opts	The option map in which to store supported options
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
	Options.store(Opts, "Generate-requests-headers", generate_req_headers);
	Options.store(Opts, "Generate-requests-sources", generate_req_sources);
	Options.store(Opts, "Generation-directory", generation_directory);
	Options.store(Opts, "Generation-header-template", generation_header_template);
	Options.store(Opts, "Generation-source-template", generation_source_template);
	Options.store(Opts, "Generation-request-groups", generation_request_groups);
      }
      
      /**
       * registerMatchers
       *
       * @brief Register the ASTMatcher that will found nodes we are interested in
       *
       * This method register 1 matcher for each oracle ProC generated statement
       * to rewrite. 
       * The matcher bind elements we will use, for detecting the found statement 
       * we want to rewrite , and for writing new code.
       *
       * @param Finder  the recursive visitor that will use our matcher for sending 
       *                us AST node.
       */
      void 
      ExecSQLPrepareFmtdToFunctionCall::registerMatchers(MatchFinder *Finder) 
      {
	/* Add a matcher for finding compound statements starting */
	/* with a sqlstm variable declaration */
        Finder->addMatcher(varDecl(
		      hasAncestor(declStmt(hasAncestor(compoundStmt(hasAncestor(functionDecl().bind("function"))).bind("proCBlock")))),
		      hasName("sqlstm"))
			   , this);
      }

      /**
       * ExecSQLPrepareFmtdToFunctionCall::registerPPCallbacks
       *
       * @brief Register callback for intercepting all pre-processor actions
       *
       * Allows to register a callback for executing our actions at every C/C++ 
       * pre-processor processing. Thanks to this callback we will collect all string
       * literal macro expansions.
       *
       * @param[in] compiler	the compiler instance we will intercept
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::registerPPCallbacks(CompilerInstance &compiler)
      {
	compiler
	  .getPreprocessor()
	  .addPPCallbacks(llvm::make_unique<GetStringLiteralsDefines>(this,
								      &m_macrosStringLiterals));
      }
      
      
      /*
       * ExecSQLPrepareFmtdToFunctionCall::emitDiagAndFix
       *
       * @brief Emit a diagnostic message and possible replacement fix for each
       *        statement we will be notified with.
       *std::unique_ptr<PPCallbacks>
       * This method is called each time a statement to handle (rewrite) is found.
       * One replacement will be emited for eachnode found.
       * It is passed all necessary arguments for:
       * - creating a comprehensive diagnostic message
       * - computing the locations of code we will replace
       * - computing the new code that will replace old one
       *
       * @param loc_start       The CompoundStmt start location
       * @param loc_end         The CompoundStmt end location
       * @param function_name   The function name that will be called
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::emitDiagAndFix(const SourceLocation& loc_start,
					    const SourceLocation& loc_end,
					    const std::string& function_name)
      {
	/* Range of the statement to change */
	SourceRange stmt_range(loc_start, loc_end);

	/* Diagnostic builder for a found AST node */
	/* Default is a warning, and it is emitted */
	/* as soon as the diag builder is destroyed */
	DiagnosticBuilder mydiag = diag(loc_end,
					"ProC Statement Block shall be replaced"
					" by a function call named '%0'")
	  << function_name;

	/* Replacement code built */
	std::string replt_code = function_name;
	replt_code.append(std::string("();"));

	/* Emit the replacement over the found statement range */
	mydiag << FixItHint::CreateReplacement(stmt_range, replt_code);
      }

      /**
       * ExecSQLPrepareFmtdToFunctionCall::processTemplate
       *
       * @brief Process a template file with values from the 
       *        provided map
       *
       * @param[in] tmpl	Template file pathname
       * @param[in] fname	Output file pathname
       * @param[in] values_map	Map containing values to be replaced
       *
       * @retval true 	if template was processed and file was created
       * @retval false 	if something wrong occurs
       */
      bool
      ExecSQLPrepareFmtdToFunctionCall::processTemplate(const std::string& tmpl,
					     const std::string& fname,
					     string2_map& values_map)
      {
	// Return value
	bool ret = false;
	// File buffers used for I/O
	std::filebuf fbi, fbo;

	do
	  {
	    // Open template
	    if (!fbi.open(tmpl, std::ios::in))
	      break;

	    // Open output file
	    if (!fbo.open(fname, std::ios::out))
	      break;

	    // Input and output streams
	    std::istream is(&fbi);
	    std::ostream os(&fbo);

	    // While not end of template
	    while (is)
	      {
		// Buffer for input line
		char buf[1024];
		// Read line in buffer
		is.getline(buf, 255);
		std::string aline = buf;
		// Iterate on all map values
		for (auto it = values_map.begin();
		     it != values_map.end();
		     ++it)
		  {
		    // Position of found value name
		    size_t pos = 0;
		    do
		      {
			// If we found the current value name
			if ((pos = aline.find(it->first, pos)) != std::string::npos)
			  {
			    // Erase value name at found position in input line
			    aline.erase(pos, it->first.length());
			    // Insert value replacement at the same position
			    aline.insert(pos, it->second);
			  }
		      }
		    // Do it until end of line
		    while (pos != std::string::npos);
		  }

		// Output processed line
		os << aline << std::endl;
	      }

	    // All was ok
	    ret = true;
	  }
	while (0);

	// Close output file
	if (fbo.is_open())
	  fbo.close();
	// Close input file
	if (fbi.is_open())
	  fbi.close();

	// Return status
	return ret;
      }

      /**
       * ExecSQLPrepareFmtdToFunctionCall::doRequestSourceGeneration
       *
       * @brief Generate source file for request from template
       *
       * This method calls templating engine for creating a source
       * file for the request described in the map. If something goes 
       * wrong an error is emitted.
       *
       * @param[in] tmpl         Template file pathname
       * @param[in] values_map   Map containing values for replacement strings
       *
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::doRequestSourceGeneration(DiagnosticsEngine& diag_engine,
						       const std::string& tmpl,
						       string2_map& values_map)
      {
	SourceLocation dummy;
	// Compute output file pathname
	std::string fileName = values_map["@RequestFunctionName@"];
	fileName.append(".cpp");
	fileName.insert(0, "/");
	fileName.insert(0, generation_directory);
	// Process template for creating source file
	if (!processTemplate(tmpl, fileName, values_map))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION,
		    &fileName);
      }
      
      /**
       * ExecSQLPrepareFmtdToFunctionCall::doRequestHeaderGeneration
       *
       * @brief Generate header file for request from template
       *
       * This method calls templating engine for creating a header
       * file for the request described in the map. If something goes 
       * wrong an error is emitted.
       *
       * @param[in] tmpl         Template file pathname
       * @param[in] values_map   Map containing values for replacement strings
       *
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::doRequestHeaderGeneration(DiagnosticsEngine& diag_engine,
						       const std::string& tmpl,
						       string2_map& values_map)
      {	
	SourceLocation dummy;
	// Compute output file pathname
	std::string fileName = values_map["@RequestFunctionName@"];
	fileName.append(".h");
	fileName.insert(0, "/");
	fileName.insert(0, generation_directory);
	// Process template for creating header file
	if (!processTemplate(tmpl, fileName, values_map))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION,
		    &fileName);
      }
	

      
      /**
       * emitError
       *
       * @brief Manage error conditions by emiting an error
       *
       * This method manage any error condition by emitting a specific error message
       * to the LLVM/Clang DiagnosticsEngine. It uses diag ids that were created 
       * in constructor.
       *
       * @param diag_engine     LLVM/Clang DiagnosticsEngine instance
       * @param err_loc         Error location
       * @param kind            Kind of error to report
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::emitError(DiagnosticsEngine &diag_engine,
				       const SourceLocation& err_loc,
				       enum ExecSQLPrepareFmtdToFunctionCallErrorKind kind,
				       const std::string* msgptr)
      {
	std::string msg;
	if (msgptr != nullptr && !msgptr->empty())
	  msg = *msgptr;
	
	/* 
	 * According to the kind of error, it is reported through 
	 * diag engine.
	 */
	switch (kind)
	  {
	    /** Default unexpected diagnostic id */
	  default:
	    diag_engine.Report(err_loc, unexpected_diag_id);
	    break;

	    /** No error ID: it should never occur */
	  case ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_NO_ERROR:
	    diag_engine.Report(err_loc, no_error_diag_id);
	    break;

	    /** Access char data diag ID */
	  case ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA:
	    diag_engine.Report(err_loc, access_char_data_diag_id);
	    break;

	    /** Can't find a comment */
	  case ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START:
	    diag_engine.Report(err_loc, cant_find_comment_diag_id);
	    break;

	    /** Cannot match comment */
	  case ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH:
	    diag_engine.Report(err_loc, comment_dont_match_diag_id);
	    break;

	    /** Cannot generate request source file (no location) */
	  case ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION:
	    diag_engine.Report(source_generation_failure_diag_id).AddString(msg);
	    break;

	    /** Cannot generate request header file (no location) */
	  case ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION:
	    diag_engine.Report(header_generation_failure_diag_id).AddString(msg);
	    break;
	  }
      }

      /**
       * ExecSQLPrepareFmtdToFunctionCall::findMacroStringLiteralAtLine
       *
       * @brief find a macro expansion for a string literal used for a request
       *
       * Search the set of macro expansion collected from Pre Processor work.
       * This set contains string literals and macro names used for request formatting.
       * String literals values preserve the original source indentation.
       *
       * @param[in]    src_mgr	the source manager for translating locations 
       *                        in line nums
       * @param[in]    ln	the origin location line number for which we 
       *                        want string literal
       * @param[inout] name	the name of the macro
       * @param[inout] val	the value of the string literal
       * @param[out]   record	the pointer to the found record is one was found.
       *                        if no record were found a null is returned
       *
       */
      bool
      ExecSQLPrepareFmtdToFunctionCall::findMacroStringLiteralAtLine(SourceManager &src_mgr,
								     unsigned ln,
								     std::string& name, std::string& val,
								     SourceRangeForStringLiterals **record = nullptr)
      {
	bool ret = false;
	if (record)
	  *record = nullptr;
	for (auto it = m_macrosStringLiterals.begin();
	     it != m_macrosStringLiterals.end();
	     ++it)
	  {
	    auto sr = (*it);
	    SourceLocation sl = sr.m_usage_range.getBegin();
	    unsigned sln = src_mgr.getSpellingLineNumber(sl);
	    outs() << "Macro usage at line#" << sln << " versus searching line#" << ln << " or line #" << ln+1 << "\n";
	    if (sln == ln || sln == ln +1)
	      {
		name = sr.m_macro_name.str();
		if (record)
		  *record = new SourceRangeForStringLiterals(sr);
		unsigned ldl =
		  src_mgr.getFileOffset(src_mgr.getFileLoc(sr.m_macro_range.getEnd()))
		  - src_mgr.getFileOffset(src_mgr.getFileLoc(sr.m_macro_range.getBegin()));
		StringRef lstr(src_mgr.getCharacterData(src_mgr.getFileLoc(sr.m_macro_range.getBegin())),
			       ldl);
		val = lstr.str();
		ret = true;
		break;
	      }
	  }

	return ret;
      }
      
      /**
       * check
       *
       * @brief This method is called each time a visited AST node matching our 
       *        ASTMatcher is found.
       * 
       * This method will navigated and inspect the found AST nodes for:
       * - determining if the found nodes are elligible for rewrite
       * - extracting all necessary informations for computing rewrite 
       *   location and code (find ProC generated comment)
       *
       * @param Result          The match result provided by the recursive visitor
       *                        allowing us to access AST nodes bound to variables
       */
      void
      ExecSQLPrepareFmtdToFunctionCall::check(const MatchFinder::MatchResult &result) 
      {
	// Get the source manager
	SourceManager &srcMgr = result.Context->getSourceManager();
	DiagnosticsEngine &diagEngine = result.Context->getDiagnostics();
	ClangTool *tool = TidyContext->getToolPtr();

	/*
	 * Init check context
	 * ------------------
	 */
	
	// Get the compound statement AST node as the bounded var 'proCBlock'
	const CompoundStmt *stmt = result.Nodes.getNodeAs<CompoundStmt>("proCBlock");
	const FunctionDecl *curFunc = result.Nodes.getNodeAs<FunctionDecl>("function");

	// Get start/end locations for the statement
	SourceLocation loc_start = stmt->getLocStart();
	SourceLocation loc_end = stmt->getLocEnd();

	// Get file ID in src mgr
	FileID startFid = srcMgr.getFileID(loc_start);
	// Get compound statement start line num
	unsigned startLineNum = srcMgr.getLineNumber(startFid, srcMgr.getFileOffset(loc_start));

	// outs() << "Found one result at line " << startLineNum << "\n";
	// outs() << "Generate_req_headers: " << (generate_req_headers ? "true" : "false") << "\n";
	// outs() << "Generate_req_sources: " << (generate_req_sources ? "true" : "false") << "\n";
	// outs() << "Generation_directory: '" << generation_directory << "'\n";
	// outs() << "Generation_header_template: '" << generation_header_template << "'\n";
	// outs() << "Generation_source_template: '" << generation_source_template << "'\n";

	/*
	 * Find the comment for the EXEC SQL statement
	 * -------------------------------------------
	 *
	 * Iterate from line -2 to line -5 until finding the whole EXEC SQL comment
	 * comments are MAX_NUMBER_OF_LINE_TO_SEARCH lines max
	 */
	
	// Current line number
	size_t lineNum = startLineNum-2;
	
	// Get end comment loc
	SourceLocation comment_loc_end = srcMgr.translateLineCol(startFid, lineNum, 1);
	SourceLocation comment_loc_start;
	
	// Does error occured while doing getCharacterData 
	bool errOccured = false;
	
	// Get comment C string from the file memory buffer
	const char *commentCStr = srcMgr.getCharacterData(comment_loc_end, &errOccured);
	
	// The line data & comment
	std::string lineData;

	// Offset of comment start in file
	size_t commentStart = 0;
	
	/*
	 * Finding loop
	 * ------------
	 */
	do
	  {
	    // All was ok ?
	    if (!errOccured)
	      {
		// Assign comment C string to line data var
		lineData.assign(commentCStr);
		// And remove all remaining chars after end of line
		lineData.erase (lineData.find("\n", 1), std::string::npos);
		
		// Try to get comment start in current line 
		commentStart = lineData.find("/*");
		
		// If found, break the loop
		if (commentStart != std::string::npos)
		  break;
	      }
	    
	    else
	      // Error occured at getting char data ! break comment start find loop
	      break;

	    // try previous line
	    lineNum--;
	    // Get Location for the new current line
	    comment_loc_start = srcMgr.translateLineCol(startFid, lineNum, 1);
	    // And get char data again
	    commentCStr = srcMgr.getCharacterData(comment_loc_start, &errOccured);
	  }
	// Try again
	while (lineNum > 0);

	/*
	 * Comment found
	 */
	std::string comment;
	std::string function_name;

	// If found comment start & no error occured
	if (lineNum > 0 && !errOccured)
	  {
	    // Assign comment char data in the comnt string
	    comment = std::string(commentCStr);

	    /*
	     * Erase from end of comment to end of data
	     */

	    // Find end of comment
	    size_t endCommentPos = comment.find("*/", 1);
	    
	    // Erase until end of character data
	    comment.erase (endCommentPos +2, std::string::npos);

	    /*
	     * Remove all \n from comment
	     */
	    
	    // Find CR from start in comment
	    size_t crpos = comment.find("\n", 0);

	    // Iterate
	    do
	      {
		// If one has been found
		if (crpos != std::string::npos)
		  {
		    // Erase CR
		    comment.erase(crpos, 1);
		  }
		// Find again CR
		crpos = comment.find("\n", 0);
	      }
	    // Until no more is found
	    while (crpos != std::string::npos);

	    outs() << "comment for compound statement at line #" << startLineNum << ": '" << comment << "'\n";

	    /*
	     * Create function call for the request
	     */

	    // Regex for processing PREPARE comment
	    Regex reqRePrep("^.*EXEC SQL[ \t]+(prepare|PREPARE)[ \t]+([A-Za-z0-9]+)[ \t]+(from|FROM)[ \t]+:([A-Za-z]+);.*$");
	    
	    // Returned matches
	    SmallVector<StringRef, 8> matches;

	    /*
	     * First let's try to match comment against a regex designed for prepape requests
	     * ==============================================================================
	     */
	    if (reqRePrep.match(comment, &matches))
	      {
		/*
		 * Find the request literal
		 */
		
		// The request name used in ProC (syntax :reqName)
		std::string fromReqName = matches[4];
		std::string reqName = matches[2];
		// Some result that will be passed to the templating engine
		std::string requestDefineValue;
		std::string fromReqNameLength;

		
		outs() << "!!!*** Prepare request comment match: from request name is '" << fromReqName
		       << "' and request name is '"<< reqName << "'\n";

		// we try with a form of prepare request, one using an intermediate
		// char * variable and a sprintf
		// something as:
		//   char *req;
		//   char reqBuf[1234];
		//   sprintf(req, fmt, ...);
		//   req = reqBuf
		//   EXEC SQL prepape aRequest from :req;
		// Let's try with <something> = <request> statement;
		
		// Statement matcher for a binary operator = 
		StatementMatcher m_matcher
		  // Search for a binary operator = (assignment)
		  =	binaryOperator(hasOperatorName("="),
				       // The LHS (something) is the one from the 'EXEC SQL from :something' (bound to lhs)
				       hasLHS(declRefExpr(hasDeclaration(namedDecl(hasName(fromReqName.c_str())))).bind("lhs")),
				       // The RHS is bound to rhs var
		                       hasRHS(hasDescendant(declRefExpr().bind("rhs"))),
				       // In the current function. Binary operator is bound to binop
                                       hasAncestor(functionDecl(hasName(curFunc->getNameAsString())))).bind("binop");

		// Prepare matcher finder for our customized matcher
		FindAssignMatcher faMatcher(this);
		MatchFinder finder;
		finder.addMatcher(m_matcher, &faMatcher);
		// Clear collector vector
		m_req_assign_collector.clear();
		// Run the visitor pattern for collecting matches
		tool->run(newFrontendActionFactory(&finder).get());

		// Debug display
		outs() << "Found " << m_req_assign_collector.size() << " assignments\n";

		// Declarations for vars we will provide to templating engine 
		std::string requestFormatArgsDef, requestFormatArgsUsage;
		std::string requestLiteralDefName, requestLiteralDefValue;
		std::string sprintfTarget, requestInterName;
				
		// The collected records
		struct AssignmentRecord *lastRecord = (struct AssignmentRecord *)nullptr;

		// If we found something, 
		if (!m_req_assign_collector.empty())
		  {
		    // Let's iterate over all collected matches
		    for (auto it = m_req_assign_collector.begin();
			 it != m_req_assign_collector.end();
			 ++it)
		      {
			// And find the one just before the ProC statement
			struct AssignmentRecord *record = (*it);
			if (record->binop_linenum > startLineNum)
			  break;
			// Didn't find yet, keep current as the last one
			lastRecord = record;
		      }
		    
		    // Last record collected just before the ProC statement
		    if (lastRecord != nullptr)
		      {
			QualType fromType = lastRecord->rhs->getType().withConst();
			CharUnits cu = TidyContext->getASTContext()->getTypeSizeInChars(fromType);
			size_t fromReqNameLengthSize = (size_t)cu.getQuantity();
			std::stringbuf buffer;      // empty stringbuf
			std::ostream bos (&buffer);  // associate stream buffer to stream
			bos << fromReqNameLengthSize;
			fromReqNameLength = buffer.str();
			outs() << "Found assignment at line #" << lastRecord->binop_linenum
			       << ": '" << lastRecord->lhs->getFoundDecl()->getNameAsString()
			       << " = " << lastRecord->rhs->getFoundDecl()->getNameAsString() << "'\n";
			sprintfTarget.assign(lastRecord->rhs->getFoundDecl()->getNameAsString());
			requestInterName.assign(lastRecord->lhs->getFoundDecl()->getNameAsString());

			/*
			 * Get the sprintf call defining the request
			 */
			    
			// Declaration matcher for the request string literal
			StatementMatcher m_matcher2
			  = callExpr(hasDescendant(declRefExpr(hasDeclaration(namedDecl(hasName("sprintf"))))),
				     hasArgument(0,
						 declRefExpr(hasDeclaration(varDecl(namedDecl(hasName(sprintfTarget.c_str()))))).bind("arg0")),
				     hasAncestor(functionDecl(hasName(curFunc->getNameAsString().insert(0, "::"))))).bind("callExpr");
		    

			// Prepare matcher finder for our customized matcher
			FindReqFmtMatcher reqFmtMatcher(this);
			MatchFinder finder;
			finder.addMatcher(m_matcher2, &reqFmtMatcher);
			// Clear collector vector
			m_req_fmt_collector.clear();
			// Run the visitor pattern for collecting matches
			tool->run(newFrontendActionFactory(&finder).get());

			outs() << "Found " << m_req_fmt_collector.size() << " sprintf formatters\n";

			// The collected records
			struct ReqFmtRecord *lastFmtRecord = (struct ReqFmtRecord *)NULL;

			// Let's iterate over all collected matches
			if (!m_req_fmt_collector.empty())
			  {
			    for (auto it = m_req_fmt_collector.begin();
				 it != m_req_fmt_collector.end();
				 ++it)
			      {
				// And find the one just before the ProC statement
				struct ReqFmtRecord *record = (*it);
				if (record->callexpr_linenum > startLineNum)
				  break;
				// Didn't find yet
				lastFmtRecord = record;
			      }

			    // Last record collected just before the ProC statement
			    if (lastFmtRecord)
			      {
				outs() << "Found sprintf formater at line #" << lastFmtRecord->callexpr_linenum << "\n";
				unsigned int num_args = lastFmtRecord->callExpr->getNumArgs();
				outs() << "    Formater has " << num_args << " args \n";

				// A set for enforcing uniqueness of args for function args definition/declaration
				std::set<std::string> args_set;
				args_set.clear();
				emplace_ret_t emplace_ret;
				      
				// Iterate on all args after the second one (args index start at 0)
				for (unsigned int num_arg = 1; num_arg < num_args; num_arg++)
				  {
				    outs() << "Processing arg #" << num_arg << "\n";
				    // Let's get arg
				    const Expr *arg; 
				    const Expr *upper_arg = lastFmtRecord->callExpr->getArg(num_arg);
				    // Check if arg is null
				    if (upper_arg != nullptr)
				      {
					// Avoid implicit casts
					arg = upper_arg->IgnoreImpCasts();

					//outs() << "arg = " << arg << "\n";

					// If we are processing first argument, and this arg is a string literal
					if (num_arg == 1 && arg && isa<StringLiteral>(*arg))
					  {
					    SourceRangeForStringLiterals *sr;
					      
					    if (findMacroStringLiteralAtLine(srcMgr,
									     lastFmtRecord->callexpr_linenum,
									     requestLiteralDefName, requestLiteralDefValue,
									     &sr))
					      {
						unsigned literalStartLineNumber = srcMgr.getSpellingLineNumber(sr->m_macro_range.getBegin());
						unsigned literalEndLineNumber = srcMgr.getSpellingLineNumber(sr->m_macro_range.getEnd());
						outs() << "*>*>*> Request Literal '" << requestLiteralDefName << "'from line #"
						       << literalStartLineNumber << " to line #" << literalEndLineNumber << "\n";
						outs() << "\"" << requestLiteralDefValue <<"\"\n";
					      }
					    else
					      {
						errs() << "ERROR !! Didn't found back macro expansion for "
						       << "the string literal used at line number " << lastFmtRecord->callexpr_linenum << "\n";
					      }
					  }
					// First arg is not a string literal ...
					else if (arg && num_arg == 1)
					  {
					    // TODO: handle this case: checking in AST
					    errs() << "!!! *** ERROR: First arg is expected to be a string literal but it is not the case here !!!\n";
					    outs() << "!!! *** ERROR: First arg is expected to be a string literal but it is not the case here !!!\n";
					  }
					// Other args
					else if (arg)
					  {
					    if (isa<DeclRefExpr>(*arg))
					      {
						// Get the decl ref expr
						const DeclRefExpr &argExpr = llvm::cast<DeclRefExpr>(*arg);
						//outs() << "got argExpr\n";
						DeclarationNameInfo dni = argExpr.getNameInfo();
						//outs() << "decl name = " << dni.getName().getAsString() << "\n";
						std::string declName = dni.getName().getAsString();
						// Append arg name
						requestFormatArgsUsage.append(declName);
						// Get the qualified type for this arg
						QualType qargt = argExpr.getDecl()->getType();
						// And the type pointer
						const Type *argt = qargt.getTypePtr();

						outs() << "arg type = " << argt << "\n";

						// If arg is an array
						if (argt->isArrayType())
						  {
						    // Get the ConstantArrayType for it (it is mandatory a constant array type in our
						    // case, remember it is provided to sprintf format string)
						    // TODO: Perhaps we should check this
						    const ConstantArrayType *arrt = argExpr.getDecl()->getASTContext().getAsConstantArrayType(qargt);

						    outs() << "arr type = " << arrt << "\n";

						    // Check uniqueness of arg for def/decl by adding name to the dedicated set
						    emplace_ret = args_set.emplace(declName);

						    outs() << "emplace in set = " << (emplace_ret.second ? "SUCCESS" : "FAILURE") << "\n";

						    // If could emplace it, it means it don't already exists in args list
						    if (emplace_ret.second)
						      {
							// First add coma for the third and after
							if (num_arg > 2)
							  requestFormatArgsDef.append(", ");
							outs() << "append ,\n";

							// So we add it, first the element type
							requestFormatArgsDef.append(arrt->getElementType().getAsString());
							outs() << "append element type " << arrt->getElementType().getAsString() << "\n";

							// Then a space
							requestFormatArgsDef.append(" ");
							outs() << "append space\n";

							// Then the param name
							requestFormatArgsDef.append(declName);
							outs() << "append name " << declName << "\n";

							// An openning bracket before the size
							requestFormatArgsDef.append("[");
							outs() << "append open bracket\n";

							// The size as a number
							std::string arrsz = arrt->getSize().toString(10, false);
							outs() << "size of array = " << arrsz << "\n";
						    
							// // Formatted in a string
							// std::stringbuf buf;
							// // Through an output stream
							// std::ostream bos (&buf);  // associate stream buffer to stream
							// bos << arrsz;
							// outs() << "formatted as = " << buf.str() << "\n";
							requestFormatArgsDef.append(arrsz);

							// And finally the closing bracket
							requestFormatArgsDef.append("]");
							outs() << "append close bracket\n";
						      }
						  }
						// Type is builting type
						else
						  {
						    // Try to add to args_set for uniqueness enforcing
						    emplace_ret = args_set.emplace(declName);
						    // We were able to add this arg to the set
						    if (emplace_ret.second)
						      {
							// First add coma for the third and after
							if (num_arg > 2)
							  {
							    requestFormatArgsDef.append(", ");
							    outs() << "append coma\n";
							  }
						    
							// Se we can add to the def/decl, first the type name
							requestFormatArgsDef.append(qargt.getAsString());
							outs() << "append qargt name: " << qargt.getAsString() << "\n";
						    
							// Then a space
							requestFormatArgsDef.append(" ");
							outs() << "append space\n";

						    
							// and the param name
							requestFormatArgsDef.append(declName);
							outs() << "append decl name: " << declName << "\n";
						      }
						  }
						if (num_arg <num_args-1)
						  requestFormatArgsUsage.append(", ");
					      }
					  }
					else
					  {
					    errs() << "ERROR !! Arg is not castable in DeclRefExpr\n";
					  }
				      }
				    else
				      // Something failed about getting argument !!
				      // TODO: understand this weird case !!!
				      errs() << "ERROR !!! Cannot generate request " << sprintfTarget << "\n";
				  }

				outs() << "args set clear\n";
				args_set.clear();
				    
				// Display generated function args
				outs() << "requestFormatArgsUsage = '" << requestFormatArgsUsage << "'\n";
				outs() << "requestFormatArgsDef = '" << requestFormatArgsDef << "'\n";
			      }
			  }

			// Compute function name
			function_name.assign("prepare");
			// Get match in rest string
			std::string rest(reqName);
			// Capitalize it
			rest[0] &= ~0x20;
			// And append to function name
			function_name.append(rest);
			outs() << "function name = " << function_name << "\n";
			    
			// 'EXEC SQL' statement computation
			// => prepare reqName from :fromReqName
			std::string requestExecSql = "prepare ";
			requestExecSql.append(reqName);
			requestExecSql.append(" from :");
			requestExecSql.append(fromReqName);
			outs() << "exec sql = " << requestExecSql << "\n";

			// If header generation was requested
			if (generate_req_headers)
			  {
			    // Build the map
			    string2_map values_map;
			    values_map["@RequestFunctionName@"] = function_name;
			    values_map["@RequestFormatArgsDecl@"] = requestFormatArgsDef;
			    
			    outs() << "do source generation !\n";
			    
			    // And provide it to the templating engine
			    doRequestHeaderGeneration(diagEngine,
						      generation_header_template,
						      values_map);
			  }
			    
			// If source generation was requested
			if (generate_req_sources)
			  {
			    // Build the map
			    string2_map values_map;
			    values_map["@RequestFunctionName@"] = function_name;
			    values_map["@RequestLiteralDefName@"] = requestLiteralDefName;
			    values_map["@RequestLiteralDefValue@"] = requestLiteralDefValue;
			    values_map["@RequestInterName@"] = fromReqName;
			    values_map["@RequestFormatArgsDef@"] = requestFormatArgsDef;
			    values_map["@FromRequestNameLength@"] = fromReqNameLength;
			    values_map["@FromRequestName@"] = sprintfTarget;
			    values_map["@RequestFormatArgsUsage@"] = requestFormatArgsUsage;
			    values_map["@RequestExecSql@"] = requestExecSql;

			    outs() << "do header generation !\n";
			    
			    // And provide it to the templating engine
			    doRequestSourceGeneration(diagEngine,
						      generation_source_template,
						      values_map);
			  }
		      }
		    else
		      // TODO: emit standard error. This should not occurs
		      errs() << "!!! *** ERROR: no match found for assignments\n";
		  }
		// Now, emit errors, warnings and fixes
		emitDiagAndFix(loc_start, loc_end, function_name);
	      }
	
	    else
	      // Didn't match comment at all!
	      emitError(diagEngine,
			comment_loc_start,
			ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH);
	  }
	
	else
	  {
	    if (errOccured)
	      // An error occured while accessing memory buffer for sources
	      emitError(diagEngine,
			loc_start,
			ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA);

	    else
	      // We didn't found the comment start ???
	      emitError(diagEngine,
			comment_loc_end,
			ExecSQLPrepareFmtdToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START);
	  }
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
