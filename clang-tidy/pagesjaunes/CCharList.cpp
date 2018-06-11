//===--- CCharList.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include "CCharList.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Regex.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#define LOG_OPTIONS 1
#define LOG_MEMBERS_FILE 1
#define LOG_DIAG_N_FIX 1

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;

using empret_t = std::pair<std::map<std::string, std::map<std::string, std::string>>::iterator, bool>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {
      declmap_t CCharList::vardecl_map;
      declocc_t CCharList::vardecl_occmap;
      declmap_t CCharList::arrayvardecl_map;
      declocc_t CCharList::arrayvardecl_occmap;
      declmap_t CCharList::ptrvardecl_map;
      declocc_t CCharList::ptrvardecl_occmap;
      declmap_t CCharList::fielddecl_map;
      declocc_t CCharList::fielddecl_occmap;
      declmap_t CCharList::arrayfielddecl_map;
      declocc_t CCharList::arrayfielddecl_occmap;
      declmap_t CCharList::ptrfielddecl_map;
      declocc_t CCharList::ptrfielddecl_occmap;
      declmap_t CCharList::parmdecl_map;
      declocc_t CCharList::parmdecl_occmap;
      declmap_t CCharList::arrayparmdecl_map;
      declocc_t CCharList::arrayparmdecl_occmap;
      declmap_t CCharList::ptrparmdecl_map;
      declocc_t CCharList::ptrparmdecl_occmap;
      
      /**
       * CCharList constructor
       *
       * @brief Constructor for the CCharList rewriting ClangTidyCheck
       *
       * The rule is created a new check using its \c ClangTidyCheck base class.
       * Name and context are provided and stored locally.
       * Some diag ids corresponding to errors handled by rule are created:
       * - no_error_diag_id: No error
       * - array_type_not_found_diag_id: Didn't found a constant array type
       * - record_decl_not_found_diag_id: Didn't found a RecordDecl (this is a req)
       * - member_has_no_def_diag_id: Didn't found the definition for our type changed member
       * - member_not_found_diag_id: Didn't found the member (unexpected)
       * - member2_not_found_diag_id: Didn't found the second member (unexpected)
       * - unexpected_ast_node_kind_diag_id: Bad node kind detected (unexpected)
       *
       * @param Name    A StringRef for the new check name
       * @param Context The ClangTidyContext allowing to access other contexts
       */
      CCharList::CCharList(StringRef Name,
                           ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          TidyContext(Context),
          file_inclusion_regex(Options.get("File-inclusion-regex", ".*")),
          handle_var_decl(Options.get("Handle-variable-declarations", 1U)),
          handle_field_decl(Options.get("Handle-field-declarations", 0U)),
          allowed_members_file(Options.get("Allowed-members-file", "members.lst")),
          handle_parm_decl(Options.get("Handle-parameter-declarations", 0U)),
          handle_char_decl(Options.get("Handle-char-declarations", 0U)),
          handle_char_array_decl(Options.get("Handle-char-array-declarations", 1U)),
          handle_char_ptr_decl(Options.get("Handle-char-pointer-declarations", 1U)),
          output_csv_file_pathname(Options.get("Result-CSV-file-pathname", "results.csv"))
      {
#if LOG_OPTIONS
        llvm::outs() << "CCharList::CCharList(StringRef Name, ClangTidyContext *Context)\n";
        llvm::outs() << "    File-inclusion-regex = '" << file_inclusion_regex << "'\n";
        llvm::outs() << "    Handle-variable-declarations = " << handle_var_decl << "\n";
        llvm::outs() << "    Handle-field-declarations = " << handle_field_decl << "\n";
        llvm::outs() << "    Handle-parameter-declarations = " << handle_parm_decl << "\n";
        llvm::outs() << "    Handle-char-declarations = " << handle_char_decl << "\n";
        llvm::outs() << "    Handle-char-array-declarations = " << handle_char_array_decl << "\n";
        llvm::outs() << "    Handle-char-pointer-declarations = " << handle_char_ptr_decl << "\n";
        llvm::outs() << "    Result-CSV-file-pathname = " << output_csv_file_pathname << "\n";
        llvm::outs() << "    Allowed-members-file = '" << allowed_members_file << "'\n";
#endif

        // Read the allowed members file
        // This file contains one line for each member with the following syntax:
        // <struct_name>.<member_name>

        try
          {
            readAllowedMembersFile();
            if (m_allowedMembers.size() == 0)
              llvm::errs() << "Warning ! no members allowed for processing: this tool will do nothing !\n";
          }
        catch (std::exception& e)
          {
            llvm::errs() << "Couldn't read the allowed members file: " << allowed_members_file << " \n";
            llvm::errs() << "std::exception: " << e.what() << "\n";
          }
      }


      /**
       * readAllowedMembersFile
       */
      void
      CCharList::readAllowedMembersFile()
      {
#if LOG_MEMBERS_FILE
        llvm::outs() << "Trying to read file: " << allowed_members_file << "\n";
#endif

        std::ifstream src(allowed_members_file.c_str(), std::ios::binary);
        if (src.is_open())
          {
            src.exceptions(std::ifstream::badbit);
            char buf[512];
            while (!src.eof())
              {
                src.getline(buf, 512);
                std::string allowed_member_str(buf);
                if (buf != std::string("") && (src.rdstate() & std::ifstream::failbit ) == 0)
                  {
                    std::string::size_type dotpos = allowed_member_str.find('.');
                    std::pair<std::string , std::string> member;
                    if (dotpos != std::string::npos)
                      member = std::make_pair(allowed_member_str.substr(0, dotpos), allowed_member_str.substr(dotpos + 1));
                    else
                      member = std::make_pair(allowed_member_str, std::string(""));
#if LOG_MEMBERS_FILE
                    llvm::outs() << "Adding allowed " << (dotpos == std::string::npos ? "structure" : "member") << ": " << member.first << ", " << member.second << "\n";
#endif
                    m_allowedMembers.push_back(member);
                  }
              }
            src.close();
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
      CCharList::onStartOfTranslationUnit()
      {
        CCharList::vardecl_map.clear();
        CCharList::vardecl_occmap.clear();
        CCharList::arrayvardecl_map.clear();
        CCharList::arrayvardecl_occmap.clear();
        CCharList::ptrvardecl_map.clear();
        CCharList::ptrvardecl_occmap.clear();
        CCharList::fielddecl_map.clear();
        CCharList::fielddecl_occmap.clear();
        CCharList::arrayfielddecl_map.clear();
        CCharList::arrayfielddecl_occmap.clear();
        CCharList::ptrfielddecl_map.clear();
        CCharList::ptrfielddecl_occmap.clear();
        CCharList::parmdecl_map.clear();
        CCharList::parmdecl_occmap.clear();
        CCharList::arrayparmdecl_map.clear();
        CCharList::arrayparmdecl_occmap.clear();
        CCharList::ptrparmdecl_map.clear();
        CCharList::ptrparmdecl_occmap.clear();
      }
      
      /**
       * onEndOfTranslationUnit
       *
       * @brief called at end of processing of translation unit
       *
       * Override to be called at end of translation unit
       */
      void
      CCharList::onEndOfTranslationUnit()
      {
        unsigned vardecl_numocc = 0;
        unsigned arrayvardecl_numocc = 0;
        unsigned ptrvardecl_numocc = 0;
        unsigned fielddecl_numocc = 0;
        unsigned arrayfielddecl_numocc = 0;
        unsigned ptrfielddecl_numocc = 0;
        unsigned parmdecl_numocc = 0;
        unsigned arrayparmdecl_numocc = 0;
        unsigned ptrparmdecl_numocc = 0;
        
        TranslationUnitDecl *tru = TidyContext->getASTContext()->getTranslationUnitDecl();
        const SourceManager& srcMgr = TidyContext->getASTContext()->getSourceManager();
        std::string filename = srcMgr.getFilename(srcMgr.getSpellingLoc(tru->getLocStart())).str();
        std::filebuf fbo;

        do
          {
            if (!fbo.open(output_csv_file_pathname, std::ios::out))
              break;

	    std::ostream os(&fbo);

#define DUMP_RECORD(condvar, condtype, declmap, occmap, numocc)                 \
        if ((condvar) && (condtype))                                            \
          for (auto it = (declmap).begin(); it != (declmap).end(); it++)        \
            {                                                                   \
              auto key = it->first;                                             \
              auto declmap = it->second;                                        \
              os << "=======================================================\n" \
                 << declmap["kind"] << ","                                      \
                 << declmap["typeName"] << ","                                  \
                 << declmap["varName"] << ","                                   \
                 << declmap["fileName"] << ","                                  \
                 << declmap["line"] << ","                                      \
                 << declmap["column"] << "\n";                                  \
              auto occ = (occmap)[key];                                         \
              (numocc) += occ.size();                                           \
              os << "Occurences," << occ.size() << "\n";                        \
              for (auto itv = occ.begin(); itv != occ.end(); itv++)             \
                {                                                               \
                  os << (*itv)["filename"] << ","                               \
                     << (*itv)["code"] << ","                                   \
                     << (*itv)["line"] << ","                                   \
                     << (*itv)["column"] << "\n";                               \
                }                                                               \
            }
            
            DUMP_RECORD(handle_var_decl, handle_char_decl, vardecl_map, vardecl_occmap, vardecl_numocc);
            DUMP_RECORD(handle_var_decl, handle_char_array_decl, arrayvardecl_map, arrayvardecl_occmap, arrayvardecl_numocc);
            DUMP_RECORD(handle_var_decl, handle_char_ptr_decl, ptrvardecl_map, ptrvardecl_occmap, ptrvardecl_numocc);
            DUMP_RECORD(handle_field_decl, handle_char_decl, fielddecl_map, fielddecl_occmap, fielddecl_numocc);
            DUMP_RECORD(handle_field_decl, handle_char_array_decl, arrayfielddecl_map, arrayfielddecl_occmap, arrayfielddecl_numocc);
            DUMP_RECORD(handle_field_decl, handle_char_ptr_decl, ptrfielddecl_map, ptrfielddecl_occmap, ptrfielddecl_numocc);
            DUMP_RECORD(handle_parm_decl, handle_char_decl, parmdecl_map, parmdecl_occmap, parmdecl_numocc);
            DUMP_RECORD(handle_parm_decl, handle_char_array_decl, arrayparmdecl_map, arrayparmdecl_occmap, arrayparmdecl_numocc);
            DUMP_RECORD(handle_parm_decl, handle_char_ptr_decl, ptrparmdecl_map, ptrparmdecl_occmap, ptrparmdecl_numocc);

#undef DUMP_RECORD

            os << "******************\n"
               << "** STATS RESUME **\n"
               << "******************\n";

            if (handle_var_decl && handle_char_decl)
              os << " ** Number of char variable declarations: " << vardecl_map.size() << "\n"
                 << "     => occurences : " <<  vardecl_numocc << "\n";
            
            if (handle_var_decl && handle_char_array_decl)
              os << " ** Number of char[] variable declarations: " << arrayvardecl_map.size() << "\n"
                 << "     => occurences : " <<  arrayvardecl_numocc << "\n";
            
            if (handle_var_decl && handle_char_ptr_decl)
              os << " ** Number of char * variable declarations: " << ptrvardecl_map.size() << "\n"
                 << "     => occurences : " <<  ptrvardecl_numocc << "\n";
            
            if (handle_field_decl && handle_char_decl)
              os << " ** Number of char field declarations: " << fielddecl_map.size() << "\n"
                 << "     => occurences : " <<  fielddecl_numocc << "\n";
            
            if (handle_field_decl && handle_char_array_decl)
              os << " ** Number of char[] field declarations: " << arrayfielddecl_map.size() << "\n"
                 << "     => occurences : " <<  arrayfielddecl_numocc << "\n";
            
            if (handle_field_decl && handle_char_ptr_decl)
              os << " ** Number of char * field declarations: " << ptrfielddecl_map.size() << "\n"
                 << "     => occurences : " <<  ptrfielddecl_numocc << "\n";
            
            if (handle_parm_decl && handle_char_decl)
              os << " ** Number of char parameter declarations: " << parmdecl_map.size() << "\n"
                 << "     => occurences : " <<  parmdecl_numocc << "\n";
            
            if (handle_parm_decl && handle_char_array_decl)
              os << " ** Number of char[] parameter declarations: " << arrayparmdecl_map.size() << "\n"
                 << "     => occurences : " <<  arrayparmdecl_numocc << "\n";
            
            if (handle_parm_decl && handle_char_ptr_decl)
              os << " ** Number of char * parameter declarations: " << ptrparmdecl_map.size() << "\n"
                 << "     => occurences : " <<  ptrparmdecl_numocc << "\n";

            fbo.close();
          }
        while (0);
      }
      
      /**
       * storeOptions
       *
       * @brief Store options for this check
       *
       * This check support three options for enabling/disabling each
       * call processing:
       *
       * @param Opts    The option map in which to store supported options
       */
      void
      CCharList::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
#if LOG_OPTIONS
        llvm::outs() << "CCharList::storeOptions(ClangTidyOptions::OptionMap &Opts)\n";
#endif
        Options.store(Opts, "File-inclusion-regex", file_inclusion_regex);
        Options.store(Opts, "Handle-variable-declarations", handle_var_decl);
        Options.store(Opts, "Handle-field-declarations", handle_field_decl);
        Options.store(Opts, "Handle-parameter-declarations", handle_parm_decl);
        Options.store(Opts, "Handle-char-declarations", handle_char_decl);
        Options.store(Opts, "Handle-char-array-declarations", handle_char_array_decl);
        Options.store(Opts, "Handle-char-pointer-declarations", handle_char_ptr_decl);
        Options.store(Opts, "Result-CSV-file-pathname", output_csv_file_pathname);

#if LOG_OPTIONS
        llvm::outs() << "    File-inclusion-regex = '" << file_inclusion_regex << "'\n";        
        llvm::outs() << "    Handle-variable-declarations = " << handle_var_decl << "\n";
        llvm::outs() << "    Handle-field-declarations = " << handle_field_decl << "\n";
        llvm::outs() << "    Allowed-members-file = " << allowed_members_file << "\n";        
        llvm::outs() << "    Handle-parameter-declarations = " << handle_parm_decl << "\n";
        llvm::outs() << "    Handle-char-declarations = " << handle_char_decl << "\n";
        llvm::outs() << "    Handle-char-array-declarations = " << handle_char_array_decl << "\n";
        llvm::outs() << "    Handle-char-pointer-declarations = " << handle_char_ptr_decl << "\n";
        llvm::outs() << "    Result-CSV-file-pathname = " << output_csv_file_pathname << "\n";
#endif
      }

      /**
       * registerMatchers
       *
       * @brief Register the ASTMatchers that will found nodes we are interested in
       *
       * This method register 3 matchers for each string manipulation calls we want 
       * to rewrite. These three calls are:
       * - strcmp
       * - strcpy
       * - strlen
       * The matchers bind elements we will use, for detecting we want to rewrite the
       * foudn statement, and for writing new code.
       *
       * @param Finder  the recursive visitor that will use our matcher for sending 
       *                us AST node.
       */
      void 
      CCharList::registerMatchers(MatchFinder *Finder) 
      {
        // VarDecl, FieldDecl, ParmVarDecl (FIXME: TypedefDecl??, FunctionDecl ??)
        Finder->addMatcher(varDecl(hasType(constantArrayType(hasElementType(builtinType(),
                                                                            isAnyCharacter())))).bind("charArrayVarDecl"), this);
        Finder->addMatcher(varDecl(hasType(isAnyCharacter())).bind("charVarDecl"), this);
        Finder->addMatcher(varDecl(hasType(pointerType(pointee(isAnyCharacter())))).bind("charPtrVarDecl"), this);

        Finder->addMatcher(fieldDecl(hasType(constantArrayType(hasElementType(builtinType(),
                                                                              isAnyCharacter())))).bind("charArrayFieldDecl"), this);
        Finder->addMatcher(fieldDecl(hasType(isAnyCharacter())).bind("charFieldDecl"), this);
        Finder->addMatcher(fieldDecl(hasType(pointerType(pointee(isAnyCharacter())))).bind("charPtrFieldDecl"), this);

        Finder->addMatcher(parmVarDecl(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                isAnyCharacter())))).bind("charArrayParmVarDecl"), this);
        Finder->addMatcher(parmVarDecl(hasType(isAnyCharacter())).bind("charParmVarDecl"), this);                                    
        Finder->addMatcher(parmVarDecl(hasType(pointerType(pointee(isAnyCharacter())))).bind("charPtrParmVarDecl"), this);

      }

      /*
       * emitDiagAndFix
       *
       * @brief Emit a diagnostic message and possible replacement fix for each
       *        statement we will be notified with.
       *
       * This method is called each time a statement to handle (rewrite) is found.
       * Two replacement will be emited for each node found:
       * - one for the method call statement (CallExpr)
       * - one for the member definition (changed type to std::string)
       *
       * It is passed all necessary arguments for:
       * - creating a comprehensive diagnostic message
       * - computing the locations of code we will replace
       * - computing the new code that will replace old one
       *
       * @param src_mgr         The source manager
       * @param call_start      The CallExpr start location
       * @param call_end        The CallExpr end location
       * @param function_name   The function name that is called, that we need to rewrite
       * @param member_tokens   The first member tokens
       * @param member2_tokens  The second member tokens (can be empty for strlen)
       * @param member3_tokens  The third member tokens (contain size for strncpy and strncmp)
       * @param def_start       The member definition start
       * @param def_end         The member definition end
       * @param member_name     The member name we will change the stype (using std::string)
       * @param object_name     The object name holding the member we will change type
       * @param field_name      The structure (object) field name
       * @param field_tokens    The structure (object) field tokens
       */
      void
      CCharList::emitDiagAndFix(DiagnosticsEngine &diag_engine,
                                const SourceLocation& decl_loc,
                                std::string& name)
      {
        // Name of the function
        std::string function_name;
        
        // Write new code for definition: std::string + member tokens
        std::string replt_4def = formatv("std::string {0}", name).str();

        /* 
         * !!{ ============================================================ }
         * !!{ I put the diagnostic builder in its own block because actual }
         * !!{ report is launched at destruction. As the destructor is not  }
         * !!{ available (= delete), this is the only way to achieve this.  }
         * !!{ ============================================================ }
         */

        // First report
        {
          // Create diag msg for the call expr rewrite
          DiagnosticBuilder mydiag = diag(decl_loc,
                                          "This call to '%0' shall be replaced with std::string '%1' equivalent")
            << function_name << replt_4def;
          // Emit replacement for call expr
          mydiag << FixItHint::CreateReplacement(decl_loc, replt_4def);
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
       * @param msg             Additional message for error positional params
       */
      void
      CCharList::emitError(DiagnosticsEngine &diag_engine,
                           const SourceLocation& err_loc,
                           enum CCharListErrorKind kind,
                           std::string *msg)
      {
        // According to the kind of error
        unsigned diag_id;
        switch (kind)
          {
          default:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Warning,
                              "Unexpected error occured?!");
            break;

          case CCharList::CCHAR_LIST_ERROR_NO_ERROR:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Ignored,
                              "No error");
            break;

          case CCharList::CCHAR_LIST_ERROR_ARRAY_TYPE_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Constant Array type was not found!");
            break;
            
          case CCharList::CCHAR_LIST_ERROR_RECORD_DECL_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Could not bind the Structure Access expression!");
            break;

          case CCharList::CCHAR_LIST_ERROR_MEMBER_HAS_NO_DEF:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Member has no definition!");
            break;

          case CCharList::CCHAR_LIST_ERROR_MEMBER_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Could not bind the member expression!");
            break;

          case CCharList::CCHAR_LIST_ERROR_MEMBER2_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Could not bind the second member expression!");
            break;
            
          case CCharList::CCHAR_LIST_ERROR_UNEXPECTED_AST_NODE_KIND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
              getCustomDiagID(DiagnosticsEngine::Error,
                              "Could not process member owning record kind!");
            break;
          }
        // Alloc the diag builder
        DiagnosticBuilder diag = diag_engine.Report(err_loc, diag_id);
        // If an extra message was provided, add it to diag id message
        if (msg && !msg->empty())
          diag.AddString(*msg);
      }

      /**
       *
       */
      occmap_t
      CCharList::searchOccurencesVarDecl(SourceManager &src_mgr, std::string& name)
      {
        occmap_t res;
        ClangTool *tool = TidyContext->getToolPtr();
        StatementMatcher decloccMatcher =
          declRefExpr(hasDeclaration(varDecl(hasType(isAnyCharacter()),hasName(name.c_str())).bind("vardecl"))).bind("declref");
        CCharList::FindDeclOccurenceMatcher fdoMatcher(this);
        MatchFinder finder;
        finder.addMatcher(decloccMatcher, &fdoMatcher);
        m_varDeclOccCollector.clear();
        tool->run(newFrontendActionFactory(&finder).get());
        if (!m_varDeclOccCollector.empty())
          {
            for (auto it = m_varDeclOccCollector.begin(); it != m_varDeclOccCollector.end(); it++)
              {
                std::map<std::string, std::string> entry;
                struct VarDeclOccurence *record = *it;
                SourceLocation locbegin = src_mgr.getSpellingLoc(record->codeRange->getBegin());
                SourceLocation locend = src_mgr.getSpellingLoc(record->codeRange->getEnd());
                char * start = const_cast<char *>(src_mgr.getCharacterData(locbegin));
                char * linestart = nullptr;
                if (start)
                  {
                    char *p = start;
                    // let's take start just after previous end of line
                    while (p && *p != '\n')
                      p--;
                    linestart = ++p;
                  }
                char * end = const_cast<char *>(src_mgr.getCharacterData(locend));
                if (start == end)
                  {
                    char *p = end;
                    // let's take end just before first end of line
                    while (p && *p != '\n')
                      p++;
                    end = p;
                  }
                std::string code(linestart, end - linestart);
                std::string::size_type backslash_pos = 0;
                while ((backslash_pos = code.find('\\', backslash_pos)) != std::string::npos)
                  {
                    code.insert(backslash_pos, 1, '\\');
                    backslash_pos += 2;
                  }
                std::string::size_type dblquote_pos = 0;
                while ((dblquote_pos = code.find('"', dblquote_pos)) != std::string::npos)
                  {
                    code.insert(dblquote_pos, 1, '\\');
                    dblquote_pos += 2;
                  }
                code.insert(0, 1, '"');
                code.append("\"");
                std::string filename = src_mgr.getFilename(locbegin).str();
                unsigned foff = src_mgr.getFileOffset(locbegin);
                FileID fid = src_mgr.getFileID(locbegin);
                std::stringbuf lnbuffer;
                std::stringbuf cnbuffer;
                std::ostream lnos (&lnbuffer);
                std::ostream cnos (&cnbuffer);
                unsigned linenum = src_mgr.getLineNumber(fid, foff);
                unsigned colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                cnos << colnum;
                entry.emplace("code", code);
                entry.emplace("filename", src_mgr.getFilename(locbegin).str());
                entry.emplace("line", lnbuffer.str());
                entry.emplace("column", cnbuffer.str());
                res.push_back(entry);
              }
          }
        return res;
      }

      /**
       *
       */
      occmap_t
      CCharList::searchOccurencesArrayVarDecl(SourceManager &src_mgr, std::string& name)
      {
        occmap_t res;
        ClangTool *tool = TidyContext->getToolPtr();
        StatementMatcher decloccMatcher =
          declRefExpr(hasDeclaration(varDecl(hasType(constantArrayType(hasElementType(builtinType(),isAnyCharacter()))),hasName(name.c_str())).bind("vardecl"))).bind("declref");
        FindDeclOccurenceMatcher fdoMatcher(this);
        MatchFinder finder;
        finder.addMatcher(decloccMatcher, &fdoMatcher);
        m_varDeclOccCollector.clear();
        tool->run(newFrontendActionFactory(&finder).get());
        if (!m_varDeclOccCollector.empty())
          {
            for (auto it = m_varDeclOccCollector.begin(); it != m_varDeclOccCollector.end(); it++)
              {
                std::map<std::string, std::string> entry;
                struct VarDeclOccurence *record = *it;
                SourceLocation locbegin = src_mgr.getSpellingLoc(record->codeRange->getBegin());
                SourceLocation locend = src_mgr.getSpellingLoc(record->codeRange->getEnd());
                char * start = const_cast<char *>(src_mgr.getCharacterData(locbegin)); 
                char * linestart = nullptr;
                if (start)
                  {
                    char *p = start;
                    // let's take start just after previous end of line
                    while (p && *p != '\n')
                      p--;
                    linestart = ++p;
                  }
                char * end = const_cast<char *>(src_mgr.getCharacterData(locend));
                if (start == end)
                  {
                    char *p = end;
                    // let's take end just before first end of line
                    while (p && *p != '\n')
                      p++;
                    end = p;
                  }
                std::string code(linestart, end - linestart);
                std::string::size_type backslash_pos = 0;
                while ((backslash_pos = code.find('\\', backslash_pos)) != std::string::npos)
                  {
                    code.insert(backslash_pos, 1, '\\');
                    backslash_pos += 2;
                  }
                std::string::size_type dblquote_pos = 0;
                while ((dblquote_pos = code.find('"', dblquote_pos)) != std::string::npos)
                  {
                    code.insert(dblquote_pos, 1, '\\');
                    dblquote_pos += 2;
                  }
                code.insert(0, 1, '"');
                code.append("\"");
                std::string filename = src_mgr.getFilename(locbegin).str();
                unsigned foff = src_mgr.getFileOffset(locbegin);
                FileID fid = src_mgr.getFileID(locbegin);
                std::stringbuf lnbuffer;
                std::stringbuf cnbuffer;
                std::ostream lnos (&lnbuffer);
                std::ostream cnos (&cnbuffer);
                unsigned linenum = src_mgr.getLineNumber(fid, foff);
                unsigned colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                cnos << colnum;
                entry.emplace("code", code);
                entry.emplace("filename", src_mgr.getFilename(locbegin).str());
                entry.emplace("line", lnbuffer.str());
                entry.emplace("column", cnbuffer.str());
                res.push_back(entry);
              }
          }
        return res;
      }

      /**
       *
       */
      occmap_t
      CCharList::searchOccurencesPtrVarDecl(SourceManager &src_mgr, std::string& name)
      {
        occmap_t res;
        ClangTool *tool = TidyContext->getToolPtr();
        StatementMatcher decloccMatcher =
          declRefExpr(hasDeclaration(varDecl(hasType(pointerType(pointee(isAnyCharacter()))),namedDecl(hasName(name.c_str()))).bind("vardecl"))).bind("declref");
        FindDeclOccurenceMatcher fdoMatcher(this);
        MatchFinder finder;
        finder.addMatcher(decloccMatcher, &fdoMatcher);
        m_varDeclOccCollector.clear();
        tool->run(newFrontendActionFactory(&finder).get());
        if (!m_varDeclOccCollector.empty())
          {
            for (auto it = m_varDeclOccCollector.begin(); it != m_varDeclOccCollector.end(); it++)
              {
                std::map<std::string, std::string> entry;
                struct VarDeclOccurence *record = *it;
                SourceLocation locbegin = src_mgr.getSpellingLoc(record->codeRange->getBegin());
                SourceLocation locend = src_mgr.getSpellingLoc(record->codeRange->getEnd());
                char * start = const_cast<char *>(src_mgr.getCharacterData(locbegin)); 
                char * linestart = nullptr;
                if (start)
                  {
                    char *p = start;
                    // let's take start just after previous end of line
                    while (p && *p != '\n')
                      p--;
                    linestart = ++p;
                  }
                char * end = const_cast<char *>(src_mgr.getCharacterData(locend));
                if (start == end)
                  {
                    char *p = end;
                    // let's take end just before first end of line
                    while (p && *p != '\n')
                      p++;
                    end = p;
                  }
                std::string code(linestart, end - linestart);
                std::string::size_type backslash_pos = 0;
                while ((backslash_pos = code.find('\\', backslash_pos)) != std::string::npos)
                  {
                    code.insert(backslash_pos, 1, '\\');
                    backslash_pos += 2;
                  }
                std::string::size_type dblquote_pos = 0;
                while ((dblquote_pos = code.find('"', dblquote_pos)) != std::string::npos)
                  {
                    code.insert(dblquote_pos, 1, '\\');
                    dblquote_pos += 2;
                  }
                code.insert(0, 1, '"');
                code.append("\"");
                std::string filename = src_mgr.getFilename(locbegin).str();
                unsigned foff = src_mgr.getFileOffset(locbegin);
                FileID fid = src_mgr.getFileID(locbegin);
                std::stringbuf lnbuffer;
                std::stringbuf cnbuffer;
                std::ostream lnos (&lnbuffer);
                std::ostream cnos (&cnbuffer);
                unsigned linenum = src_mgr.getLineNumber(fid, foff);
                unsigned colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                cnos << colnum;
                entry.emplace("code", code);
                entry.emplace("filename", src_mgr.getFilename(locbegin).str());
                entry.emplace("line", lnbuffer.str());
                entry.emplace("column", cnbuffer.str());
                res.push_back(entry);
              }
          }
        return res;
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
       *   location and code
       * It handles rewriting three string manipulation functions:
       * - strcmp
       * - strcpy
       * - strlen
       *
       * @param result          The match result provided by the recursive visitor
       *                        allowing us to access AST nodes bound to variables
       */
      void
      CCharList::check(const MatchFinder::MatchResult &result) 
      {
        // Get root bounded variables
        const VarDecl *vardecl = (const VarDecl *) result.Nodes.getNodeAs<VarDecl>("charVarDecl");
        const VarDecl *arrayvardecl = (const VarDecl *) result.Nodes.getNodeAs<VarDecl>("charArrayVarDecl");
        const VarDecl *ptrvardecl = (const VarDecl *) result.Nodes.getNodeAs<VarDecl>("charPtrVarDecl");
        const FieldDecl *fielddecl = (const FieldDecl *) result.Nodes.getNodeAs<FieldDecl>("charFieldDecl");
        const FieldDecl *arrayfielddecl = (const FieldDecl *) result.Nodes.getNodeAs<FieldDecl>("charArrayFieldDecl");
        const FieldDecl *ptrfielddecl = (const FieldDecl *) result.Nodes.getNodeAs<FieldDecl>("charPtrFieldDecl");
        const ParmVarDecl *parmdecl = (const ParmVarDecl *) result.Nodes.getNodeAs<ParmVarDecl>("charParmVarDecl");
        const ParmVarDecl *arrayparmdecl = (const ParmVarDecl *) result.Nodes.getNodeAs<ParmVarDecl>("charArrayParmVarDecl");
        const ParmVarDecl *ptrparmdecl = (const ParmVarDecl *) result.Nodes.getNodeAs<ParmVarDecl>("charPtrParmVarDecl");

        // Get the source manager
        SourceManager &src_mgr = result.Context->getSourceManager();
        // And diagnostics engine
        //DiagnosticsEngine &diag_engine = result.Context->getDiagnostics();

        SourceRange decl_loc_range;
        SourceLocation decl_loc;
        std::string member_name;
        std::string object_name;
        std::string varName;
        std::string typeName;
        QualType qtype;
        //bool do_out = false;
        std::string declkind;
        
        std::string entry_key;
        std::string loc_name;
        std::string filename;
        unsigned foff;
        FileID fid;
        unsigned linenum;
        unsigned colnum;
        std::stringbuf lnbuffer;
        std::stringbuf cnbuffer;
        std::ostream lnos (&lnbuffer);
        std::ostream cnos (&cnbuffer);
        std::string linenumstr;
        std::string colnumstr;
        Regex file_inclusion(file_inclusion_regex);
        SmallVector<StringRef, 8> fileInclMatches;

        if (vardecl && handle_var_decl && handle_char_decl)
          {
            std::map<std::string, std::string> vardecl_entry;
            
            decl_loc_range = vardecl->getSourceRange();
            SourceLocation tmp_decl_loc = vardecl->getLocation();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();
                
                if (isa<VarDecl>(*vardecl))
                  {
                    qtype = vardecl->getType();
                    SplitQualType qt_split = qtype.split();
                    typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                  }
                if (isa<NamedDecl>(*vardecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(vardecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";
                
                declkind = "VarDecl";
                vardecl_entry.emplace("kind", declkind);
                vardecl_entry.emplace("typeName", typeName);
                vardecl_entry.emplace("varName", varName);
                vardecl_entry.emplace("fileName", filename);
                vardecl_entry.emplace("line", linenumstr);
                vardecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    vardecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));
                    (void)vardecl_map.emplace(entry_key, vardecl_entry);
                  }
              }
          }
        else if (arrayvardecl && handle_var_decl && handle_char_array_decl)
          {
            std::map<std::string, std::string> arrayvardecl_entry;
            
            decl_loc_range = arrayvardecl->getSourceRange();
            SourceLocation tmp_decl_loc = arrayvardecl->getLocation();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                if (isa<VarDecl>(*arrayvardecl))
                  {
                    qtype = arrayvardecl->getType();
                    SplitQualType qt_split = qtype.split();
                    typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                  }
                if (isa<NamedDecl>(*arrayvardecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(arrayvardecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                declkind = "ArrayVarDecl";
                arrayvardecl_entry.emplace("kind", declkind);
                arrayvardecl_entry.emplace("typeName", typeName);
                arrayvardecl_entry.emplace("varName", varName);
                arrayvardecl_entry.emplace("fileName", filename);
                arrayvardecl_entry.emplace("line", linenumstr);
                arrayvardecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    arrayvardecl_occmap.emplace(entry_key, searchOccurencesArrayVarDecl(src_mgr, varName));
                    (void)arrayvardecl_map.emplace(entry_key, arrayvardecl_entry);
                  }
              }
          }
        else if (ptrvardecl && handle_var_decl && handle_char_ptr_decl)
          {
            std::map<std::string, std::string> ptrvardecl_entry;
            
            decl_loc_range = ptrvardecl->getSourceRange();
            SourceLocation tmp_decl_loc = ptrvardecl->getLocation();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                if (isa<VarDecl>(*ptrvardecl))
                  {
                    qtype = ptrvardecl->getType();
                    SplitQualType qt_split = qtype.split();
                    typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                  }
                if (isa<NamedDecl>(*ptrvardecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(ptrvardecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                declkind = "PtrVarDecl";
                ptrvardecl_entry.emplace("kind", declkind);
                ptrvardecl_entry.emplace("typeName", typeName);
                ptrvardecl_entry.emplace("varName", varName);
                ptrvardecl_entry.emplace("fileName", filename);
                ptrvardecl_entry.emplace("line", linenumstr);
                ptrvardecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    ptrvardecl_occmap.emplace(entry_key, searchOccurencesPtrVarDecl(src_mgr, varName));
                    (void)ptrvardecl_map.emplace(entry_key, ptrvardecl_entry);
                  }
              }
          }
        else if (fielddecl && handle_field_decl && handle_char_decl)
          {
            std::map<std::string, std::string> fielddecl_entry;
            
            decl_loc_range = fielddecl->getSourceRange();
            SourceLocation tmp_decl_loc = fielddecl->getLocation();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                if (isa<ValueDecl>(*fielddecl))
                  {
                    const ValueDecl *valuedecl = dyn_cast<ValueDecl>(fielddecl);
                    qtype = valuedecl->getType();
                    SplitQualType qt_split = qtype.split();
                    typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                  }
                if (isa<NamedDecl>(*fielddecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(fielddecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                object_name = typeName;
                member_name = varName;                

                declkind = "FieldDecl";
                fielddecl_entry.emplace("kind", declkind);
                fielddecl_entry.emplace("typeName", typeName);
                fielddecl_entry.emplace("varName", varName);
                fielddecl_entry.emplace("fileName", filename);
                fielddecl_entry.emplace("line", linenumstr);
                fielddecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
#if LOG_DIAG_N_FIX
                    // Get the source manager
                    const SourceManager& src_mgr = TidyContext->getASTContext()->getSourceManager();
                    std::string the_filename;
                    unsigned the_linenum;
                    
                    {
                      the_filename = src_mgr.getFilename(decl_loc).str();
                      unsigned foff = src_mgr.getFileOffset(decl_loc);
                      FileID fid = src_mgr.getFileID(decl_loc);
                      the_linenum = src_mgr.getLineNumber(fid, foff);
                    }
#endif
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    fielddecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));

                    // Check if struct/member is allowed for transformation
                    bool allowed = false;
#if LOG_DIAG_FIX
                    llvm::outs() << "Start testing if allowed: " << typeName << ", " << varName << "\n";
#endif

                    for (auto it = m_allowedMembers.begin(); it != m_allowedMembers.end(); it++)
                      {
                        std::string allowed_struct = it->first;
                        std::string allowed_member = it->second;
                        std::string allowed_struct_struct("struct ");
                        allowed_struct_struct.append(allowed_struct);
                        std::string allowed_struct_ptr(allowed_struct);
                        allowed_struct_ptr.append(" *");
                        std::string allowed_struct_struct_ptr(allowed_struct_struct);
                        allowed_struct_struct_ptr.append(" *");
                        
#if LOG_DIAG_N_FIX
                        llvm::outs() << "Testing versus: " << allowed_struct << ", " << allowed_struct_struct << ", " << allowed_member << "\n";
                        llvm::outs() << "Testing versus: " << allowed_struct_ptr << ", " << allowed_struct_struct_ptr << ", " << allowed_member << "\n";
#endif
                        if (allowed_struct.compare("*") == 0 && allowed_member.empty())
                          {
                            allowed = true;
                            break;
                          }
                        else
                          {
                            if (allowed_member.empty() || allowed_member.compare("*") == 0)
                              allowed = (allowed_struct == object_name ||
                                         allowed_struct_ptr == object_name ||
                                         allowed_struct_struct == object_name ||
                                         allowed_struct_struct_ptr == object_name);
                            else
                              allowed = (allowed_struct == object_name ||
                                         allowed_struct_ptr == object_name ||
                                         allowed_struct_struct == object_name ||
                                         allowed_struct_struct_ptr == object_name) &&
                                member_name == allowed_member;
                        if (allowed)
                          break;
                      }

#if LOG_DIAG_N_FIX
                        llvm::outs() << "Struct/Member: " << object_name << ", " << member_name << " is " << (allowed?"" : "not ") << "Allowed\n";
#endif
                    
                    if (allowed)
                      (void)fielddecl_map.emplace(entry_key, fielddecl_entry);
                  }
              }
          }
        else if (arrayfielddecl && handle_field_decl && handle_char_array_decl)
          {
            std::map<std::string, std::string> arrayfielddecl_entry;

            decl_loc_range = arrayfielddecl->getSourceRange();
            SourceLocation tmp_decl_loc = arrayfielddecl->getLocation();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                if (isa<ValueDecl>(*arrayfielddecl))
                  {
                    const ValueDecl *valuedecl = dyn_cast<ValueDecl>(arrayfielddecl);
                    qtype = valuedecl->getType();
                    SplitQualType qt_split = qtype.split();
                    typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                  }
                if (isa<NamedDecl>(*arrayfielddecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(arrayfielddecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                object_name = typeName;
                member_name = varName;
                
                declkind = "ArrayFieldDecl";
                arrayfielddecl_entry.emplace("kind", declkind);
                arrayfielddecl_entry.emplace("typeName", typeName);
                arrayfielddecl_entry.emplace("varName", varName);
                arrayfielddecl_entry.emplace("fileName", filename);
                arrayfielddecl_entry.emplace("line", linenumstr);
                arrayfielddecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    arrayfielddecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));
                    
                    // Check if struct/member is allowed for transformation
                    bool allowed = false;
#if LOG_DIAG_N_FIX
                    llvm::outs() << "Start testing if allowed: " << typeName << ", " << varName << "\n";
#endif
                    for (auto it = m_allowedMembers.begin(); it != m_allowedMembers.end(); it++)
                      {
                        std::string allowed_struct = it->first;
                        std::string allowed_member = it->second;
                        std::string allowed_struct_struct("struct ");
                        allowed_struct_struct.append(allowed_struct);
                        std::string allowed_struct_ptr(allowed_struct);
                        allowed_struct_ptr.append(" *");
                        std::string allowed_struct_struct_ptr(allowed_struct_struct);
                        allowed_struct_struct_ptr.append(" *");
                        
#if LOG_DIAG_N_FIX
                        llvm::outs() << "Testing versus: " << allowed_struct << ", " << allowed_struct_struct << ", " << allowed_member << "\n";
                        llvm::outs() << "Testing versus: " << allowed_struct_ptr << ", " << allowed_struct_struct_ptr << ", " << allowed_member << "\n";
#endif
                        if (allowed_struct.compare("*") == 0 && allowed_member.empty())
                          {
                            allowed = true;
                            break;
                          }
                        else
                          {
                            if (allowed_member.empty() || allowed_member.compare("*") == 0)
                              allowed = (allowed_struct == object_name ||
                                         allowed_struct_ptr == object_name ||
                                         allowed_struct_struct == object_name ||
                                         allowed_struct_struct_ptr == object_name);
                            else
                              allowed = (allowed_struct == object_name ||
                                         allowed_struct_ptr == object_name ||
                                         allowed_struct_struct == object_name ||
                                         allowed_struct_struct_ptr == object_name) &&
                                member_name == allowed_member;
                            
                            if (allowed)
                              break;
                          }
                      }

#if LOG_DIAG_N_FIX
                    llvm::outs() << "Struct/Member: " << object_name << ", " << member_name << " is " << (allowed?"" : "not ") << "Allowed\n";
#endif
                    
                    if (allowed)
                      (void)arrayfielddecl_map.emplace(entry_key, arrayfielddecl_entry);
                  }
              }
          }
        else if (ptrfielddecl && handle_field_decl && handle_char_ptr_decl)
          {
            std::map<std::string, std::string> ptrfielddecl_entry;
            
            decl_loc_range = ptrfielddecl->getSourceRange();
            SourceLocation tmp_decl_loc = ptrfielddecl->getLocation();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                if (isa<ValueDecl>(*ptrfielddecl))
                  {
                    const ValueDecl *valuedecl = dyn_cast<ValueDecl>(ptrfielddecl);
                    qtype = valuedecl->getType();
                    SplitQualType qt_split = qtype.split();
                    typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                  }
                if (isa<NamedDecl>(*ptrfielddecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(ptrfielddecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                object_name = typeName;
                member_name = varName;

                declkind = "PtrFieldDecl";
                ptrfielddecl_entry.emplace("kind", declkind);
                ptrfielddecl_entry.emplace("typeName", typeName);
                ptrfielddecl_entry.emplace("varName", varName);
                ptrfielddecl_entry.emplace("fileName", filename);
                ptrfielddecl_entry.emplace("line", linenumstr);
                ptrfielddecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    ptrfielddecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));
                    
                    // Check if struct/member is allowed for transformation
                    bool allowed = false;

#if LOG_DIAG_N_FIX
                    llvm::outs() << "Start testing if allowed: " << object_name << ", " << member_name << "\n";
#endif
        
                    for (auto it = m_allowedMembers.begin(); it != m_allowedMembers.end(); it++)
                      {
                        std::string allowed_struct = it->first;
                        std::string allowed_member = it->second;
                        std::string allowed_struct_struct("struct ");
                        allowed_struct_struct.append(allowed_struct);
                        std::string allowed_struct_ptr(allowed_struct);
                        allowed_struct_ptr.append(" *");
                        std::string allowed_struct_struct_ptr(allowed_struct_struct);
                        allowed_struct_struct_ptr.append(" *");
                        
#if LOG_DIAG_N_FIX
                        llvm::outs() << "Testing versus: " << allowed_struct << ", " << allowed_struct_struct << ", " << allowed_member << "\n";
                        llvm::outs() << "Testing versus: " << allowed_struct_ptr << ", " << allowed_struct_struct_ptr << ", " << allowed_member << "\n";
#endif
                        if (allowed_struct.compare("*") == 0 && allowed_member.empty())
                          {
                            allowed = true;
                            break;
                          }
                        else
                          {
                            if (allowed_member.empty() || allowed_member.compare("*") == 0)
                              allowed = (allowed_struct == object_name ||
                                         allowed_struct_ptr == object_name ||
                                         allowed_struct_struct == object_name ||
                                         allowed_struct_struct_ptr == object_name);
                            else
                              allowed = (allowed_struct == object_name ||
                                         allowed_struct_ptr == object_name ||
                                         allowed_struct_struct == object_name ||
                                         allowed_struct_struct_ptr == object_name) &&
                                member_name == allowed_member;
                            
                            if (allowed)
                              break;
                          }
                      }
                    
#if LOG_DIAG_N_FIX
                    llvm::outs() << "Struct/Member: " << object_name << ", " << member_name << " is " << (allowed?"" : "not ") << "Allowed\n";
#endif
        
                    if (allowed)
                      (void)ptrfielddecl_map.emplace(entry_key, ptrfielddecl_entry);
                  }
              }
          }
        else if (parmdecl && handle_parm_decl && handle_char_decl)
          {
            std::map<std::string, std::string> parmdecl_entry;
            
            decl_loc_range = parmdecl->getSourceRange();
            SourceLocation tmp_decl_loc = parmdecl->getLocStart();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                qtype = parmdecl->getOriginalType();
                SplitQualType qt_split = qtype.split();
                typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                if (isa<NamedDecl>(*parmdecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(parmdecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                declkind = "ParmDecl";
                parmdecl_entry.emplace("kind", declkind);
                parmdecl_entry.emplace("typeName", typeName);
                parmdecl_entry.emplace("varName", varName);
                parmdecl_entry.emplace("fileName", filename);
                parmdecl_entry.emplace("line", linenumstr);
                parmdecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    parmdecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));
                    (void)parmdecl_map.emplace(entry_key, parmdecl_entry);
                    if (vardecl_map.find(entry_key) != vardecl_map.end())
                      vardecl_map.erase(entry_key);
                  }
              }
          }
        else if (arrayparmdecl && handle_parm_decl && handle_char_array_decl)
          {
            std::map<std::string, std::string> arrayparmdecl_entry;

            decl_loc_range = arrayparmdecl->getSourceRange();
            SourceLocation tmp_decl_loc = arrayparmdecl->getLocStart();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                qtype = arrayparmdecl->getOriginalType();
                SplitQualType qt_split = qtype.split();
                typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                if (isa<NamedDecl>(*arrayparmdecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(arrayparmdecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                declkind = "ArrayParmDecl";
                arrayparmdecl_entry.emplace("kind", declkind);
                arrayparmdecl_entry.emplace("typeName", typeName);
                arrayparmdecl_entry.emplace("varName", varName);
                arrayparmdecl_entry.emplace("fileName", filename);
                arrayparmdecl_entry.emplace("line", linenumstr);
                arrayparmdecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    arrayparmdecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));
                    (void)arrayparmdecl_map.emplace(entry_key, arrayparmdecl_entry);
                    if (arrayvardecl_map.find(entry_key) != arrayvardecl_map.end())
                      arrayvardecl_map.erase(entry_key);
                  }
              }
          }
        else if (ptrparmdecl && handle_parm_decl && handle_char_ptr_decl)
          {
            std::map<std::string, std::string> ptrparmdecl_entry;
            
            decl_loc_range = ptrparmdecl->getSourceRange();
            SourceLocation tmp_decl_loc = ptrparmdecl->getLocStart();
            decl_loc = src_mgr.getSpellingLoc(tmp_decl_loc);
            loc_name = decl_loc.printToString(src_mgr);
            filename = src_mgr.getFilename(decl_loc).str();
            if (file_inclusion.match(filename, &fileInclMatches))
              {
                foff = src_mgr.getFileOffset(decl_loc);
                fid = src_mgr.getFileID(decl_loc);
                linenum = src_mgr.getLineNumber(fid, foff);
                colnum = src_mgr.getColumnNumber(fid, foff);
                lnos << linenum;
                linenumstr = lnbuffer.str();
                cnos << colnum;
                colnumstr = cnbuffer.str();

                qtype = ptrparmdecl->getOriginalType();
                SplitQualType qt_split = qtype.split();
                typeName = QualType::getAsString(qt_split, TidyContext->getASTContext()->getPrintingPolicy());
                if (isa<NamedDecl>(*ptrparmdecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(ptrparmdecl);
                    varName = namedDecl->getNameAsString();
                  }
                else
                  varName = "<unknown>";

                declkind = "PtrParmDecl";
                ptrparmdecl_entry.emplace("kind", declkind);
                ptrparmdecl_entry.emplace("typeName", typeName);
                ptrparmdecl_entry.emplace("varName", varName);
                ptrparmdecl_entry.emplace("fileName", filename);
                ptrparmdecl_entry.emplace("line", linenumstr);
                ptrparmdecl_entry.emplace("column", colnumstr);
                if (!varName.empty())
                  {
                    entry_key.assign(varName);
                    entry_key.append("|");
                    entry_key.append(typeName);
                    entry_key.append("|");
                    entry_key.append(filename);
                    entry_key.append("|");
                    entry_key.append(linenumstr);
                    //do_out = true;
                    ptrparmdecl_occmap.emplace(entry_key, searchOccurencesVarDecl(src_mgr, varName));
                    (void)ptrparmdecl_map.emplace(entry_key, ptrparmdecl_entry);
                    if (ptrvardecl_map.find(entry_key) != ptrvardecl_map.end())
                      ptrvardecl_map.erase(entry_key);
                  }
              }
          }

        //        if (0 && do_out)
        //outs() << "==" << declkind << "," << varName << "," << typeName << "," << filename << "," << linenum << "," << colnum << "\n";
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
