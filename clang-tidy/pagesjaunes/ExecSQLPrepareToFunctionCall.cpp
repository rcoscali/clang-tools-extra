//===--- ExecSQLPrepareToFunctionCall.cpp - clang-tidy ----------------------------===//
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

#include "ExecSQLPrepareToFunctionCall.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
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

      namespace
      {
	// Keep track of macro expansions that defines a StringLiteral
	// then keep the associated source_range_set_t to be able to get the
	// original define (for keeping indentation)
	class GetStringLiteralsDefines : public PPCallbacks
	{
	public:
	  explicit GetStringLiteralsDefines(ExecSQLPrepareToFunctionCall *parent,
					    ExecSQLPrepareToFunctionCall::source_range_set_t *srset)
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
					    ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_UNSUPPORTED_STRING_CHARSET,
					    &slKind);
		      }
		  }
		
		if (have_literal)
		  {
		    //outs() << "Adding macro '" << macroName << "' expansion at " << range.getBegin().printToString(m_src_mgr) << "...'\n";
		    ExecSQLPrepareToFunctionCall::SourceRangeForStringLiterals *ent
		      = new ExecSQLPrepareToFunctionCall::SourceRangeForStringLiterals(range, sr, macroName);
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
	  ExecSQLPrepareToFunctionCall *m_parent;
	  const SourceManager &m_src_mgr; 
	  DiagnosticsEngine &m_diag_engine;
	  ExecSQLPrepareToFunctionCall::source_range_set_t *m_macrosStringLiteralsPtr;
	};
      } // namespace
      
      /**
       * ExecSQLPrepareToFunctionCall constructor
       *
       * @brief Constructor for the ExecSQLPrepareToFunctionCall rewriting check
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
      ExecSQLPrepareToFunctionCall::ExecSQLPrepareToFunctionCall(StringRef Name,
								 ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),	/** Init check (super class) */
	  TidyContext(Context),			/** Init our TidyContext instance */

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
						 "./pagesjaunes_prepare.h.tmpl")),
	  /// Generation source default template (string)
	  generation_source_template(Options.get("Generation-source-template",
						 "./pagesjaunes_prepare.pc.tmpl")),
	  /// Request grouping option: Filename containing a json map for
	  /// a group name indexing a vector of requests name
	  generation_request_groups(Options.get("Generation-request-groups",
						"./request_groups.json")),
          /// Conditionnaly report modification in .pc file if this is true
          generation_do_report_modification_in_pc(Options.get("Generation-do-report-modification-in-PC",
                                                              false)),
          /// Directory of the original .pc file in which to report modification
          generation_report_modification_in_dir(Options.get("Generation-report-modification-in-dir",
                                                             "./"))
      {
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
	    emitError(Context->getASTContext()->getDiagnostics(),
		      SourceLocation(),
		      ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_INVALID_GROUPS_FILE,
		      &generation_request_groups);
	  }
      }
      
      /**
       * onStartOfTranslationUnit
       *
       * @brief called at start of processing of translation unit
       *
       * Override to be called at start of translation unit
       */
      void ExecSQLPrepareToFunctionCall::onStartOfTranslationUnit()
      {
        replacement_per_comment.clear();
      }
      
      /**
       * onEndOfTranslationUnit
       *
       * @brief called at end of processing of translation unit
       *
       * Override to be called at end of translation unit
       */
      void ExecSQLPrepareToFunctionCall::onEndOfTranslationUnit()
      {
        // Get data from processed requests
        for (auto it = replacement_per_comment.begin(); it != replacement_per_comment.end(); ++it)
          {
            bool had_cr = false;
            std::string execsql;
            std::string fullcomment;
            std::string funcname;
            std::string originalfile;
            std::string filename;
            unsigned int line = 0;
            std::string reqname;
            std::string rpltcode;
            unsigned int pcLineNum = 0;
            std::string pcFilename;
            bool has_pcFilename = false;
            bool has_pcLineNum = false;
            bool has_pcFileLocation = false;
        
            std::string comment = it->first;
            auto map_for_values = it->second;
            
            for (auto iit = map_for_values.begin(); iit != map_for_values.end(); ++iit)
              {
                std::string key = iit->first;
                std::string val = iit->second;

                if (key.compare("had_cr") == 0)
                  had_cr = (val.compare("1") == 0);

                else if (key.compare("execsql") == 0)
                  execsql = val;

                else if (key.compare("fullcomment") == 0)
                  fullcomment = val;

                else if (key.compare("funcname") == 0)
                  funcname = val;

                else if (key.compare("originalfile") == 0)
                  {
                    Regex fileline("^(.*)#([0-9]+)$");
                    SmallVector<StringRef, 8> fileMatches;

                    if (fileline.match(val, &fileMatches))
                      {
                        originalfile = fileMatches[1];
                        std::istringstream isstr;
                        std::stringbuf *pbuf = isstr.rdbuf();
                        pbuf->str(fileMatches[2]);
                        isstr >> line;
                      }

                    else
                      originalfile = val;

                  }
                else if (key.compare("reqname") == 0)
                  reqname = val;

                else if (key.compare("rpltcode") == 0)
                  rpltcode = val;

                else if (key.compare("pclinenum") == 0)
                  {
                    std::istringstream isstr;
                    std::stringbuf *pbuf = isstr.rdbuf();
                    pbuf->str(val);
                    isstr >> pcLineNum;
                    has_pcLineNum = true;
                  }
                else if (key.compare("pcfilename") == 0)
                  {
                    pcFilename = val;
                    has_pcFilename = true;
                  }                
              }

            if (has_pcLineNum && has_pcFilename)
              has_pcFileLocation = true;

            char* buffer = nullptr;
            StringRef Buffer;
            std::string NewBuffer;
            
            // If #line are not present, need to find the line in file with regex
            if (!has_pcFileLocation)
              {
                // Without #line, we need to compute the filename
                pcFilename = generation_report_modification_in_dir;
                pcFilename.append("/");
                std::string::size_type dot_c_pos = originalfile.find('.');
                pcFilename.append(originalfile.substr(0, dot_c_pos +1));
                pcFilename.append("pc");

                std::ifstream ifs (pcFilename, std::ifstream::in);
                if (ifs.is_open())
                  {
                    std::filebuf* pbuf = ifs.rdbuf();
                    std::size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);

                    if (!size)
                      // TODO: add reported llvm error
                      outs() << originalfile << ":" << line
                             << ":1: warning: Your original file in which to report modifications is empty !\n";

                    else
                      {
                        pbuf->pubseekpos (0,ifs.in);
                        // allocate memory to contain file data
                        buffer=new char[size];
                        
                        // get file data
                        pbuf->sgetn (buffer,size);
                        ifs.close();
                        
                        Buffer = StringRef(buffer);
                        std::string allocReqReStr = "(EXEC SQL[[:space:]]+";
                        allocReqReStr.append(execsql);
                        allocReqReStr.append(");");
                        Regex allocReqRe(allocReqReStr.c_str(), Regex::NoFlags);
                        SmallVector<StringRef, 8> allocReqMatches;
                        
                        if (allocReqRe.match(Buffer, &allocReqMatches))
                          {
                            StringRef RpltCode(rpltcode);
                            NewBuffer = allocReqRe.sub(RpltCode, Buffer);
                          }

                        else
                          {
                            outs() << originalfile << ":" << line
                                   << ":1: warning: Couldn't find 'EXEC SQL " << execsql
                                   << ";' statement to replace with '" << rpltcode
                                   << "' in original '" << pcFilename
                                   << "' file ! Already replaced ?\n";
                            NewBuffer = Buffer.str();
                          }
                      }
                  }

                else
                  // TODO: add reported llvm error
                  outs() << originalfile << ":" << line
                         << ":1: warning: Cannot open original file in which to report modifications: " << pcFilename
                         << "\n";
              }
            else
              {
                std::ifstream ifs (pcFilename, std::ifstream::in);
                if (ifs.is_open())
                  {
                    std::filebuf* pbuf = ifs.rdbuf();
                    std::size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);
                    pbuf->pubseekpos (0,ifs.in);

                    if (!size)
                      // TODO: add reported llvm error
                      outs() << originalfile << ":" << line
                             << ":1: warning: Original file in which to report modifications is empty !\n";

                    else
                      {
                        // allocate memory to contain file data
                        buffer=new char[size];
                        
                        // get file data
                        pbuf->sgetn (buffer, size);
                        ifs.close();
                        
                        StringRef Buffer(buffer);
                        SmallVector<StringRef, 10000> linesbuf;
                        Buffer.split(linesbuf, '\n');
                        
                        pcLineNum--;
                        unsigned int pcStartLineNum = pcLineNum;
                        unsigned int pcEndLineNum = pcLineNum;
                        
                        if (had_cr)
                          while (linesbuf[pcStartLineNum].find("EXEC") == StringRef::npos)
                            pcStartLineNum--;
                        
                        if (pcEndLineNum > pcStartLineNum)
                          for (unsigned int n = pcStartLineNum+1; n <= pcEndLineNum; n++)
                            outs() << (n+1) << " " << linesbuf[n].str() << "\n";
                        
                        size_t startpos = linesbuf[pcStartLineNum].find(StringRef("EXEC"));
                        size_t endpos = linesbuf[pcEndLineNum].rfind(';');
                        std::string indent = linesbuf[pcStartLineNum].substr(0, startpos).str();
                        if (startpos != StringRef::npos)
                          {
                            std::string newline = indent;
                            newline.append(rpltcode);
                            if (endpos != StringRef::npos)
                              newline.append(linesbuf[pcEndLineNum].substr(endpos+1, StringRef::npos));
                            linesbuf[pcStartLineNum] = StringRef(newline);
                            if (pcEndLineNum > pcStartLineNum)
                              for (unsigned int n = pcStartLineNum+1; n <= pcEndLineNum; n++)
                                linesbuf[n] = indent;
                          }

                        else
                          outs() << originalfile << ":" << line
                                 << ":1: warning: Couldn't find 'EXEC SQL " << execsql
                                 << ";' statement to replace with '" << rpltcode
                                 << "' in original '" << pcFilename
                                 << "' file! Already replaced ?\n";
                          
                        NewBuffer.clear();
                        for (auto it = linesbuf.begin(); it != linesbuf.end(); ++it)
                          {
                            StringRef line = *it;
                            NewBuffer.append(line.str());
                            NewBuffer.append("\n");
                          }
                      }
                  }
              }

            std::ofstream ofs(pcFilename, std::ios::out | std::ios::trunc);
            if (ofs.is_open())
              {
                ofs.write(NewBuffer.c_str(), NewBuffer.size());
                ofs.flush();
                ofs.close();
              }

            else
              // TODO: add reported llvm error
              outs() << originalfile << ":" << line
                     << ":1: warning: Cannot overwrite file " << pcFilename << " !\n";

            delete buffer;
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
       * - Generation-request-groups
       *
       * @param Opts	The option map in which to store supported options
       */
      void
      ExecSQLPrepareToFunctionCall::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
	Options.store(Opts, "Generate-requests-headers", generate_req_headers);
	Options.store(Opts, "Generate-requests-sources", generate_req_sources);
	Options.store(Opts, "Generation-directory", generation_directory);
	Options.store(Opts, "Generation-header-template", generation_header_template);
	Options.store(Opts, "Generation-source-template", generation_source_template);
	Options.store(Opts, "Generation-request-groups", generation_request_groups);
        Options.store(Opts, "Generation-do-report-modification-in-PC", generation_do_report_modification_in_pc);
        Options.store(Opts, "Generation-report-modification-in-dir", generation_report_modification_in_dir);        
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
      ExecSQLPrepareToFunctionCall::registerMatchers(MatchFinder *Finder) 
      {
	/* Add a matcher for finding compound statements starting */
	/* with a sqlstm variable declaration */
        Finder->addMatcher(varDecl(
		      hasAncestor(declStmt(hasAncestor(compoundStmt(hasAncestor(functionDecl().bind("function"))).bind("proCBlock")))),
		      hasName("sqlstm"))
			   , this);
      }

      /**
       * ExecSQLPrepareToFunctionCall::registerPPCallbacks
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
      ExecSQLPrepareToFunctionCall::registerPPCallbacks(CompilerInstance &compiler)
      {
	compiler
	  .getPreprocessor()
	  .addPPCallbacks(llvm::make_unique<GetStringLiteralsDefines>(this,
								      &m_macrosStringLiterals));
      }
      
      
      /*
       * ExecSQLPrepareToFunctionCall::emitDiagAndFix
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
      ExecSQLPrepareToFunctionCall::emitDiagAndFix(const SourceLocation& loc_start,
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

        return replt_code;
      }

      /**
       * ExecSQLPrepareToFunctionCall::processTemplate
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
      ExecSQLPrepareToFunctionCall::processTemplate(const std::string& tmpl,
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
       * ExecSQLPrepareToFunctionCall::doRequestSourceGeneration
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
      ExecSQLPrepareToFunctionCall::doRequestSourceGeneration(DiagnosticsEngine& diag_engine,
						       const std::string& tmpl,
						       string2_map& values_map)
      {
	SourceLocation dummy;
	// Compute output file pathname
	std::string fileName = values_map["@RequestFunctionName@"];
	fileName.append(GENERATION_SOURCE_FILENAME_EXTENSION);
	fileName.insert(0, "/");
	fileName.insert(0, generation_directory);
	// Process template for creating source file
	if (!processTemplate(tmpl, fileName, values_map))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION,
		    &fileName);
      }
      
      /**
       * ExecSQLPrepareToFunctionCall::doRequestHeaderGeneration
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
      ExecSQLPrepareToFunctionCall::doRequestHeaderGeneration(DiagnosticsEngine& diag_engine,
							      const std::string& tmpl,
							      string2_map& values_map)
      {	
	SourceLocation dummy;
	// Compute output file pathname
	std::string fileName = values_map["@RequestFunctionName@"];
	fileName.append(GENERATION_HEADER_FILENAME_EXTENSION);
	fileName.insert(0, "/");
	fileName.insert(0, generation_directory);
	// Process template for creating header file
	if (!processTemplate(tmpl, fileName, values_map))
	  emitError(diag_engine,
		    dummy,
		    ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION,
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
      ExecSQLPrepareToFunctionCall::emitError(DiagnosticsEngine &diag_engine,
				       const SourceLocation& err_loc,
				       enum ExecSQLPrepareToFunctionCallErrorKind kind,
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
                              "Unexpected error occured?!") ;
            diag_engine.Report(err_loc, diag_id);
            break;

            /** No error ID: it should never occur */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_NO_ERROR:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Remark,
                              "No error");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Access char data diag ID */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't access character data in file cache memory buffers!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Can't find a comment */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't find ProC comment start! This result has been discarded!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Cannot match comment */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Warning,
                              "Couldn't match ProC comment for function name creation!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Cannot generate request source file (no location) */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't generate request source file %0!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request header file (no location) */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Couldn't generate request header file %0!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Unsupported String Literal charset */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_UNSUPPORTED_STRING_CHARSET:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Token for weird charset string (%0) found!");
            diag_engine.Report(diag_id).AddString(msg);
           break;

            /** Invalid groups file error */
          case ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_INVALID_GROUPS_FILE:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Cannot parse invalid groups file '%0'!");
            diag_engine.Report(diag_id).AddString(msg);
            break;
            
          }
      }
      
      /**
       * ExecSQLPrepareToFunctionCall::findMacroStringLiteralDefAtLine
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
      ExecSQLPrepareToFunctionCall::findMacroStringLiteralDefAtLine(SourceManager &src_mgr,
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
      ExecSQLPrepareToFunctionCall::check(const MatchFinder::MatchResult &result) 
      {
        map_replacement_values rv;
        
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
	std::stringbuf slnbuffer;
	std::ostream slnos (&slnbuffer);
	slnos << startLineNum;
	std::string originalSourceFilename = srcMgr.getFileEntryForID(srcMgr.getMainFileID())->getName().str().append(slnbuffer.str().insert(0, "#"));

	outs() << "Found one result at line " << startLineNum << " of file '" << originalSourceFilename << "\n";

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

	    //outs() << "comment for compound statement at line #" << startLineNum << ": '" << comment << "'\n";

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
		
                if (generation_do_report_modification_in_pc)
                  {
                    std::ostringstream had_cr_strstream;
                    had_cr_strstream << had_cr;
                    rv.insert(std::pair<std::string, std::string>("had_cr", had_cr_strstream.str()));
                    rv.insert(std::pair<std::string, std::string>("fullcomment", comment));
                    rv.insert(std::pair<std::string, std::string>("reqname", reqName));
                    rv.insert(std::pair<std::string, std::string>("fromreqname", fromReqName));
                  }

		//outs() << "!!!*** Prepare request comment match: from request name is '" << fromReqName
		//       << "' and request name is '"<< reqName << "'\n";

		/*
		 * First let's try to find a request that is initialized from a 
		 * string literal (constant string) with sprintf
		 * The next statement matcher find such requests
		 */
                StatementMatcher m_matcher
		  // find a sprintf call expr
		  = callExpr(hasDescendant(declRefExpr(hasDeclaration(namedDecl(hasName("sprintf"))))),
			 // with arg 0 being the symbol specified in the ProC request (target decl bound to vardecl)
                         hasArgument(0,
			     declRefExpr(hasDeclaration(varDecl(namedDecl(hasName(fromReqName.c_str()))).bind("vardecl")))),
			 // and with arg1 being the searched literal (literal bound to reqLiteral)
                         hasArgument(1,
                             stringLiteral().bind("reqLiteral")),
                         // and this call is in the current function (call bound to callExpr))
			     hasAncestor(functionDecl(hasName(curFunc->getNameAsString().insert(0, "::"))))).bind("callExpr");

		// Prepare matcher finder for our customized matcher
		CopyRequestMatcher crMatcher(this);
		// The matcher implementing visitor pattern
		MatchFinder finder;
		// Add our matcher to our processing class
		finder.addMatcher(m_matcher, &crMatcher);
		// Clear collector vector (the result filled with found patterns)
		m_req_copy_collector.clear();
		// Run the visitor pattern for collecting matches
		tool->run(newFrontendActionFactory(&finder).get());

		// Display number of matches
		//outs() << "Found " << m_req_copy_collector.size() << " sprintf calls\n";
		
		// The collected records
		struct StringLiteralRecord *lastRecord = (struct StringLiteralRecord *)nullptr;

		// Some result that will be passed to the templating engine
		std::string requestDefineValue;
		std::string requestDefineName;
		std::string fromReqNameLength;

		// If result is not an empty set
		// We found a request of type:
		// #define GIVreq "select ...."
		//     char reqBuf[1234];
		//     sprintf(reqBuf, GIVreq);
		//     EXEC SQL prepare aRequest from :reqBuf;
		if (!m_req_copy_collector.empty())
		  {
		    // Let's iterate over all collected matches
		    for (auto it = m_req_copy_collector.begin();
			 it != m_req_copy_collector.end();
			 ++it)
		      {
			// And find the one just before the ProC statement
			struct StringLiteralRecord *record = (*it);
			if (record->call_linenum > startLineNum)
			  break;
			// Didn't find yet, so keep current as the last one
			lastRecord = record;
		      }
		    
		    // Found the last record collected just before the ProC statement
		    if (lastRecord != nullptr)
		      {
			// Get the qualified type for the sprintf target variable
			QualType fromType = lastRecord->varDecl->getType().withConst();
			// We need the size of the char table
			CharUnits cu = TidyContext->getASTContext()->getTypeSizeInChars(fromType);
			size_t fromReqNameLengthSize = (size_t)cu.getQuantity();

			// Let's format this size in a string for templating engine
			std::stringbuf buffer;      // empty stringbuf
			std::ostream bos (&buffer);  // associate stream buffer to stream
			bos << fromReqNameLengthSize;
			fromReqNameLength = buffer.str();

			SourceRangeForStringLiterals *sr;
			
			if (findMacroStringLiteralDefAtLine(srcMgr,
							    lastRecord->linenum,
							    requestDefineName, requestDefineValue,
							    &sr))
			  {
			    //unsigned literalStartLineNumber = srcMgr.getSpellingLineNumber(sr->m_macro_range.getBegin());
			    //unsigned literalEndLineNumber = srcMgr.getSpellingLineNumber(sr->m_macro_range.getEnd());
			    // outs() << "*>*>*> Request Literal '" << requestDefineName << "'from line #"
			    // 	   << literalStartLineNumber << " to line #" << literalEndLineNumber << "\n";
			    // outs() << "\"" << requestDefineValue <<"\"\n";

			    
			  }
			
			else
			  errs() << "ERROR !! Didn't found back macro expansion for "
				 << "the string literal used at line number " << lastRecord->vardecl_linenum << "\n";
	    
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
			
                        if (generation_do_report_modification_in_pc)
                          {
                            rv.insert(std::pair<std::string, std::string>("funcname", function_name));
                            rv.insert(std::pair<std::string, std::string>("execsql", requestExecSql));
                          }

			if (generate_req_headers)
			  {
			    string2_map values_map;
			    values_map["@RequestFunctionName@"] = function_name;
			    values_map["@OriginalSourceFilename@"] = originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1);
			    doRequestHeaderGeneration(diagEngine,
						      generation_header_template,
						      values_map);
			  }
			
			if (generate_req_sources)
			  {
			    string2_map values_map;
			    values_map["@FromRequestName@"] = fromReqName;
			    values_map["@OriginalSourceFilename@"] = originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1);
			    values_map["@FromRequestNameLength@"] = fromReqNameLength;
			    values_map["@RequestDefineName@"] = requestDefineName;
			    values_map["@RequestDefineValue@"] = requestDefineValue;
			    values_map["@RequestFunctionName@"] = function_name;
			    values_map["@RequestExecSql@"] = requestExecSql;
			    doRequestSourceGeneration(diagEngine,
						      generation_source_template,
						      values_map);
			    
			  }
			
			// Now, emit errors, warnings and fixes
			std::string rplt_code = emitDiagAndFix(loc_start, loc_end, function_name);			

                        if (generation_do_report_modification_in_pc)
                          {
                            rv.insert(std::pair<std::string, std::string>("rpltcode", rplt_code));
                            rv.insert(std::pair<std::string, std::string>("originalfile", originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1)));
                            std::ostringstream commentStartLineNum;
                            commentStartLineNum << comment << ":" << startLineNum;
                            replacement_per_comment.insert(std::pair<std::string, std::map<std::string, std::string>>(commentStartLineNum.str(), rv));
                          } 
		      }
		  }
	      }
	    
	    else
	      // Didn't match comment at all!
	      emitError(diagEngine,
			comment_loc_start,
			ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH);
	  }
	else
	  {
	    if (errOccured)
	      // An error occured while accessing memory buffer for sources
	      emitError(diagEngine,
			loc_start,
			ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA);

	    else
	      // We didn't found the comment start ???
	      emitError(diagEngine,
			comment_loc_end,
			ExecSQLPrepareToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START);
	  }
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
