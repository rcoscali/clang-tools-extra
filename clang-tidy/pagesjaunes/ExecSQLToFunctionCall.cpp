//===--- ExecSQLToFunctionCall.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <fstream>

#include "ExecSQLToFunctionCall.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Regex.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {
      /**
       * ExecSQLToFunctionCall constructor
       *
       * @brief Constructor for the ExecSQLToFunctionCall rewriting check
       *
       * The rule is created a new check using its \c ClangTidyCheck base class.
       * Name and context are provided and stored locally.
       * Some diag ids corresponding to errors handled by rule are created:
       * - unexpected_diag_id: Unexpected error
       * - no_error_diag_id: No error
       * - access_char_data_diag_id: Couldn't access memory buffer for comment (unexpected)
       * - cant_find_comment_diag_id: Comment not available (unexpected)
       * - comment_dont_match_diag_id: Invalid comment structure (unexpected)
       *
       * @param Name    A StringRef for the new check name
       * @param Context The ClangTidyContext allowing to access other contexts
       */
      ExecSQLToFunctionCall::ExecSQLToFunctionCall(StringRef Name,
						   ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),	/** Init check (super class) */
	  TidyContext(Context),			/** Init our TidyContext instance */
	  /** Unexpected error occured */
	  unexpected_diag_id(Context->
			     getCustomDiagID(DiagnosticsEngine::Warning,
					     "Unexpected error occured?!")),
	  /** No error: never thrown */
	  no_error_diag_id(Context->
			   getCustomDiagID(DiagnosticsEngine::Ignored,
					   "No error")),
	  /** Access char data error occured */
	  access_char_data_diag_id(Context->
				   getCustomDiagID(DiagnosticsEngine::Error,
						   "Couldn't access character data in file cache memory buffers!")),
	  /** Cannot find comment error */
	  cant_find_comment_diag_id(Context->
				    getCustomDiagID(DiagnosticsEngine::Error,
						    "Couldn't find ProC comment start! This result has been discarded!")),
	  /** Cannot parse comment as a ProC SQL rqt statement */
	  comment_dont_match_diag_id(Context->
				     getCustomDiagID(DiagnosticsEngine::Error,
						     "Couldn't match ProC comment for function name creation!")),
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
						 "./pagesjaunes.h.tmpl")),
	  /// Generation source default template (string)
	  generation_source_template(Options.get("Generation-source-template",
						 "./pagesjaunes.pc.tmpl")),
	  /// Generation header template for prepare requests (string)
	  generation_prepare_header_template(Options.get("Generation-prepare-header-template",
							 "./pagesjaunes_prepare.h.tmpl")),
	  /// Generation source template for prepare requests (string)
	  generation_prepare_source_template(Options.get("Generation-prepare-source-template",
							 "./pagesjaunes_prepare.pc.tmpl"))
      {}
      
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
      ExecSQLToFunctionCall::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
	Options.store(Opts, "Generate-requests-headers", generate_req_headers);
	Options.store(Opts, "Generate-requests-sources", generate_req_sources);
	Options.store(Opts, "Generation-directory", generation_directory);
	Options.store(Opts, "Generation-header-template", generation_header_template);
	Options.store(Opts, "Generation-source-template", generation_source_template);
	Options.store(Opts, "Generation-prepare-header-template", generation_prepare_header_template);
	Options.store(Opts, "Generation-prepare-source-template", generation_prepare_source_template);
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
      ExecSQLToFunctionCall::registerMatchers(MatchFinder *Finder) 
      {
	/* Add a matcher for finding compound statements starting */
	/* with a sqlstm variable declaration */
        Finder->addMatcher(varDecl(hasAncestor(declStmt(hasAncestor(compoundStmt(hasAncestor(functionDecl().bind("function"))).bind("proCBlock")))),
				   hasName("sqlstm"))
			   , this);
      }
      
      /*
       * emitDiagAndFix
       *
       * @brief Emit a diagnostic message and possible replacement fix for each
       *        statement we will be notified with.
       *
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
      ExecSQLToFunctionCall::emitDiagAndFix(const SourceLocation& loc_start,
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
       * ExecSQLToFunctionCall::doRequestSourceGeneration
       *
       * @brief Generate source file for request from template
       *
       * @param[in] tmpl         Template file pathname
       * @param[in] values_map   Map containing values for replacement strings
       *
       */
      void
      ExecSQLToFunctionCall::doRequestSourceGeneration(const std::string& tmpl,
						       string2_map& values_map)
      {
	std::filebuf fbi, fbo;
	std::string fileName = values_map["@RequestFunctionName@"];
	fileName.append(".cpp");
	fileName.insert(0, "/");
	fileName.insert(0, generation_directory);
	if (fbi.open(tmpl, std::ios::in))
	  {
	    if (fbo.open(fileName, std::ios::out))
	      {
		std::istream is(&fbi);
		std::ostream os(&fbo);

		while (is)
		  {
		    char buf[255];
		    is.getline(buf, 255);
		    std::string aline = buf;
		    for (auto it = values_map.begin();
			 it != values_map.end();
			 ++it)
		      {
			size_t pos = 0;
			do
			  {
			    if ((pos = aline.find(it->first, pos)) != std::string::npos)
			      {
				aline.erase(pos, it->first.length());
				aline.insert(pos, it->second);
			      }
			  }
			while (pos != std::string::npos);
		      }
		    
		    os << aline << std::endl;
		  }
		fbo.close();
	      }
	    fbi.close();
	  }
      }
      
      /**
       *
       * Generate header file for request
       */
      void ExecSQLToFunctionCall::doRequestHeaderGeneration(const std::string& tmpl,
							    string2_map& values_map)
      {	
	std::filebuf fbi, fbo;
	std::string fileName = values_map["@RequestFunctionName@"];
	fileName.append(".h");
	fileName.insert(0, "/");
	fileName.insert(0, generation_directory);
	if (fbi.open(tmpl, std::ios::in))
	  {
	    if (fbo.open(fileName, std::ios::out))
	      {
		std::istream is(&fbi);
		std::ostream os(&fbo);

		while (is)
		  {
		    char buf[255];
		    is.getline(buf, 255);
		    std::string aline = buf;
		    for (auto it = values_map.begin();
			 it != values_map.end();
			 ++it)
		      {
			size_t pos = 0;
			do
			  {
			    if ((pos = aline.find(it->first, pos)) != std::string::npos)
			      {
				aline.erase(pos, it->first.length());
				aline.insert(pos, it->second);
			      }
			  }
			while (pos != std::string::npos);

		      }
		    os << aline << std::endl;			 
		  }
		fbo.close();
	      }
	    fbi.close();
	  }
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
      ExecSQLToFunctionCall::emitError(DiagnosticsEngine &diag_engine,
				       const SourceLocation& err_loc,
				       enum ExecSQLToFunctionCallErrorKind kind)
      {
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
	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_NO_ERROR:
	    diag_engine.Report(err_loc, no_error_diag_id);
	    break;

	    /** Access char data diag ID */
	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA:
	    diag_engine.Report(err_loc, access_char_data_diag_id);
	    break;

	    /** Can't find a comment */
	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START:
	    diag_engine.Report(err_loc, cant_find_comment_diag_id);
	    break;

	    /** Cannot match comment */
	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH:
	    diag_engine.Report(err_loc, comment_dont_match_diag_id);
	    break;
	  }
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
      ExecSQLToFunctionCall::check(const MatchFinder::MatchResult &result) 
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

	outs() << "Found one result at line " << startLineNum << "\n";
	outs() << "Generate_req_headers: " << (generate_req_headers ? "true" : "false") << "\n";
	outs() << "Generate_req_sources: " << (generate_req_sources ? "true" : "false") << "\n";
	outs() << "Generation_directory: '" << generation_directory << "'\n";
	outs() << "Generation_header_template: '" << generation_header_template << "'\n";
	outs() << "Generation_source_template: '" << generation_source_template << "'\n";

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
		size_t commentStart = lineData.find("/*");
		
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
	while (true);

	/*
	 * Comment found
	 */
	std::string comment;
	std::string function_name;

	// If found comment start & no error occured
	if (!errOccured)
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

	    outs() << "comment for block at line #" << startLineNum << ": '" << comment << "'\n";

	    /*
	     * Create function call for the request
	     */

	    // Regex for processing comment for all remaining requests
	    Regex reqRe("^.*EXEC SQL[ \t]+([A-Za-z]+)[ \t]+([A-Za-z0-9]+).*;.*$");
	    
	    // Regex for processing PREPARE comment
	    Regex reqRePrep("^.*EXEC SQL[ \t]+(prepare|PREPARE)[ \t]+([A-Za-z0-9]+)[ \t]+from[ \t]+:([A-Za-z]+);.*$");
	    
	    // Returned matches
	    SmallVector<StringRef, 8> matches;

	    // If comment match
	    if (reqRePrep.match(comment, &matches))
	      {
		outs() << "!!!*** Prep comment match: from request name is '" << matches[3] << "' and request name is '"<< matches[2] << "'\n";

		/*
		 * Find the request literal
		 */
		
		// The request name used in ProC (syntax :reqName)
		std::string fromReqName = matches[3];
		std::string reqName = matches[2];
		
		// Statement matcher for the request string literal
                DeclarationMatcher m_matcher
		  = functionDecl(hasDescendant(
                        callExpr(hasDescendant(declRefExpr(hasDeclaration(namedDecl(hasName("sprintf"))))),
                            hasArgument(0,
                                declRefExpr(hasDeclaration(namedDecl(hasName(fromReqName.c_str()))))),
                            hasArgument(1,
                                stringLiteral().bind("reqLiteral"))).bind("callExpr")),
                        hasName(curFunc->getNameAsString()));

		// Prepare matcher finder for our customized matcher
		CopyRequestMatcher crMatcher(this);
		MatchFinder finder;
		finder.addMatcher(m_matcher, &crMatcher);
		// Clear collector vector
		m_req_copy_collector.clear();
		// Run the visitor pattern for collecting matches
		tool->run(newFrontendActionFactory(&finder).get());
		// The collected records
		struct StringLiteralRecord *lastRecord = (struct StringLiteralRecord *)NULL;
		// Let's iterate over all collected matches
		for (auto it = m_req_copy_collector.begin();
		     it != m_req_copy_collector.end();
		     ++it)
		  {
		    // And find the one just before the ProC statement
		    struct StringLiteralRecord *record = (*it);
		    if (record->call_linenum > startLineNum)
		      break;
		    // Didn't find yet
		    lastRecord = record;
		  }

		std::string requestDefineValue;

		// Last record collected just before the ProC statement
		if (lastRecord)
		  {
		    outs() << "Found literal at line #" << lastRecord->linenum << ": '" << lastRecord->literal->getString() << "'\n";
		    outs() << "     for a call at line #" << lastRecord->call_linenum << "'\n";
		    requestDefineValue.assign(lastRecord->literal->getString());
		  }

		// Compute function name
		function_name.assign("prepare");
		// Get match in rest string
		std::string rest(reqName);
		// Capitalize it
		rest[0] &= ~0x20;
		// And append to function name
		function_name.append(rest);

		// 'EXEC SQL' statement
		std::string requestExecSql = "prepare ";
		requestExecSql.append(reqName);
		requestExecSql.append(" from :");
		requestExecSql.append(fromReqName);

		if (generate_req_headers)
		  {
		    string2_map values_map;
		    values_map["@RequestFunctionName@"] = function_name;
		    doRequestHeaderGeneration(generation_prepare_header_template,
					      values_map);
		  }

		if (generate_req_sources)
		  {
		    string2_map values_map;
		    values_map["@FromRequestName@"] = fromReqName;
		    values_map["@RequestDefineName@"] = reqName;
		    values_map["@RequestDefineValue@"] = requestDefineValue;
		    values_map["@RequestFunctionName@"] = function_name;
		    values_map["@RequestExecSql@"] = requestExecSql;
		    doRequestSourceGeneration(generation_prepare_source_template,
					      values_map);
		  }

		// Emit errors, warnings and fixes
		emitDiagAndFix(loc_start, loc_end, function_name);
	      }
	    
	    // If comment match
	    else if (reqRe.match(comment, &matches))
	      {
		// Start at first match ($0 is the whole match)
		auto it = matches.begin();
		// Skip it
		it++;
		//outs() << "match #1  = '" << (*it).str() << "'\n";
		// Append first match (action) to function name
		function_name.append((*it).lower());
		// Get next match
		it++;
		//outs() << "match #2  = '" << (*it).str() << "'\n";
		// Get match in rest string
		std::string rest((*it).str());
		// Capitalize it
		rest[0] &= ~0x20;
		// And append to function name
		function_name.append(rest);

		// Got it, emit changes
		//outs() << "** Function name = " << function_name << " for proC block at line # " << startLineNum << "\n";

		if (generate_req_headers)
		  {
		    string2_map values_map;
		    values_map["@RequestFunctionName@"] = function_name;
		    doRequestHeaderGeneration(generation_header_template,
					      values_map);
		  }

		if (generate_req_sources)
		  {
		    string2_map values_map;
		    values_map["@RequestFunctionName@"] = function_name;
		    doRequestSourceGeneration(generation_source_template,
					      values_map);
		  }

		// Emit errors, warnings and fixes
		emitDiagAndFix(loc_start, loc_end, function_name);
	      }
	    else
	      emitError(diagEngine,
			comment_loc_start,
			ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH);
	  }
	else
	  {
	    if (errOccured)
	      emitError(diagEngine,
			loc_start,
			ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA);

	    else
	      emitError(diagEngine,
			comment_loc_end,
			ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START);
	  }
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
