//===--- ExecSQLToFunctionCall.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

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
      ExecSQLToFunctionCall::ExecSQLToFunctionCall(StringRef Name,
						   ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),
	  TidyContext(Context),
	  unexpected_diag_id(Context->
			     getCustomDiagID(DiagnosticsEngine::Warning,
					     "Unexpected error occured?!")),
	  no_error_diag_id(Context->
			   getCustomDiagID(DiagnosticsEngine::Ignored,
					   "No error")),
	  access_char_data_diag_id(Context->
				   getCustomDiagID(DiagnosticsEngine::Error,
						   "Couldn't access character data in file cache memory buffers!")),
	  cant_find_comment_diag_id(Context->
				    getCustomDiagID(DiagnosticsEngine::Error,
						    "Couldn't find ProC comment start! This result has been discarded!")),
	  comment_dont_match_diag_id(Context->
				     getCustomDiagID(DiagnosticsEngine::Error,
						     "Couldn't match ProC comment for function name creation!"))
      {}
      
      void 
      ExecSQLToFunctionCall::registerMatchers(MatchFinder *Finder) 
      {
        Finder->addMatcher(
			   varDecl(
				   hasAncestor(declStmt(hasAncestor(compoundStmt().bind("proCBlock")))),
				   hasName("sqlstm")
				   )
			   , this);
      }
      
      void
      ExecSQLToFunctionCall::emitDiagAndFix(const SourceLocation& loc_start,
					    const SourceLocation& loc_end,
					    const std::string& function_name)
      {
	SourceRange stmt_range(loc_start, loc_end);
	DiagnosticBuilder mydiag = diag(loc_end,
					"ProC Statement Block shall be replaced by a function call named '%0'") << function_name;
	std::string replt_code("	");
	replt_code.append(function_name);
	replt_code.append("();");
	mydiag << FixItHint::CreateReplacement(stmt_range, replt_code);
      }
      
      void
      ExecSQLToFunctionCall::emitError(DiagnosticsEngine &diag_engine,
				       const SourceLocation& err_loc,
				       enum ExecSQLToFunctionCallErrorKind kind)
      {
	switch (kind)
	  {
	  default:
	    diag_engine.Report(err_loc, unexpected_diag_id);
	    break;

	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_NO_ERROR:
	    diag_engine.Report(err_loc, no_error_diag_id);
	    break;

	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA:
	    diag_engine.Report(err_loc, access_char_data_diag_id);
	    break;

	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START:
	    diag_engine.Report(err_loc, cant_find_comment_diag_id);
	    break;

	  case ExecSQLToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH:
	    diag_engine.Report(err_loc, comment_dont_match_diag_id);
	    break;
	  }
      }
      
      void
      ExecSQLToFunctionCall::check(const MatchFinder::MatchResult &Result) 
      {
	// Get the compound statement AST node as the bounded var 'proCBlock'
	const CompoundStmt *stmt = Result.Nodes.getNodeAs<CompoundStmt>("proCBlock");
	// Get the source manager
	SourceManager &srcMgr = Result.Context->getSourceManager();
	DiagnosticsEngine &diagEngine = Result.Context->getDiagnostics();

	// Get start/end locations for the statement
	SourceLocation loc_start = stmt->getLocStart();
	SourceLocation loc_end = stmt->getLocEnd();

	// Get file ID in src mgr
	FileID startFid = srcMgr.getFileID(loc_start);
	// Get compound statement start line num
	unsigned startLineNum = srcMgr.getLineNumber(startFid, srcMgr.getFileOffset(loc_start));

	//outs() << "found one result at line " << startLineNum << "\n";

	/*
	 * iteration from line -2 to line -5 until finding the whole EXEC SQL comment
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

	// Finding loop
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
	     * Remove all \n
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

	    //outs() << "comment for block at line #" << startLineNum << ": " << comment << "\n";
	    
	    /*
	     * Create function call for the request
	     */

	    // Regex for processing comment
	    Regex reqRe("^.*EXEC SQL[ \t]+([A-Za-z]+)[ \t]+([A-Za-z]+).*;.*$");
	    // Returned matches
	    SmallVector<StringRef, 8> matches;

	    // If comment match
	    if (reqRe.match(comment, &matches))
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
