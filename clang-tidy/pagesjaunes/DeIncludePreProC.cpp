//===--- DeIncludePreProC.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "DeIncludePreProC.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RawCommentList.h"
#include "clang/Basic/LLVM.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CompilationDatabase.h"
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
       * DeIncludePreProC constructor
       *
       * @brief Constructor for the \c DeIncludePreProC rewriting check
       *
       * The rule is created a new check using its \c ClangTidyCheck base class.
       * Name and context are provided and stored locally.
       * Some diag ids corresponding to errors handled by rule are created:
       *   - \c unexpected_diag_id: Unexpected error
       *   - \c no_error_diag_id: No error
       *   - \c access_char_data_diag_id: Couldn't access memory buffer for comment (unexpected)
       *   - \c cant_find_comment_diag_id: Comment not available (unexpected)
       *   - \c comment_dont_match_diag_id: Invalid comment structure (unexpected)
       * This check allows to customize several values through options. These options
       * are:
       *   - \c Comment-regex: Allows to customize the value of the regular expression used
       *     for finding comments created from the EXEC SQL include statements
       *   - \c Headers-to-include-in: Allows to customize the headers IN filter. Headers that
       *     are not in this filter are not included and then not processed. 
       *   - \c Headers-to-exclude-from: Allows to customize the headers OUT filter. Headers
       *     that are in this filter are not included and then not processed. 
       *   - \c Headers-directories: Allows to customize the headers directories that will be
       *     searched. 
       *     TODO: Try to find a way to use compilation database to avoid this option.
       *
       *
       * @param Name    A StringRef for the new check name
       * @param Context The ClangTidyContext allowing to access other contexts
       */
      DeIncludePreProC::DeIncludePreProC(StringRef Name,
						   ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),	/** Init check (super class) */
	  TidyContext(Context),			/** Init our TidyContext instance */
	  /** Unexpected error occured */
	  unexpected_diag_id(Context->
			     getCustomDiagID(DiagnosticsEngine::Warning,
					     "Unexpected error occured?!")),
	  /** No error diag id: never thrown */
	  no_error_diag_id(Context->
			   getCustomDiagID(DiagnosticsEngine::Ignored,
					   "No error")),
	  /** Access error diag id: Access char data error occured */
	  access_char_data_diag_id(Context->
				   getCustomDiagID(DiagnosticsEngine::Error,
						   "Couldn't access character data in file cache memory buffers!")),
	  /** Comment parse diag id: Cannot find comment error */
	  cant_find_comment_diag_id(Context->
				    getCustomDiagID(DiagnosticsEngine::Error,
						    "Couldn't find ProC comment start! This result has been discarded!")),
	  /** Diag ID for parse comment error: Cannot parse it as a ProC SQL rqt statement */
	  comment_dont_match_diag_id(Context->
				     getCustomDiagID(DiagnosticsEngine::Error,
						     "Couldn't match ProC comment for function name creation!")),
	  /** Check option for comment regex */
	  comment_regex(Options.get("Comment-regex", "^.*EXEC SQL[ \t]+include[ \t]+\"?([-0-9A-Za-z._]*)\"?.*$")),
	  /** Check option for setting a restriction list of headers to process */
	  headers_to_include_in(Options.get("Headers-to-include-in", "")),
	  /** Check option for setting a header exclusion list */
          headers_to_exclude_from(Options.get("Headers-to-exclude-from", "oraca,sqlca")),
	  /** Check option for setting a list include directories */
	  headers_directories(Options.get("Headers-directories", ""))
      {
	// Init toIncludein value from the tokenization of the
	// headers_to_include_in option value
	if (!headers_to_include_in.empty())
	  toIncludeIn = tokenizeString(headers_to_include_in, ",;:");
	
	// Init toExcludeFrom value from the tokenization of the
	// headers_to_exclude_from option value
	if (!headers_to_exclude_from.empty())
	  toExcludeFrom = tokenizeString(headers_to_exclude_from, ",;:");

	// Init headersDirectories value from the tokenization of the
	// headers_directories option value
	if (!headers_directories.empty())
	  headersDirectories = tokenizeString(headers_directories, ",;:");
      }
      
      /**
       * storeOptions
       *
       * @brief Store options for this check
       *
       * This check support one option for customizing comment regex 
       * - Comment-regex
       *
       * @param Opts	The option map in which to store supported options
       */
      void
      DeIncludePreProC::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
	Options.store(Opts, "Comment-regex", comment_regex);
	Options.store(Opts, "Headers-to-include-in", headers_to_include_in);
	Options.store(Opts, "Headers-to-exclude-from", headers_to_exclude_from);
	Options.store(Opts, "Headers-directories", headers_directories);
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
      DeIncludePreProC::registerMatchers(MatchFinder *Finder) 
      {
	/* Add a matcher for finding the translation unit */
	/* This is just a trick to be called once per file */
	/* The translation unit is also available through ASTContext */
	/* Rest of processing is based on a custom navigation through comments */
        Finder->addMatcher(translationUnitDecl().bind("translation_unit")
			   , this);
      }
      
      /**
       * emitDiagAndFix
       *
       * @brief Emit a diagnostic message and possible replacement fix for each
       *        statement we will be notified with.
       *
       * This method is called each time a statement to handle (rewrite) is found.
       * One replacement will be emited for each node found.
       * It is passed all necessary arguments for:
       * - creating a comprehensive diagnostic message
       * - computing the locations of code we will replace
       * - computing the new code that will replace old one
       *
       * @param loc_start       The CompoundStmt start location
       * @param loc_end         The CompoundStmt end location
       * @param hdr_filename    The function name that will be called
       */
      void
      DeIncludePreProC::emitDiagAndFix(const SourceLocation& loc_start,
				       const SourceLocation& loc_end,
				       const std::string& hdr_filename)
      {
	/* Range of the statement to change */
	SourceRange stmt_range(loc_start, loc_end);

	/* Diagnostic builder for a found AST node */
	/* Default is a warning, and it is emitted */
	/* as soon as the diag builder is destroyed */
	DiagnosticBuilder mydiag = diag(loc_end,
					"Header file '%0' replaced by a "
					" standard include")
	  << hdr_filename;

	/* Replacement code built */
	std::string replt_code = "#include \"";
	replt_code.append(hdr_filename);
	replt_code.append("\"");

	/* Emit the replacement over the found statement range */
	mydiag << FixItHint::CreateReplacement(stmt_range, replt_code);
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
      DeIncludePreProC::emitError(DiagnosticsEngine &diag_engine,
				       const SourceLocation& err_loc,
				       enum DeIncludePreProCErrorKind kind)
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
	  case DeIncludePreProC::DE_INCLUDE_PRE_PROC_ERROR_NO_ERROR:
	    diag_engine.Report(err_loc, no_error_diag_id);
	    break;

	    /** Access char data diag ID */
	  case DeIncludePreProC::DE_INCLUDE_PRE_PROC_ERROR_ACCESS_CHAR_DATA:
	    diag_engine.Report(err_loc, access_char_data_diag_id);
	    break;

	    /** Can't find a comment */
	  case DeIncludePreProC::DE_INCLUDE_PRE_PROC_ERROR_CANT_FIND_COMMENT_START:
	    diag_engine.Report(err_loc, cant_find_comment_diag_id);
	    break;

	    /** Cannot match comment */
	  case DeIncludePreProC::DE_INCLUDE_PRE_PROC_ERROR_COMMENT_DONT_MATCH:
	    diag_engine.Report(err_loc, comment_dont_match_diag_id);
	    break;
	  }
      }

      /**
       * count_buffer_chars_number
       *
       * @brief 		Count characters from char_to_count in the buffer \c buf
       *
       * This method allows to count chars in a buffer. It allows to determine, for 
       * ex., the exact number of lines for computing the exact position of a header
       * in a source (a header could contain some UTF-8 chars that forbid to calculate
       * the size of a string from a number of bytes, as a char is variable multibyte)
       *
       * @param buf		The buffer in which to count characters in
       * @param char_to_count	The string containing the characters to be counted
       *
       * @return		Number of characters counted from the provided buffer
       */
      unsigned int
      DeIncludePreProC::count_buffer_chars_number (const std::string& buf,
						   const char* char_to_count)
      {
	// Number of found characters
	std::string::size_type chars = 0;
	// Position of the found char
	std::string::size_type pos = -1;
	// Iterate over the buffer
	do
	  {
	    // Find a character from next position of previous found char
	    pos = buf.find(char_to_count, pos+1);
	    // If char was found, ...
	    if (pos != std::string::npos)
	      // .. increment the result
	      chars++;
	    // or, ...
	    else
	      // ... break the loop
	      break;
	  }
	// Iterate until no more char to find. If char is not found
	// tsd::string returns std::string::npos, that is actually -1
	while (pos > 0);

	// Return the result
	return chars;
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
       * The comment depends on which pre-processoris used. Default value is set
       * for processing comments created through Oracle ProC.
       * Customizing the regex is available through \c Comment-regex option.
       * An example for setting this option is:
       * {
       *  key: 
       *    pagesjaunes-de-include-preproc.Comment-regex, 
       *  value: 
       *    '^.*EXEC SQL[ \t]+include[ \t]+\"([_A-Za-z.]+)\".*$'
       * }
       * 
       *
       * @param Result          The match result provided by the recursive visitor
       *                        allowing us to access AST nodes bound to variables
       */
      void
      DeIncludePreProC::check(const MatchFinder::MatchResult &result) 
      {
	// Comment regular expression for a processed header
	Regex comment_re(comment_regex);
	// Translation unit found by matcher
	const TranslationUnitDecl *translation_unit =
	  result.Nodes.getNodeAs<TranslationUnitDecl>("translation_unit");

	// Found a translation unit 
	if (translation_unit)
	  {
	    // AST context
	    ASTContext &ast_ctxt = translation_unit->getASTContext();
	    // Raw comments list
	    RawCommentList &raw_comments = ast_ctxt.getRawCommentList();
	    // Get an array of the comments
	    ArrayRef<RawComment *> raw_comments_array = raw_comments.getComments();

	    // Filter comments for finding all includes
	    for (auto cit = raw_comments_array.begin();
		 cit != raw_comments_array.end();
		 ++cit)
	      {
		// Matches result
		SmallVector<StringRef, 8> matches;
		// Raw text for the comment
		std::string raw_text = (*cit)->getRawText(ast_ctxt.getSourceManager()).str();
		// Raw comment
		RawComment *raw_comment = (*cit);
		// Location of the comment start
		SourceLocation comment_start_loc = raw_comment->getLocStart();

		// If comment matches
		if (comment_re.match(raw_text, &matches))
		  {
		    // Header file name
		    std::string header_name = matches[1].str();

		    if ((toIncludeIn.empty() ||
			 contain(toIncludeIn, header_name)) &&
			(toExcludeFrom.empty() ||
			 !contain(toExcludeFrom, header_name)))
		      {
			std::string raw_txt = raw_comment->getRawText(ast_ctxt.getSourceManager()).str();
			raw_text.erase(raw_text.find("*/")+2,std::string::npos);
			unsigned raw_text_len = raw_text.length();

			SourceManager &src_mgr = ast_ctxt.getSourceManager();
			FileManager &file_mgr = src_mgr.getFileManager();
			FileID fid = src_mgr.getMainFileID();

			const FileEntry *hdr_fentry = nullptr;
			for (auto it = headersDirectories.begin();
			     it != headersDirectories.end();
			     ++it)
			  {
			    std::string hdr_dir = (*it).append("/");
			    hdr_fentry = file_mgr.getFile(hdr_dir.append(header_name), true);
			    if (hdr_fentry)
			      break;
			  }
			// Found hdr file included here
			if (hdr_fentry)
			  {
			    auto buf = src_mgr.getMemoryBufferForFile(hdr_fentry);
			    const char *pbuf = buf->getBufferStart();
			    std::string pbufstr = std::string(pbuf);

			    unsigned int hdr_lines = count_buffer_chars_number(pbufstr, "\n");
			    // Get location of start of included hdr file
			    unsigned comment_start_file_offset = src_mgr.getFileOffset(comment_start_loc);
			    unsigned comment_end_file_offset = comment_start_file_offset + raw_text_len -1;
			    unsigned comment_end_line_number = src_mgr.getLineNumber(fid, comment_end_file_offset+1);
			    unsigned hdr_start_line_number = comment_end_line_number +1;
			    unsigned hdr_end_line_number = hdr_start_line_number + hdr_lines;
			    // outs() << "Hdr start line number = " << hdr_start_line_number << "\n";
			    // outs() << "Hdr end line number = " << hdr_end_line_number << "\n";
			    SourceLocation hdr_start = src_mgr.translateLineCol(fid, hdr_start_line_number, 1);
			    SourceLocation hdr_end = src_mgr.translateLineCol(fid, hdr_end_line_number, 1);

			    emitDiagAndFix(hdr_start, hdr_end,
					   header_name);
			  }
		      }
		  }
		    
	      }
	  }
      }

      /**
       * 
       */
      bool
      DeIncludePreProC::contain(std::vector<std::string>& set,
				std::string& str)
      {
	for (auto it = set.begin();
	     it != set.end();
	     ++it)
	  {
	    if (str.compare((*it)) == 0)
	      return true;
	  }
	return false;
      }

      /**
       *
       */
      std::vector<std::string>
      DeIncludePreProC::tokenizeString(const std::string& str,
				       const std::string& delims)
	{
	  std::vector<std::string> tokens;

	  std::string::size_type lastPos = str.find_first_not_of(delims, 0);
	  std::string::size_type pos = str.find_first_of(delims, lastPos);

	  while (pos != std::string::npos ||
		 lastPos != std::string::npos)
	    {
	      tokens.push_back(str.substr(lastPos, pos - lastPos));
	      lastPos = str.find_first_not_of(delims, pos);
	      pos = str.find_first_of(delims, lastPos);
	    }
	  return tokens;
	}
      
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
