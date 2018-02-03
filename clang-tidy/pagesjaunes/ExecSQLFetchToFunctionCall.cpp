//===--- ExecSQLFetchToFunctionCall.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <set>
#include <vector>
#include <map>
#include <sstream>
#include <string>

#include "ExecSQLFetchToFunctionCall.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Regex.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace std::chrono;
using emplace_ret_t = std::pair<std::set<std::string>::iterator, bool>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      namespace
      {
	// Keep track of macro expansions that defines a StringLiteral
	// then keep the associated source_range_set_t to be able to get the
	// original define (for keeping indentation)
	class GetStringLiteralsDefines : public PPCallbacks
	{
	public:
	  explicit GetStringLiteralsDefines(ExecSQLFetchToFunctionCall *parent,
					    ExecSQLFetchToFunctionCall::source_range_set_t *srset)
	    : m_parent(parent),
	      m_src_mgr(m_parent->TidyContext->getASTContext()->getSourceManager()),
	      m_diag_engine(m_parent->TidyContext->getASTContext()->getDiagnostics()),
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
			SourceLocation sloc = t.getLocation();
			SourceLocation eloc = t.getEndLoc();
			sr = SourceRange(sloc, eloc);
			have_literal = true;
		      }
		    else if (t.is(tok::wide_string_literal) ||
			     t.is(tok::angle_string_literal) ||
			     t.is(tok::utf8_string_literal) ||
			     t.is(tok::utf16_string_literal) ||
			     t.is(tok::utf32_string_literal))
		      {
			// TODO: handle that kind of string charsets
			std::string slKind;
			if (t.is(tok::wide_string_literal))
			  slKind = "Wide String";
			else if (t.is(tok::angle_string_literal))
			  slKind = "Angle String";
			else if (t.is(tok::utf8_string_literal))
			  slKind = "UTF8 String";
			else if (t.is(tok::utf16_string_literal))
			  slKind = "UTF16 String";
			else if (t.is(tok::utf32_string_literal))
			  slKind = "UTF32 String";

			//errs() << "*** Token for weird string (" << slKind << ") found\n";
			m_parent->emitError(m_diag_engine,
					    t.getLocation(),
					    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_UNSUPPORTED_STRING_CHARSET,
					    &slKind);
		      }
		  }
		
		if (have_literal)
		  {
		    //outs() << "Adding macro '" << macroName << "' expansion at " << range.getBegin().printToString(m_src_mgr) << "...'\n";
		    ExecSQLFetchToFunctionCall::SourceRangeForStringLiterals *ent
		      = new ExecSQLFetchToFunctionCall::SourceRangeForStringLiterals(range, sr, macroName);
		    m_macrosStringLiteralsPtr->insert(ent);
		  }
	      }
	  }

	  void
	  EndOfMainFile() override
	  {
	    //outs() << "!!!***> END OF MAIN FILE !\n";
	  }
	  
	private:
	  ExecSQLFetchToFunctionCall *m_parent;
	  const SourceManager &m_src_mgr; 
	  DiagnosticsEngine &m_diag_engine;
	  ExecSQLFetchToFunctionCall::source_range_set_t *m_macrosStringLiteralsPtr;
	};
      } // namespace
      
      /**
       * ExecSQLFetchToFunctionCall constructor
       *
       * @brief Constructor for the ExecSQLFetchToFunctionCall rewriting check
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
      ExecSQLFetchToFunctionCall::ExecSQLFetchToFunctionCall(StringRef Name,
							     ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),	/** Init check (super class) */
	  TidyContext(Context),			/** Init our TidyContext instance */

	  /*
	   * Options
	   */
	  /// Generate requests header files (bool)
	  generate_req_headers(Options.get("Generate-requests-headers",
					   0U)),
	  /// Generate requests source files (bool)
	  generate_req_sources(Options.get("Generate-requests-sources",
					   0U)),
	  /// Generate requests source files (bool)
	  generate_req_allow_overwrite(Options.get("Generate-requests-allow-overwrite",
                                                   1U)),
	  /// Generation directory (string)
	  generation_directory(Options.get("Generation-directory",
					   "./")),
	  /// Generation header default template (string)
	  generation_header_template(Options.get("Generation-header-template",
						 "./pagesjaunes_fetch.h.tmpl")),
	  /// Generation source default template (string)
	  generation_source_template(Options.get("Generation-source-template",
						 "./pagesjaunes_fetch.pc.tmpl")),
	  /// Request grouping option: Filename containing a json map for
	  /// a group name indexing a vector of requests name
	  generation_request_groups(Options.get("Generation-request-groups",
						"./request_groups.json")),
	  /// Simplify request args list if possible
	  generation_simplify_function_args(Options.get("Generation-simplify-function-args",
							0U)),
          /// Conditionnaly report modification in .pc file if this is true
          generation_do_report_modification_in_pc(Options.get("Generation-do-report-modification-in-PC",
                                                              1U)),
          /// Directory of the original .pc file in which to report modification
          generation_report_modification_in_dir(Options.get("Generation-report-modification-in-dir",
                                                            "./")),
          /// Keep EXEC SQL comments
          generation_do_keep_commented_out_exec_sql(Options.get("Generation-keep-commented-out-exec-sql-in-PC",
                                                                0U))
      {
        llvm::outs() << "ExecSQLFetchToFunctionCall::ExecSQLFetchToFunctionCall(StringRef Name, ClangTidyContext *Context):\n";
        llvm::outs() << "    generate_req_headers = " << (generate_req_headers?"True":"False") << "\n";
        llvm::outs() << "    generate_req_sources = " << (generate_req_sources?"True":"False") << "\n";
        llvm::outs() << "    generate_req_allow_overwrite = " << (generate_req_allow_overwrite?"True":"False") << "\n";
        llvm::outs() << "    generation_directory = '" << generation_directory << "'\n";
        llvm::outs() << "    generation_header_template = '" << generation_header_template << "'\n";
        llvm::outs() << "    generation_source_template = '" << generation_source_template << "'\n";
        llvm::outs() << "    generation_request_groups = '" << generation_request_groups << "'\n";
        llvm::outs() << "    generation_simplify_function_args = " << (generation_simplify_function_args?"True":"False") << "\n";
        llvm::outs() << "    generation_do_report_modification_in_pc = " << (generation_do_report_modification_in_pc?"True":"False") << "\n";
        llvm::outs() << "    generation_do_keep_commented_out_exec_sql = " << (generation_do_keep_commented_out_exec_sql?"True":"False") << "\n";
        llvm::outs() << "    generation_report_modification_in_dir = '" << generation_report_modification_in_dir << "'\n";

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
		std::vector<std::string> agroup = it->second.get<std::vector<std::string>>();
		auto status = req_groups.emplace(it->first, agroup);
		if (!status.second)
		  errs() << "ERROR!! Couldn't add group '" << it->first
			 << "': a group with same name already exists  in file '"
			 << generation_request_groups << "' !!\n";
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
	    // emitError(Context->getASTContext()->getDiagnostics(),
	    //           SourceLocation(),
	    //           ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_INVALID_GROUPS_FILE,
	    //           &generation_request_groups);
	  }
      }
      
      /**
       * onStartOfTranslationUnit
       *
       * @brief called at start of processing of translation unit
       *
       * Override to be called at start of translation unit
       */
      void
      ExecSQLFetchToFunctionCall::onStartOfTranslationUnit()
      {
        clang::tidy::pagesjaunes::onStartOfTranslationUnit(replacement_per_comment);
      }
      
      /**
       * onEndOfTranslationUnit
       *
       * @brief called at end of processing of translation unit
       *
       * Override to be called at end of translation unit
       */
      void
      ExecSQLFetchToFunctionCall::onEndOfTranslationUnit()
      {
        if (generation_do_report_modification_in_pc)
          clang::tidy::pagesjaunes::onEndOfTranslationUnit(replacement_per_comment,
                                                           generation_report_modification_in_dir,
                                                           generation_do_keep_commented_out_exec_sql);
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
       * - Generation-request-groups
       * - Generation-simplify-function-args
       *
       * @param Opts	The option map in which to store supported options
       */
      void
      ExecSQLFetchToFunctionCall::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
	Options.store(Opts, "Generate-requests-headers", generate_req_headers);
	Options.store(Opts, "Generate-requests-sources", generate_req_sources);
	Options.store(Opts, "Generate-requests-allow-overwrite", generate_req_allow_overwrite);
	Options.store(Opts, "Generation-directory", generation_directory);
	Options.store(Opts, "Generation-header-template", generation_header_template);
	Options.store(Opts, "Generation-source-template", generation_source_template);
	Options.store(Opts, "Generation-request-groups", generation_request_groups);
	Options.store(Opts, "Generation-simplify-function-args", generation_simplify_function_args);
        Options.store(Opts, "Generation-do-report-modification-in-PC", generation_do_report_modification_in_pc);
        Options.store(Opts, "Generation-report-modification-in-dir", generation_report_modification_in_dir);  
        Options.store(Opts, "Generation-keep-commented-out-exec-sql-in-PC", generation_do_keep_commented_out_exec_sql);  

        llvm::outs() << "ExecSQLFetchToFunctionCall::storeOptions(ClangTidyOptions::OptionMap &Opts):\n";
        llvm::outs() << "    generate_req_headers = " << (generate_req_headers?"True":"False") << "\n";
        llvm::outs() << "    generate_req_sources = " << (generate_req_sources?"True":"False") << "\n";
        llvm::outs() << "    generate_req_allow_overwrite = " << (generate_req_allow_overwrite?"True":"False") << "\n";
        llvm::outs() << "    generation_directory = '" << generation_directory << "'\n";
        llvm::outs() << "    generation_header_template = '" << generation_header_template << "'\n";
        llvm::outs() << "    generation_source_template = '" << generation_source_template << "'\n";
        llvm::outs() << "    generation_request_groups = '" << generation_request_groups << "'\n";
        llvm::outs() << "    generation_simplify_function_args = " << (generation_simplify_function_args?"True":"False") << "\n";
        llvm::outs() << "    generation_do_report_modification_in_pc = " << (generation_do_report_modification_in_pc?"True":"False") << "\n";
        llvm::outs() << "    generation_do_keep_commented_out_exec_sql = " << (generation_do_keep_commented_out_exec_sql?"True":"False") << "\n";
        llvm::outs() << "    generation_report_modification_in_dir = '" << generation_report_modification_in_dir << "'\n";
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
      ExecSQLFetchToFunctionCall::registerMatchers(MatchFinder *Finder) 
      {
        if (!getLangOpts().CPlusPlus)
          return;
        
	/* Add a matcher for finding compound statements starting */
	/* with a sqlstm variable declaration */
        Finder->addMatcher(varDecl(
		      hasAncestor(declStmt(hasAncestor(compoundStmt(hasAncestor(functionDecl().bind("function"))).bind("proCBlock")))),
		      hasName("sqlstm"))
			   , this);
      }

      /**
       * ExecSQLFetchToFunctionCall::registerPPCallbacks
       *
       * @brief Register callback for intercepting all pre-processor actions
       *
       * Allows to register a callback for executing our actions at every C/C++ 
       * pre-processor processing. Thanks to this callback we will collect all string
       * literal macro expansions.
       *
       * @param[in] compiler	the compiler instance we will intercept
       */
      void ExecSQLFetchToFunctionCall::registerPPCallbacks(CompilerInstance &compiler) {
	compiler
	  .getPreprocessor()
	  .addPPCallbacks(llvm::make_unique<GetStringLiteralsDefines>(this,
								      &m_macrosStringLiterals));
      }
      
      
      /*
       * ExecSQLFetchToFunctionCall::emitDiagAndFix
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
      std::string
      ExecSQLFetchToFunctionCall::emitDiagAndFix(const SourceLocation& loc_start,
                                                 const SourceLocation& loc_end,
                                                 const std::string& function_name,
                                                 const std::string& function_args)
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
	replt_code.append(std::string("("));
	replt_code.append(function_args);
	replt_code.append(std::string(");"));

	/* Emit the replacement over the found statement range */
	mydiag << FixItHint::CreateReplacement(stmt_range, replt_code);

        return replt_code;
      }

      /**
       * ExecSQLFetchToFunctionCall::processTemplate
       *
       * @brief Process a template file with values from the 
       *        provided map
       *
       * @param[in] tmpl	 Template file pathname
       * @param[in] fname	 Output file pathname
       * @param[in] values_map	 Map containing values to be replaced
       *
       * @retval true 	if template was processed and file was created
       * @retval false 	if something wrong occurs
       */
      bool
      ExecSQLFetchToFunctionCall::processTemplate(const std::string& tmpl,
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
       * ExecSQLFetchToFunctionCall::doRequestSourceGeneration
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
      ExecSQLFetchToFunctionCall::doRequestSourceGeneration(DiagnosticsEngine& diag_engine,
                                                            const std::string& tmpl,
                                                            string2_map& values_map)
      {
	SourceLocation dummy;
        struct stat buffer;   

	// Compute output file pathname
        std::string dirName = generation_directory;
	std::string fileBasename = values_map["@OriginalSourceFileBasename@"];
	std::string fileName = values_map["@RequestFunctionName@"];

        // Replace %B with original file basename
        size_t percent_pos = std::string::npos;
        if ((percent_pos = dirName.find("%B")) != std::string::npos)
          dirName.replace(percent_pos, 2, fileBasename);

        // Handling dir creation errors
        int mkdret = mkdir(dirName.c_str(),
                           S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        dirName.append("/");
        fileName.insert(0, "/");
        fileName.insert(0, dirName);
	fileName.append(GENERATION_SOURCE_FILENAME_EXTENSION);

        if (mkdret == -1 && errno != EEXIST)
	  emitError(diag_engine,
		    dummy,
		    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_CREATE_DIR,
		    &fileName);          
        else if (!generate_req_allow_overwrite &&
                 (stat (fileName.c_str(), &buffer) == 0))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_EXISTS,
		    &fileName);
	// Process template for creating source file
	else if (!processTemplate(tmpl, fileName, values_map))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION,
		    &fileName);
      }
      
      /**
       * ExecSQLFetchToFunctionCall::doRequestHeaderGeneration
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
      ExecSQLFetchToFunctionCall::doRequestHeaderGeneration(DiagnosticsEngine& diag_engine,
                                                            const std::string& tmpl,
                                                            string2_map& values_map)
      {	
	SourceLocation dummy;
        struct stat buffer;   

	// Compute output file pathname
        std::string dirName = generation_directory;
	std::string fileName = values_map["@RequestFunctionName@"];
	std::string fileBasename = values_map["@OriginalSourceFileBasename@"];

        // Replace %B with original .pc file Basename
        size_t percent_pos = std::string::npos;
        if ((percent_pos = dirName.find("%B")) != std::string::npos)
          dirName.replace(percent_pos, 2, fileBasename);

        // Handling dir creation errors
        int mkdret = mkdir(dirName.c_str(),
                           S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	dirName.append("/");
        fileName.insert(0, "/");
        fileName.insert(0, dirName);
	fileName.append(GENERATION_HEADER_FILENAME_EXTENSION);

        if (mkdret == -1 && errno != EEXIST)
	  emitError(diag_engine,
		    dummy,
		    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_CREATE_DIR,
		    &fileName);          
        else if (!generate_req_allow_overwrite &&
                 (stat (fileName.c_str(), &buffer) == 0))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_EXISTS,
		    &fileName);
	// Process template for creating header file
	else if (!processTemplate(tmpl, fileName, values_map))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION,
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
      ExecSQLFetchToFunctionCall::emitError(DiagnosticsEngine &diag_engine,
					    const SourceLocation& err_loc,
					    enum ExecSQLFetchToFunctionCallErrorKind kind,
					    const std::string* msgptr)
      {
	std::string msg;
	if (msgptr != nullptr && !msgptr->empty())
	  msg = *msgptr;
	
	/* 
	 * According to the kind of error, it is reported through 
	 * diag engine.
	 */
	unsigned diag_id;
	switch (kind)
          {
            /** Default unexpected diagnostic id */
          default:
            diag_id =TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Warning,
                              "Unexpected error occured?!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** No error ID: it should never occur */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_NO_ERROR:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Ignored,
                              "No error");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Access char data diag ID */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't access character data in file cache memory buffers!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Can't find a comment */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't find ProC comment start! This result has been discarded!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Cannot match comment */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Warning,
                              "Couldn't match ProC comment for function name creation!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Cannot generate request source file (no location) */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't generate request source file %0!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request header file (no location) */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't generate request header file %0!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request source file (already exists) */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_EXISTS:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Warning,
                              "Source file '%0' already exists: will not overwrite!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request header file (already exists) */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_EXISTS:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Warning,
                              "Header file '%0' already exists: will not overwrite!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request source file (create dir) */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_CREATE_DIR:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't create directory for '%0'!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request header file (no location) */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_CREATE_DIR:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't create directory for '%0'!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Unsupported String Literal charset */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_UNSUPPORTED_STRING_CHARSET:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Token for weird charset string (%0) found!");
            diag_engine.Report(diag_id).AddString(msg);
           break;

            /** Invalid groups file error */
          case ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_INVALID_GROUPS_FILE:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Cannot parse invalid groups file '%0'!");
            diag_engine.Report(diag_id).AddString(msg);
            break;
            
          }
      }
      
      /**
       * ExecSQLFetchToFunctionCall::findMacroStringLiteralDefAtLine
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
      ExecSQLFetchToFunctionCall::findMacroStringLiteralDefAtLine(SourceManager &src_mgr,
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
	    SourceLocation sl = sr.m_macro_range.getBegin();
	    unsigned sln = src_mgr.getSpellingLineNumber(sl);
	    //outs() << "Macro def at line#" << sln << " versus searching line#" << ln << " or line #" << ln+1 << "\n";
	    if (sln == ln || sln + 1 == ln)
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

      /*
       * ExecSQLFetchToFunctionCall::createParamsDef
       *
       * @brief Format a string for providing params definition
       *
       * This method allows to format a string containing one parameter
       * definition.
       *
       * @param[in] type	the type of the parameter
       * @param[in] elemtype	the element type in case of contant array
       * @param[in] size	the size of the constant array
       * @param[in] name	the name of the parameter
       *
       * @return the string formatted for providing params definition
       */
      std::string
      ExecSQLFetchToFunctionCall::createParamsDef(const std::string& type,
                                                 const std::string& elemtype,
                                                 const std::string& size,
                                                 const std::string& name)
      {
        return clang::tidy::pagesjaunes::createParamsDef(type, elemtype, size, name);
      }

      /*
       * ExecSQLFetchToFunctionCall::createParamsDeclareSection
       *
       * @brief Format a string for providing the declare section
       *
       * This method allows to format a string containing one parameter
       * and one host variable in a declare section
       *
       * @param[in] type	the type of the parameter/host variable
       * @param[in] elemtype	the element type in case of contant array
       * @param[in] size	the size of the constant array
       * @param[in] name	the name of the host variable
       * @param[in] paramname	the name of the parameter
       *
       * @return the string formatted for providing params/host vars
       *         declare section
       */
      std::string
      ExecSQLFetchToFunctionCall::createParamsDeclareSection(const std::string& type,
                                                            const std::string& elemtype,
                                                            const std::string& size,
                                                            const std::string& name,
                                                            const std::string& paramname)
      {
        return clang::tidy::pagesjaunes::createParamsDeclareSection(type, elemtype, size, name, paramname);
      }

      /*
       * ExecSQLFetchToFunctionCall::createParamsDecl
       *
       * @brief Format a string for providing params declaration
       *
       * This method allows to format a string containing one parameter
       * declaration.
       *
       * @param[in] type	the type of the parameter
       * @param[in] elemtype	the element type in case of contant array
       * @param[in] size	the size of the constant array
       *
       * @return the string formatted for providing params declaration
       */
      std::string
      ExecSQLFetchToFunctionCall::createParamsDecl(const std::string& type,
                                                  const std::string& elemtype,
                                                  const std::string& size)
      {
        return clang::tidy::pagesjaunes::createParamsDecl(type, elemtype, size);
      }
      
      /*
       * ExecSQLFetchToFunctionCall::createParamsCall
       *
       * @brief Format a string for providing function call arguments
       *
       * This method allows to format a string containing one parameter
       * for function call.
       *
       * @param[in] name	the name of the parameter
       *
       * @return the string formatted for providing params declaration
       */
      std::string
      ExecSQLFetchToFunctionCall::createParamsCall(const std::string& name)
      {
        return clang::tidy::pagesjaunes::createParamsCall(name);
      }
      
      /*
       * ExecSQLFetchToFunctionCall::createHostVarList
       *
       * @brief Format a string for providing host var list for the request
       *
       * This method allows to format a string containing one host variable
       * for the request.
       *
       * @param[in] name	the name of the host variable
       *
       * @return the string formatted for providing host variable list for
       *         the request
       */
      std::string
      ExecSQLFetchToFunctionCall::createHostVarList(const std::string& name, bool isIndicator = false)
      {
        return clang::tidy::pagesjaunes::createHostVarList(name, isIndicator);
      }
      
      /**
       * ExecSQLFetchToFunctionCall::findSymbolInFunction
       *
       * @brief Find a symbol, its definition and line number in the current function
       *
       * This method search the AST from the current function for a given symbol. When
       * found it return a record struct having pointer to AST nodes of interrest.
       *
       * @param[in] tool	The clang::tooling::Tool instance
       * @param[in] varName	The name of the symbol to find
       * @param[in] func	The AST node of the function to search into
       *
       * @return The pointer to the varDecl node instance for the searched symbol
       */
      const VarDecl *
      ExecSQLFetchToFunctionCall::findSymbolInFunction(std::string& varName, const FunctionDecl *func)
      {
        VarDeclMatcher vdMatcher(this);
        return clang::tidy::pagesjaunes::findSymbolInFunction(vdMatcher,
                                                              TidyContext->getToolPtr(),
                                                              varName,
                                                              func,
                                                              m_req_var_decl_collector);
      }

      /**
       * ExecSQLFetchToFunctionCall::findDeclInFunction
       *
       * @brief Find a declaration of a symbol in the context of a function by using the function
       * DeclContext iterators until the symbol is found.
       * This method do not update AST. It only browse known declarations in the context of a function.
       * On successfull completion the map will contain:
       * - a key "symName" with the symbol name provided
       * - a key "typeName" with the symbol type name found
       * - a key "elementType" with the type of the element if the type
       *   found is a constant array type
       * - a key "elemntSize" with the number of element if the type
       *   found is a constant array type
       *
       * @param[in] func 	the function of browse for finding the symbol
       * @param[in] symName 	the name of the symbol to find
       *
       * @return the map containing informations on the found symbol. The map is empty is no symbol
       *         with the same name were found.
       */
      string2_map
      ExecSQLFetchToFunctionCall::findDeclInFunction(const FunctionDecl *func, const std::string& symName)
      {
        return clang::tidy::pagesjaunes::findDeclInFunction(func, symName);
      }

      /**
       * ExecSQLFetchToFunctionCall::findCXXRecordMemberInTranslationUnit
       *
       * @brief This function will browse a translation unit and search for a specific 
       *        named CXXRecord and a named member of it
       * 
       * @param[in]	the translation unit declaration context to browse for the record
       * @param[in] 	the struct/union/class name to find in the context of the translation
       *                unit
       * @param[in] 	the member name to find in the class/struct
       *
       * @return a map containing informations about the found member. if no record/member were 
       *         found, the returned map is empty
       */
      string2_map
      ExecSQLFetchToFunctionCall::findCXXRecordMemberInTranslationUnit(const TranslationUnitDecl *transUnit,
                                                                      const std::string& cxxRecordName,
                                                                      const std::string& memberName)
      {
        return clang::tidy::pagesjaunes::findCXXRecordMemberInTranslationUnit(transUnit, cxxRecordName, memberName);
      }

      /**
       * ExecSQLFetchToFunctionCall::decodeHostVars
       *
       * This function decode an input string of host variables (and indicators). It
       * parse the string and return a data structure made of maps containing host
       * variables. 
       * It supports pointers and struct dereferencing and returns values of record 
       * or struct variables, members, indicators.
       * It trims record and member values.
       * The first level of map contains the number of the host variable as key and a map
       * for the host variable description.
       *
       *  #1 -> map host var #1
       *  #2 -> map host var #2
       *  #3 -> map host var #3
       *  ...
       *  #n -> map host var #n
       *
       * Each host var map contains some string keys for each variable component.
       * They are:
       *  - "full": the full match for this host variable (contains spaces and is the exact value matched)
       *  - "hostvar": the complete host var referencing expr (spaces were trimmed)
       *  - "hostrecord": the value of the host variable record/struct variable value.
       *  - "hostmember": the value of the record/struct member for this host variable expr.
       *  - "deref": Dereferencing operator for this host variable. Empty if not a dereferencing expr.  
       * 
       * For indicators the same field/keys are available with an 'i' appended for 'indicators'.
       * Unitary tests are availables in test/decode_host_vars.cpp
       * For example: the expr ':var1:Ivar1, :var2:Ivar2'
       * returns the following maps
       *
       * var #1                          var #2
       *     full = ':var1'                 full = ':var2'
       *     fulli = ':Ivar1, '             fulli = ':Ivar2'      
       *     hostvar = 'var1'               hostvar = 'var2'         
       *     hostvari = 'Ivar1'             hostvari = 'Ivar2'          
       *     hostrecord = 'var1'            hostrecord = 'var2'            
       *     hostrecordi = 'Ivar1'          hostrecordi = 'Ivar2'             
       *     hostmember = 'var1'            hostmember = 'var2'         
       *     hostmemberi = 'Ivar1'          hostmemberi = 'Ivar2'              
       *     deref = ''                     deref = ''      
       *     derefi = ''                    derefi = ''         
       *                             
       * @param[in]  hostVarList the host vars expr to parse/decode
       *
       * @return a map containing the host parsed variables expr
       */
      map_host_vars
      ExecSQLFetchToFunctionCall::decodeHostVars(const std::string &hostVarsList)
      {
        return clang::tidy::pagesjaunes::decodeHostVars(hostVarsList);
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
      ExecSQLFetchToFunctionCall::check(const MatchFinder::MatchResult &result) 
      {
        outs() << "void ExecSQLFetchToFunctionCall::check(const MatchFinder::MatchResult &result): ENTRY!\n";
        map_replacement_values rv;
        
	// Get the source manager
        ASTContext* astCtxt = result.Context;
	SourceManager &srcMgr = astCtxt->getSourceManager();
	DiagnosticsEngine &diagEngine = astCtxt->getDiagnostics();

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
	std::stringbuf slnbuffer;
	std::ostream slnos (&slnbuffer);
	slnos << startLineNum;
	std::string originalSourceFileBasename = srcMgr.getFileEntryForID(srcMgr.getMainFileID())->getName().str();
        // If original file name is a path, keep only basename
        // (erase all before / and all after .)
        if (originalSourceFileBasename.rfind("/") != std::string::npos)
          originalSourceFileBasename.erase(0, originalSourceFileBasename.rfind("/")+1);
        originalSourceFileBasename.erase(originalSourceFileBasename.rfind("."), std::string::npos);
	std::string originalSourceFilename = srcMgr.getFileEntryForID(srcMgr.getMainFileID())->getName().str().append(slnbuffer.str().insert(0, "#"));

	//outs() << "Found one result at line " << startLineNum << " of file '" << originalSourceFilename << "\n";

	/*
	 * Find the comment for the EXEC SQL statement
	 * -------------------------------------------
	 *
	 * Iterate from line +2 until finding the whole EXEC SQL comment
	 * Line +2 because line+1, when line numbers are generated by ProC, contains start of EXEC SQL
	 */
	
	// Current line number
	size_t lineNum = startLineNum+2;
	
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
	
        // If #line is available keep line number and filename of the .pc file
        unsigned int pcLineNumStart = 0;
        unsigned int pcLineNumEnd = 0;
        std::string pcFilename;
        bool found_line_info = false;
        
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
		
                if (lineData.find("#line ") != std::string::npos)
                  {
                    // Keep the line number and file:
                    // Format of the preprocessor #line
                    // #line <linenum> <filepath>
                    // linenum is an unsigned int
                    // and filepath is a dbl-quoted string
                    Regex lineDefineRe(PAGESJAUNES_REGEX_EXEC_SQL_ALL_LINE_DEFINE_RE);
                    SmallVector<StringRef, 8> lineMatches;
                    if (lineDefineRe.match(lineData, &lineMatches))
                      {
                        found_line_info = true;
                        std::istringstream isstr;
                        std::stringbuf *pbuf = isstr.rdbuf();
                        pbuf->str(lineMatches[1]);
                        if (pcLineNumStart)
                          isstr >> pcLineNumEnd;
                        else
                          isstr >> pcLineNumStart;
                        pcFilename = lineMatches[2];
                      }
                    else
                      {
                        errs() << "Cannot match a #line definition !\n";
                      }
                  }
                
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
            bool had_cr = false;

	    // Iterate
	    do
	      {
		// If one has been found
		if (crpos != std::string::npos)
		  {
		    // Erase CR
		    comment.erase(crpos, 1);
                    had_cr = true;
		  }
		// Find again CR
		crpos = comment.find("\n", 0);
	      }
	    // Until no more is found
	    while (crpos != std::string::npos);

	    outs() << "comment found for compound statement at line #" << startLineNum << ": '" << comment << "'\n";

	    /*
	     * Create function call for the request
	     */

	    // Regex for processing comment for all remaining requests (see ExecSQLCommon.h)
	    Regex fetchReqRe(PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE);
	    
	    // Returned matches
	    SmallVector<StringRef, 8> matches;

	    /* Templating engines vars
	     *   - RequestFunctionName
	     *   - RequestFunctionArgs
	     *   - RequestFunctionParamDef
	     *   - RequestFunctionParamDecl
	     *   - RequestExecSql
	     */
	    std::string requestFunctionName;
	    std::string requestFunctionArgs;
	    std::string requestExecSqlDeclareSection;
	    std::string requestFunctionParamsDef;
	    std::string requestFunctionParamsDecl;
	    std::string requestExecSql;
	    std::string generationDateTime;

	    std::string requestInto;
	    std::string requestArgs;

	    /*
	     * Now we match against a more permissive regex for all other (simpler) requests
	     * =============================================================================
	     */
	    if (fetchReqRe.match(comment, &matches))
	      {
		/*
		 * Find the request var assignment
		 */
		
		// The request name used in ProC
                requestExecSql = matches[PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_EXECSQL];
                requestExecSql.append(" ");
		std::string reqName = matches[PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_REQNAME];
		std::string intoReqNames = matches[PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_INTONAMES];
                std::string newHostVarList;
                std::set<std::string> cursorArgsSet;
                emplace_ret_t emplace_ret;

                // Let's iterate over each host variable (if some are used)
                map_host_vars mhv = decodeHostVars(intoReqNames);
                for (auto mhvit = mhv.begin(); mhvit != mhv.end(); ++mhvit)
                  {
                    auto hvm = mhvit->second;
                    // We have one host variable
                    if (!hvm["hostvar"].empty())
                      {
                        // Find the corresponding declaration in the function
                        string2_map hostVarMap = findDeclInFunction(curFunc, hvm["hostrecord"]);
                        // If the host variable is a field of a struct
                        if (!hvm["deref"].empty())
                          {
                            // Find the struct definition for getting the field
                            string2_map memberVarMap = findCXXRecordMemberInTranslationUnit(result.Context->getTranslationUnitDecl(),
                                                                                            hostVarMap["typeName"],
                                                                                            hvm["hostmember"]);
                            // Let's rename the parameter by prepending 'a_'
                            // FIXME: handle potential collisions on param names
                            std::string paramName = "a_";
                            paramName.append(memberVarMap["fieldName"]);

                            if (generation_simplify_function_args)
			      emplace_ret = cursorArgsSet.emplace(paramName);

			    if (!generation_simplify_function_args || emplace_ret.second)
			      {
                                // Format the various usage we make of it
                                requestFunctionParamsDef.append(createParamsDef(memberVarMap["fieldTypeName"],
                                                                                memberVarMap["elementType"],
                                                                                memberVarMap["elementSize"],
                                                                                paramName));
                                requestExecSqlDeclareSection.append(createParamsDeclareSection(memberVarMap["fieldTypeName"],
                                                                                               memberVarMap["elementType"],
                                                                                               memberVarMap["elementSize"],
                                                                                               memberVarMap["fieldName"],
                                                                                               paramName));
                                requestFunctionParamsDecl.append(createParamsDecl(memberVarMap["fieldTypeName"],
                                                                                  memberVarMap["elementType"],
                                                                                  memberVarMap["elementSize"]));
                                requestFunctionArgs.append(createParamsCall(hvm["hostvar"]));
                                newHostVarList.append(createHostVarList(memberVarMap["fieldName"]));
                              }
                          }
                        // Host var is a standard decl in the function
                        else
                          {
                            // Let's rename the parameter by prepending 'a_'
                            std::string paramName = "a_";
                            paramName.append(hostVarMap["symName"]);
                            
                            if (generation_simplify_function_args)
			      emplace_ret = cursorArgsSet.emplace(paramName);

			    if (!generation_simplify_function_args || emplace_ret.second)
			      {
                                // Format the various usage we make of it
                                requestFunctionParamsDef.append(createParamsDef(hostVarMap["typeName"],
                                                                                hostVarMap["elementType"],
                                                                                hostVarMap["elementSize"],
                                                                                paramName));
                                requestExecSqlDeclareSection.append(createParamsDeclareSection(hostVarMap["typeName"],
                                                                                               hostVarMap["elementType"],
                                                                                               hostVarMap["elementSize"],
                                                                                               hostVarMap["symName"],
                                                                                               paramName));
                                requestFunctionParamsDecl.append(createParamsDecl(hostVarMap["typeName"],
                                                                                  hostVarMap["elementType"],
                                                                                  hostVarMap["elementSize"]));
                                requestFunctionArgs.append(createParamsCall(hostVarMap["symName"]));
                                newHostVarList.append(createHostVarList(hostVarMap["symName"]));
                              }
                          }                        
                      }

                    // We also have an indicator for this host variable
                    if (!hvm["hostvari"].empty())
                      {
                        // Find the decl in func
                        string2_map hostIndicVarMap = findDeclInFunction(curFunc, hvm["hostrecordi"]);
                        // Member of a struct
                        if (!hvm["derefi"].empty())
                          {
                            // Find the struct definition for getting field specification
                            string2_map memberIndicVarMap = findCXXRecordMemberInTranslationUnit(result.Context->getTranslationUnitDecl(),
                                                                                                 hostIndicVarMap["typeName"],
                                                                                                 hvm["hostmemberi"]);
                            // Prepend "a_" for param name
                            std::string paramName = "a_";
                            paramName.append(memberIndicVarMap["fieldName"]);

                            if (generation_simplify_function_args)
			      emplace_ret = cursorArgsSet.emplace(paramName);

			    if (!generation_simplify_function_args || emplace_ret.second)
			      {
                                // Format various usage we make of it
                                requestFunctionParamsDef.append(createParamsDef(memberIndicVarMap["fieldTypeName"],
                                                                                memberIndicVarMap["elementType"],
                                                                                memberIndicVarMap["elementSize"],
                                                                                paramName));
                                requestExecSqlDeclareSection.append(createParamsDeclareSection(memberIndicVarMap["fieldTypeName"],
                                                                                               memberIndicVarMap["elementType"],
                                                                                               memberIndicVarMap["elementSize"],
                                                                                               memberIndicVarMap["fieldName"],
                                                                                               paramName));
                                requestFunctionParamsDecl.append(createParamsDecl(memberIndicVarMap["fieldTypeName"],
                                                                                  memberIndicVarMap["elementType"],
                                                                                  memberIndicVarMap["elementSize"]));
                                requestFunctionArgs.append(createParamsCall(hvm["hostvari"]));
                                newHostVarList.append(createHostVarList(memberIndicVarMap["fieldName"], /*isIndicator=*/true));
                              }
                          }
                        // Not a member of struct
                        else
                          {
                            // Compute a param name
                            std::string paramName = "a_";
                            paramName.append(hostIndicVarMap["symName"]);

                            if (generation_simplify_function_args)
			      emplace_ret = cursorArgsSet.emplace(paramName);

			    if (!generation_simplify_function_args || emplace_ret.second)
			      {
                                // And format usages
                                requestFunctionParamsDef.append(createParamsDef(hostIndicVarMap["typeName"],
                                                                                hostIndicVarMap["elementType"],
                                                                                hostIndicVarMap["elementSize"],
                                                                                paramName));
                                requestExecSqlDeclareSection.append(createParamsDeclareSection(hostIndicVarMap["typeName"],
                                                                                               hostIndicVarMap["elementType"],
                                                                                               hostIndicVarMap["elementSize"],
                                                                                               hostIndicVarMap["symName"],
                                                                                               paramName));
                                requestFunctionParamsDecl.append(createParamsDecl(hostIndicVarMap["typeName"],
                                                                                  hostIndicVarMap["elementType"],
                                                                                  hostIndicVarMap["elementSize"]));
                                requestFunctionArgs.append(createParamsCall(hostIndicVarMap["symName"]));
                                newHostVarList.append(createHostVarList(hostIndicVarMap["symName"], /*isIndicator=*/true));
                              }
                          }
                      }
                    else
                      newHostVarList.append(createHostVarList("", /*isIndicator=*/true));
                  }

                // Remove last commas + space
                if (requestFunctionParamsDef.length() > 2)
                  requestFunctionParamsDef.erase(requestFunctionParamsDef.length() -2, std::string::npos);
                else
                  requestFunctionParamsDef.assign("void");
                
                // Remove last commas + space
                if (requestFunctionParamsDecl.length() > 2)
                  requestFunctionParamsDecl.erase(requestFunctionParamsDecl.length() -2, std::string::npos);
                else
                  requestFunctionParamsDecl.assign("void");
                
                // Remove last commas + space
                if (requestFunctionArgs.length() > 2)
                  requestFunctionArgs.erase(requestFunctionArgs.length() -2, std::string::npos);
                
                // Remove last CR
                if (requestExecSqlDeclareSection.length() > 2)
                  {
                    requestExecSqlDeclareSection.insert(0, "    EXEC SQL BEGIN DECLARE SECTION;\n");
                    requestExecSqlDeclareSection.append("    EXEC SQL END DECLARE SECTION;\n");
                  }
                else
                  requestExecSqlDeclareSection.assign("    // No declare section");

                // Remove last comma + space
                if (newHostVarList.length() > 2)
                  newHostVarList.erase(newHostVarList.length() -2, std::string::npos);
                    
                if (generation_do_report_modification_in_pc)
                  {
                    std::ostringstream had_cr_strstream;
                    had_cr_strstream << had_cr;
                    rv.insert(std::pair<std::string, std::string>("had_cr", had_cr_strstream.str()));
                    rv.insert(std::pair<std::string, std::string>("fullcomment", comment));
                    rv.insert(std::pair<std::string, std::string>("reqname", reqName));
                    rv.insert(std::pair<std::string, std::string>("intoreqnames", intoReqNames));
                    if (found_line_info)
                      {
                        rv.insert(std::pair<std::string, std::string>("pcfilename", pcFilename));
                        // Exec SQL Start line
                        std::ostringstream pc_line_num_start;
                        pc_line_num_start << pcLineNumStart;
                        rv.insert(std::pair<std::string, std::string>("pclinenumstart", pc_line_num_start.str()));
                        outs() << "Found #line for comment: parsed line num start = " << pcLineNumStart << " from file: '" << pcFilename << "'\n";
                        // Exec SQL End line
                        std::ostringstream pc_line_num_end;
                        pc_line_num_end << pcLineNumEnd;
                        rv.insert(std::pair<std::string, std::string>("pclinenumend", pc_line_num_end.str()));
                        outs() << "Found #line for comment: parsed line num end = " << pcLineNumEnd << " from file: '" << pcFilename << "'\n";
                      }
                  }
                    
                requestInto.assign(" ");
                requestInto.append(matches[PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_INTO]);
                requestInto.append(" ");
                
                requestExecSql.append(reqName);
		requestExecSql.append(requestInto);
		requestExecSql.append(newHostVarList);
		
		requestFunctionName = "fetch";
		reqName[0] &= ~0x20;
		requestFunctionName.append(reqName);
		
		// Got it, emit changes
		//outs() << "** Function name = " << function_name << " for proC block at line # " << startLineNum << "\n";
		
                if (generation_do_report_modification_in_pc)
                  {
                    rv.insert(std::pair<std::string, std::string>("execsql", requestExecSql));
                    rv.insert(std::pair<std::string, std::string>("funcname", requestFunctionName));
                  }

                auto now_time = system_clock::to_time_t(system_clock::now());
                generationDateTime = std::ctime(&now_time);

		// If headers generation was requested
		if (generate_req_headers)
		  {
		    // Build the map for templating engine
		    string2_map values_map;
		    values_map["@RequestFunctionName@"] = requestFunctionName;
		    values_map["@RequestFunctionParamsDecl@"] = requestFunctionParamsDecl;
		    values_map["@OriginalSourceFilename@"] = originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1);
                    values_map["@OriginalSourceFileBasename@"] = originalSourceFileBasename;
                    values_map["@GenerationDateTime@"] = generationDateTime;

		    // And call it
		    doRequestHeaderGeneration(diagEngine,
					      generation_header_template,
					      values_map);
		  }
		
		// If source generation was requested
		if (generate_req_sources)
		  {
		    // Build the map for templating engine
		    string2_map values_map;
		    values_map["@RequestFunctionName@"] = requestFunctionName;
		    values_map["@RequestFunctionParamsDef@"] = requestFunctionParamsDef;
		    values_map["@ExecSqlDeclareSection@"] = requestExecSqlDeclareSection;
		    values_map["@OriginalSourceFilename@"] = originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1);
                    values_map["@OriginalSourceFileBasename@"] = originalSourceFileBasename;
		    values_map["@RequestExecSql@"] = requestExecSql;
                    values_map["@GenerationDateTime@"] = generationDateTime;
		    // And call it
		    doRequestSourceGeneration(diagEngine,
					      generation_source_template,
					      values_map);
		  }
		
		// Emit errors, warnings and fixes
		std::string rplt_code = emitDiagAndFix(loc_start, loc_end, requestFunctionName, requestFunctionArgs);

                if (generation_do_report_modification_in_pc)
                  {
                    rv.insert(std::pair<std::string, std::string>("rpltcode", rplt_code));
                    rv.insert(std::pair<std::string, std::string>("originalfile", originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1)));
                    std::ostringstream commentStartLineNum;
                    commentStartLineNum << comment << ":" << startLineNum;
                    outs() << "Replacements values for " << commentStartLineNum.str() << "\n";
                    for (auto repit = rv.begin(); repit != rv.end(); ++repit)
                      {
                        outs() << "key: '" << repit->first << "' = '" << repit->second << "'\n";
                      }
                    replacement_per_comment.insert(std::pair<std::string, std::map<std::string, std::string>>(commentStartLineNum.str(), rv));
                  }                
	      }
	    
	    else
	      // Didn't match comment at all!
	      emitError(diagEngine,
	      		comment_loc_start,
	      		ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH);
	  }
	else
	  {
	    if (errOccured)
	      // An error occured while accessing memory buffer for sources
	      emitError(diagEngine,
			loc_start,
			ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA);

	    else
	      // We didn't found the comment start ???
	      emitError(diagEngine,
			comment_loc_end,
			ExecSQLFetchToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START);
	  }
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
