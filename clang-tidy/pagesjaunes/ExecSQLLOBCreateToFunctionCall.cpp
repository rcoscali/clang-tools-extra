//===--- ExecSQLLOBCreateToFunctionCall.cpp - clang-tidy ----------------------------===//
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

#include "ExecSQLLOBCreateToFunctionCall.h"
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

      namespace {
        // Keep track of macro expansions that defines a StringLiteral
        // then keep the associated source_range_set_t to be able to get the
        // original define (for keeping indentation)
        class GetStringLiteralsDefines : public PPCallbacks {
        public:
          explicit GetStringLiteralsDefines(ExecSQLLOBCreateToFunctionCall *parent,
                                            ExecSQLLOBCreateToFunctionCall::source_range_set_t *srset)
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
                        errs() << "*** Token for weird string (wide, utf etc) found\n";
                      }
                  }
                
                if (have_literal)
                  {
                    //outs() << "Adding macro '" << macroName << "' expansion at " << range.getBegin().printToString(m_src_mgr) << "...'\n";
                    ExecSQLLOBCreateToFunctionCall::SourceRangeForStringLiterals *ent
                      = new ExecSQLLOBCreateToFunctionCall::SourceRangeForStringLiterals(range, sr, macroName);
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
          ExecSQLLOBCreateToFunctionCall *m_parent;
          const SourceManager &m_src_mgr; 
          ExecSQLLOBCreateToFunctionCall::source_range_set_t *m_macrosStringLiteralsPtr;
        };
      } // namespace
      
      /**
       * ExecSQLLOBCreateToFunctionCall constructor
       *
       * @brief Constructor for the ExecSQLLOBCreateToFunctionCall rewriting check
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
      ExecSQLLOBCreateToFunctionCall::ExecSQLLOBCreateToFunctionCall(StringRef Name,
                                                                     ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),        /** Init check (super class) */
          TidyContext(Context),                 /** Init our TidyContext instance */
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
                                                 "./pagesjaunes_lob_create_temporary.h.tmpl")),
          /// Generation source default template (string)
          generation_source_template(Options.get("Generation-source-template",
                                                 "./pagesjaunes_lob_create_temporary.pc.tmpl")),
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
                std::vector<std::string> agroup = it->second.get<std::vector<std::string>>();
                auto status = req_groups.emplace(it->first, agroup);
                if (!status.second)
                  errs() << "ERROR!! Couldn't add group '" << it->first
                         << "': a group with same name already exists  in file '"
                         << generation_request_groups << "' !!\n";
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
       * - Generation-request-groups
       *
       * @param Opts    The option map in which to store supported options
       */
      void
      ExecSQLLOBCreateToFunctionCall::storeOptions(ClangTidyOptions::OptionMap &Opts)
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
      ExecSQLLOBCreateToFunctionCall::registerMatchers(MatchFinder *Finder) 
      {
        /* Add a matcher for finding compound statements starting */
        /* with a sqlstm variable declaration */
        Finder->addMatcher(varDecl(
                                   hasAncestor(declStmt(hasAncestor(compoundStmt(hasAncestor(functionDecl().bind("function"))).bind("proCBlock")))),
                                   hasName("sqlstm"))
                           , this);
      }

      /**
       * ExecSQLLOBCreateToFunctionCall::registerPPCallbacks
       *
       * @brief Register callback for intercepting all pre-processor actions
       *
       * Allows to register a callback for executing our actions at every C/C++ 
       * pre-processor processing. Thanks to this callback we will collect all string
       * literal macro expansions.
       *
       * @param[in] compiler    the compiler instance we will intercept
       */
      void ExecSQLLOBCreateToFunctionCall::registerPPCallbacks(CompilerInstance &compiler) {
        compiler
          .getPreprocessor()
          .addPPCallbacks(llvm::make_unique<GetStringLiteralsDefines>(this,
                                                                      &m_macrosStringLiterals));
      }
      
      
      /*
       * ExecSQLLOBCreateToFunctionCall::emitDiagAndFix
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
      ExecSQLLOBCreateToFunctionCall::emitDiagAndFix(const SourceLocation& loc_start,
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
       * ExecSQLLOBCreateToFunctionCall::processTemplate
       *
       * @brief Process a template file with values from the 
       *        provided map
       *
       * @param[in] tmpl        Template file pathname
       * @param[in] fname       Output file pathname
       * @param[in] values_map  Map containing values to be replaced
       *
       * @retval true   if template was processed and file was created
       * @retval false  if something wrong occurs
       */
      bool
      ExecSQLLOBCreateToFunctionCall::processTemplate(const std::string& tmpl,
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
       * ExecSQLLOBCreateToFunctionCall::doRequestSourceGeneration
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
      ExecSQLLOBCreateToFunctionCall::doRequestSourceGeneration(DiagnosticsEngine& diag_engine,
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
                    ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION,
                    &fileName);
      }
      
      /**
       * ExecSQLLOBCreateToFunctionCall::doRequestHeaderGeneration
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
      ExecSQLLOBCreateToFunctionCall::doRequestHeaderGeneration(DiagnosticsEngine& diag_engine,
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
                    ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION,
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
      ExecSQLLOBCreateToFunctionCall::emitError(DiagnosticsEngine &diag_engine,
                                                const SourceLocation& err_loc,
                                                enum ExecSQLLOBCreateToFunctionCallErrorKind kind,
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
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
                             getCustomDiagID(DiagnosticsEngine::Warning,
                                             "Unexpected error occured?!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** No error ID: it should never occur */
          case ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_NO_ERROR:
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
                           getCustomDiagID(DiagnosticsEngine::Ignored,
                                           "No error");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Access char data diag ID */
          case ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA:
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Couldn't access character data in file cache memory buffers!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Can't find a comment */
          case ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START:
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Couldn't find ProC comment start! This result has been discarded!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Cannot match comment */
          case ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH:
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Warning,
			      "Couldn't match ProC comment for function name creation!");
            diag_engine.Report(err_loc, diag_id);
            break;

            /** Cannot generate request source file (no location) */
          case ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_SOURCE_GENERATION:
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Couldn't generate request source file %0!");
            diag_engine.Report(diag_id).AddString(msg);
            break;

            /** Cannot generate request header file (no location) */
          case ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_HEADER_GENERATION:
	    diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Couldn't generate request header file %0!");
            diag_engine.Report(diag_id).AddString(msg);
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
      ExecSQLLOBCreateToFunctionCall::check(const MatchFinder::MatchResult &result) 
      {
        // Get the source manager
        SourceManager &srcMgr = result.Context->getSourceManager();
        DiagnosticsEngine &diagEngine = result.Context->getDiagnostics();

        /*
         * Init check context
         * ------------------
         */
        
        // Get the compound statement AST node as the bounded var 'proCBlock'
        const CompoundStmt *stmt = result.Nodes.getNodeAs<CompoundStmt>("proCBlock");

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

        //outs() << "Found one result at line " << startLineNum << " of file '" << originalSourceFilename << "\n";
        
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

            //outs() << "comment for compound statement at line #" << startLineNum << ": '" << comment << "'\n";

            /*
             * Create function call for the request
             */

            // Regex for processing comment for all remaining requests
            Regex lobCreateReqRe("^.*EXEC SQL[ \t]+(LOB|lob)[ \t]+(CREATE|create)[ \t]+(TEMPORARY|temporary)[ \t]+:([A-Za-z0-9]+);.*$");
            
            // Returned matches
            SmallVector<StringRef, 8> matches;

            /*
             * Now we match against a more permissive regex for all other (simpler) requests
             * =============================================================================
             */
            if (lobCreateReqRe.match(comment, &matches))
              {
                std::string reqAllocName = matches[4];
                std::string requestExecSql = "LOB CREATE TEMPORARY :";
                requestExecSql.append(reqAllocName);
                
                // Append first match (action) to function name
                function_name.append("lobCreateTemporary");

                // Get match in rest string
                std::string rest(reqAllocName);
                // Capitalize it
                rest[0] &= ~0x20;
                // And append to function name
                function_name.append(rest);

                // If headers generation was requested
                if (generate_req_headers)
                  {
                    // Build the map for templating engine
                    string2_map values_map;
                    values_map["@RequestFunctionName@"] = function_name;
                    values_map["@OriginalSourceFilename@"] = originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1);
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
                    values_map["@RequestFunctionName@"] = function_name;
                    values_map["@OriginalSourceFilename@"] = originalSourceFilename.substr(originalSourceFilename.find_last_of("/")+1);
                    values_map["@RequestExecSql@"] = requestExecSql;
                    // And call it
                    doRequestSourceGeneration(diagEngine,
                                              generation_source_template,
                                              values_map);
                  }

                // Emit errors, warnings and fixes
                emitDiagAndFix(loc_start, loc_end, function_name);
              }
            else
              // Didn't match comment at all!
              emitError(diagEngine,
                        comment_loc_start,
                        ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_COMMENT_DONT_MATCH);
          }
        else
          {
            if (errOccured)
              // An error occured while accessing memory buffer for sources
              emitError(diagEngine,
                        loc_start,
                        ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_ACCESS_CHAR_DATA);

            else
              // We didn't found the comment start ???
              emitError(diagEngine,
                        comment_loc_end,
                        ExecSQLLOBCreateToFunctionCall::EXEC_SQL_2_FUNC_ERROR_CANT_FIND_COMMENT_START);
          }
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
