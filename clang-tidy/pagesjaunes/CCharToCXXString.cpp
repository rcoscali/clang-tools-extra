//===--- CCharToCXXString.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <fstream>

#include "CCharToCXXString.h"
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
       * CCharToCXXString constructor
       *
       * @brief Constructor for the CCharToCXXString rewriting ClangTidyCheck
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
      CCharToCXXString::CCharToCXXString(StringRef Name,
                                         ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          TidyContext(Context),
	  handle_strcmp(Options.get("Handle-strcmp", 1U)),
	  handle_strcpy(Options.get("Handle-strcpy", 1U)),
	  handle_strcat(Options.get("Handle-strcat", 1U)),
	  handle_strlen(Options.get("Handle-strlen", 1U)),
          allowed_members_file(Options.get("Allowed-members-file", "members.lst"))
      {
        llvm::outs() << "CCharToCXXString::CCharToCXXString(StringRef Name, ClangTidyContext *Context):\n";
        llvm::outs() << "    handle_strcmp = " << (handle_strcmp?"True":"False") << "\n";
        llvm::outs() << "    handle_strcpy = " << (handle_strcpy?"True":"False") << "\n";
        llvm::outs() << "    handle_strcat = " << (handle_strcat?"True":"False") << "\n";
        llvm::outs() << "    handle_strlen = " << (handle_strlen?"True":"False") << "\n";
        llvm::outs() << "    allowed_members_file = '" << allowed_members_file << "'\n";

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
      CCharToCXXString::readAllowedMembersFile()
      {
        llvm::outs() << "Trying to read file: " << allowed_members_file << "\n";
        std::ifstream src(allowed_members_file.c_str(), std::ios::binary);
        if (src.is_open())
          {
            src.exceptions(std::ifstream::badbit);
            char buf[512];
            while (!src.eof())
              {
                src.getline(buf, 512);
                std::string allowed_member_str(buf);
                if (buf != std::string(""))
                  {
                    std::string::size_type dotpos = allowed_member_str.find('.');
                    std::pair<std::string , std::string> member;
                    if (dotpos != std::string::npos)
                      member = std::make_pair(allowed_member_str.substr(0, dotpos), allowed_member_str.substr(dotpos + 1));
                    else
                      member = std::make_pair(allowed_member_str, std::string(""));
                    llvm::outs() << "Adding allowed " << (dotpos == std::string::npos ? "structure" : "member") << ": " << member.first << ", " << member.second << "\n";
                    m_allowedMembers.push_back(member);
                  }
              }
            src.close();
          }
      }

      /**
       * storeOptions
       *
       * @brief Store options for this check
       *
       * This check support three options for enabling/disabling each
       * call processing:
       * - Handle-strcpy: allowsto enable/disable processing of strcpy
       * - Handle-strcat: allowsto enable/disable processing of strcat
       * - Handle-strcmp: allowsto enable/disable processing of strcmp
       * - Handle-strlen: allowsto enable/disable processing of strlen
       *
       * @param Opts	The option map in which to store supported options
       */
      void
      CCharToCXXString::storeOptions(ClangTidyOptions::OptionMap &Opts)
      {
	Options.store(Opts, "Handle-strcpy", handle_strcpy);
	Options.store(Opts, "Handle-strcat", handle_strcat);
	Options.store(Opts, "Handle-strcmp", handle_strcmp);
	Options.store(Opts, "Handle-strlen", handle_strlen);
	Options.store(Opts, "Allowed-members-file", allowed_members_file);

        llvm::outs() << "CCharToCXXString::storeOptions(ClangTidyOptions::OptionMap &Opts):\n";        
        llvm::outs() << "    Handle-strcpy = " << handle_strcpy << "\n";        
        llvm::outs() << "    Handle-strcat = " << handle_strcat << "\n";        
        llvm::outs() << "    Handle-strcmp = " << handle_strcmp << "\n";        
        llvm::outs() << "    Handle-strlen = " << handle_strlen << "\n";        
        llvm::outs() << "    Allowed-members-file = " << allowed_members_file << "\n";        
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
      CCharToCXXString::registerMatchers(MatchFinder *Finder) 
      {
        // Bind a strcmp call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strcmp"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strcmp_call")
                           , this);
        // Bind a strncmp call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strncmp"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strncmp_call")
                           , this);
        // Bind a strcpy call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strcpy"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strcpy_call")
                           , this);
        // Bind a strncpy call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strncpy"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strncpy_call")
                           , this);
        // Bind a strcat call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strcat"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strcat_call")
                           , this);
        // Bind a strncat call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strncat"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strncat_call")
                           , this);
        // Bind a strlen call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strlen"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strlen_call")
                           , this);
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
      CCharToCXXString::emitDiagAndFix(DiagnosticsEngine &diag_engine,
                                       const SourceLocation& call_start, const SourceLocation& call_end,
                                       enum CCharToCXXStringCallKind function_kind,
                                       std::string& member_tokens,
                                       std::string& member2_tokens,
                                       std::string& member3_tokens,
                                       const SourceLocation& def_start, const SourceLocation& def_end,
                                       std::string& member_name, std::string& object_name,
                                       std::string& field_name, std::string& field_tokens)
      {
        // Source range for statement & definition to rewrite
        SourceRange call_range(call_start, call_end);
        SourceRange def_range(def_start, def_end);

        // Check if struct/member is allowed for transformation
        bool allowed = false;
        llvm::outs() << "Start testing if allowed: " << object_name << ", " << member_name << "\n";
        for (auto it = m_allowedMembers.begin(); it != m_allowedMembers.end(); it++)
          {
            std::string allowed_struct = it->first;
            std::string allowed_member = it->second;
            std::string allowed_struct_struct("struct ");
            allowed_struct_struct.append(allowed_struct);
            
            llvm::outs() << "Testing versus: " << allowed_struct << ", " << allowed_member << "\n";
            if (allowed_member.empty())
              allowed = (allowed_struct == object_name || allowed_struct_struct == object_name);
            else
              allowed = (allowed_struct == object_name || allowed_struct_struct == object_name) && member_name == allowed_member;
            if (allowed)
              break;
          }

        llvm::outs() << "Struct/Member: " << object_name << ", " << member_name << " is " << (allowed?"" : "not ") << "Allowed\n";
        
        if (allowed)
          {
            // Name of the function
            std::string function_name;
	
            // Target rewritten code for call
            std::string replt_4call;
            switch (function_kind)
              {
              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCMP:
                // Write new code for strcmp: member_tokens + ".compare(member2)"
                replt_4call = formatv("{0}.compare({1})", member_tokens, member2_tokens).str();
                function_name.assign("strcmp");
                break;

              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCMP:
                // Write new code for strcmp: member_tokens + ".compare(member2)"
                replt_4call = formatv("{0}.compare(0, std::string::npos, {1}, {2})", member_tokens, member2_tokens, member3_tokens).str();
                function_name.assign("strncmp");
                break;

              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCPY:
                // Write new code for strcpy: member_tokens + ".assign(member2)"
                replt_4call = formatv("{0}.assign({1})", member_tokens, member2_tokens).str();
                function_name.assign("strcpy");
                break;

              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCPY:
                // Write new code for strcpy: member_tokens + ".assign(member2)"
                replt_4call = formatv("{0}.assign({1}, {2})", member_tokens, member2_tokens, member3_tokens).str();
                function_name.assign("strncpy");
                break;

              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCAT:
                // Write new code for strcpy: member_tokens + ".append(member2)"
                replt_4call = formatv("{0}.append({1})", member_tokens, member2_tokens).str();
                function_name.assign("strcat");
                break;

              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCAT:
                // Write new code for strcpy: member_tokens + ".append(member2)"
                replt_4call = formatv("{0}.append({1}, {2})", member_tokens, member2_tokens, member3_tokens).str();
                function_name.assign("strncat");
                break;

              case CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRLEN:
                // Write new code for strlen: member_tokens + ".length()"
                replt_4call = formatv("{0}.length()", member_tokens).str();
                function_name.assign("strlen");
                break;
              }

            // Write new code for definition: std::string + member tokens
            std::string replt_4def = formatv("std::string {0}", field_name).str();

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
              DiagnosticBuilder mydiag = diag(call_start,
                                              "This call to '%0' shall be replaced with std::string '%1' equivalent")
                << function_name << replt_4call;
              // Emit replacement for call expr
              mydiag << FixItHint::CreateReplacement(call_range, replt_4call);
            }

            // Second report
            {
              // Create diag msg for the call expr rewrite
              DiagnosticBuilder mydiag = diag(def_start,
                                              "The member '%0' of structure '%1' shall be replaced with 'std::string %2' equivalent")
                << member_name << object_name << field_name;
        
              mydiag << FixItHint::CreateReplacement(def_range, replt_4def);
            }
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
      CCharToCXXString::emitError(DiagnosticsEngine &diag_engine,
                                  const SourceLocation& err_loc,
                                  enum CCharToCXXStringErrorKind kind,
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

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_NO_ERROR:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Ignored,
			      "No error");
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Constant Array type was not found!");
            break;
            
          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Could not bind the Structure Access expression!");
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Member has no definition!");
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Could not bind the member expression!");
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER2_NOT_FOUND:
            diag_id = TidyContext->getASTContext()->getDiagnostics().
	      getCustomDiagID(DiagnosticsEngine::Error,
			      "Could not bind the second member expression!");
            break;
            
          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND:
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
       * checkStrcmp
       *
       * @brief check the strcmp call is ok to be processed
       *
       * Find the member definition and check if it can be replaced by
       * a string.
       *
       * @param src_mgr		Source manager
       * @param diag_engine	Diagnostics engine
       * @param call_kind	The name of the called function ("strcmp")
       * @param strcmp_call	The CallExpr AST node for strcmp call
       * @param result		The matched result
       */
      void
      CCharToCXXString::checkStrcmp(SourceManager &src_mgr,
                                    DiagnosticsEngine &diag_engine,
                                    CallExpr* strcmp_call,
                                    const MatchFinder::MatchResult &result)
      {
        do
          {
            // Get nodes bound to variables 
            const ConstantArrayType *arraytype = result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
            const CXXRecordDecl *obj_decl = result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
            const MemberExpr *member_expr = result.Nodes.getNodeAs<MemberExpr>("member_expr");

            // Get start/end locations for the statement
            SourceLocation call_start = strcmp_call->getLocStart();
            SourceLocation call_end = strcmp_call->getLocEnd();

            /* 
             * Handle unexpected cases
             */

            // Missing constant array type AST nodes
            if (!arraytype)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                break;
              }

            // Missing constant array member
            if (!obj_decl)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                break;
              }

            // Missing structure owning constant array 
            if (!obj_decl->hasDefinition())
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                break;
              }

            // Missing constant constant array member expr
            if (!member_expr)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                break;
              }

            unsigned int numArgs = strcmp_call->getNumArgs();

            const Expr *arg1 = strcmp_call->getArg(0);
            SourceLocation member_start = arg1->getLocStart();
            SourceLocation member_end = arg1->getLocEnd();

	    bool arg1_is_literal = isa<StringLiteral>(arg1->IgnoreImpCasts());

            const Expr *arg2 = strcmp_call->getArg(1);
            SourceLocation member2_start = arg2->getLocStart();
            SourceLocation member2_end = arg2->getLocEnd();

            SourceLocation member_expr_start = member_expr->getLocStart();
            SourceLocation member_expr_end = member_expr->getLocEnd();

            std::string member_name;
            SourceLocation member_loc;
            DeclarationNameLoc member_decl_name_loc;

            std::string member2_name;
            SourceLocation member2_loc;
            DeclarationNameLoc member2_decl_name_loc;
      
            const Expr *arg3 = nullptr;
            SourceLocation member3_start;
            SourceLocation member3_end;
            if (numArgs == 3)
              {
                arg3 = strcmp_call->getArg(2);
                member3_start = arg3->getLocStart();
                member3_end = arg3->getLocEnd();
              }
                
            Expr *base = member_expr->getBase();
            QualType base_type = base->getType();
            std::string object_name = base_type.getAsString();
                
            std::string obj_decl_kind = ((Decl *)(obj_decl->getDefinition()))->getDeclKindName();
            if (obj_decl_kind.compare("CXXRecord") == 0)
              {
                RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();
                    
                const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
                    
                member_name = info.getName().getAsString();
                member_loc = info.getLoc();
                member_decl_name_loc = info.getInfo();
                    
                    
                for (RecordDecl::field_iterator fit = definition->field_begin();
                     fit != definition->field_end();
                     fit++)
                  {
                    FieldDecl *field_decl = *fit;
                    std::string field_name = field_decl->getNameAsString();
                    if (!member_name.compare(field_name))
                      {
                        SourceRange field_src_range = field_decl->getSourceRange();
                        SourceLocation field_begin_loc = field_src_range.getBegin();
                        SourceLocation field_end_loc = field_src_range.getEnd();
                        const SourceLocation spelling_field_begin_loc = src_mgr.getImmediateSpellingLoc(field_begin_loc);
                        std::string field_file_name = src_mgr.getFilename(spelling_field_begin_loc);

                        SourceRange member_expr_range(member_expr_start, member_expr_end);
                        SourceRange member_range(member_start, member_end);
                        SourceRange member2_range(member2_start, member2_end);
                        SourceRange member3_range(member3_start, member3_end);
                        SourceRange field_range(field_begin_loc, field_end_loc);
                        std::string member_expr_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_expr_range), src_mgr, LangOptions(), 0);
                        std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
                        std::string member2_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member2_range), src_mgr, LangOptions(), 0);
                        std::string member3_tokens;
                        if (numArgs == 3)
                          member3_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member3_range), src_mgr, LangOptions(), 0);
                        std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);

			if (!arg1_is_literal)
			  emitDiagAndFix(diag_engine,
					 call_start, call_end,
					 numArgs == 3 ? CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCMP : CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCMP,
					 member_tokens, member2_tokens, member3_tokens, 
					 field_begin_loc, field_end_loc,
					 member_name, object_name, field_name, field_tokens);
			else
			  emitDiagAndFix(diag_engine,
					 call_start, call_end,
					 numArgs == 3 ? CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCMP : CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCMP,
					 member2_tokens, member_tokens, member3_tokens, 
					 field_begin_loc, field_end_loc,
					 member_name, object_name, field_name, field_tokens);
                      }
                  }
              }
            else
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
                          &obj_decl_kind);
                break;
              }
          }
        while(0);
      }

      /**
       * checkStrcpy
       *
       * @brief check the strcpy call is ok to be processed
       *
       * Find the member definition and check if it can be replaced by
       * a string.
       *
       * @param src_mgr		Source manager
       * @param diag_engine	Diagnostics engine
       * @param call_kind	The kind of the called function ("strcpy")
       * @param strcpy_call	The CallExpr AST node for strcpy call
       * @param result		The matched result
       */
      void
      CCharToCXXString::checkStrcpy(SourceManager &src_mgr,
                                    DiagnosticsEngine &diag_engine,
                                    CallExpr* strcpy_call,
                                    const MatchFinder::MatchResult &result)
      {
        do
          {
            const ConstantArrayType *arraytype = result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
            const CXXRecordDecl *obj_decl = result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
            const MemberExpr *member_expr = result.Nodes.getNodeAs<MemberExpr>("member_expr");
                
            // Get start/end locations for the statement
            SourceLocation call_start = strcpy_call->getLocStart();
            SourceLocation call_end = strcpy_call->getLocEnd();

            /* 
             * Handle unexpected cases
             */

            // Missing constant array type AST nodes
            if (!arraytype)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                break;
              }

            // Missing constant array member
            if (!obj_decl)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                break;
              }

            // Missing structure owning constant array 
            if (!obj_decl->hasDefinition())
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                break;
              }

            // Missing constant constant array member expr
            if (!member_expr)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                break;
              }

            unsigned int numArgs = strcpy_call->getNumArgs();

            const Expr *arg1 = strcpy_call->getArg(0);
            SourceLocation member_start = arg1->getLocStart();
            SourceLocation member_end = arg1->getLocEnd();

	    bool arg1_is_literal = isa<StringLiteral>(arg1->IgnoreImpCasts());

            const Expr *arg2 = strcpy_call->getArg(1);
            SourceLocation member2_start = arg2->getLocStart();
            SourceLocation member2_end = arg2->getLocEnd();

            const Expr *arg3 = nullptr;
            SourceLocation member3_start;
            SourceLocation member3_end;
            if (numArgs == 3)
              {
                arg3 = strcpy_call->getArg(2);
                member3_start = arg3->getLocStart();
                member3_end = arg3->getLocEnd();
              }
                
            std::string member_name;
            SourceLocation member_loc;
            DeclarationNameLoc member_decl_name_loc;

            std::string member2_name;
            SourceLocation member2_loc;
            DeclarationNameLoc member2_decl_name_loc;
                
            std::string member3_name;
            SourceLocation member3_loc;
            DeclarationNameLoc member3_decl_name_loc;
                
            const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
                    
            member_name = info.getName().getAsString();
            member_loc = info.getLoc();
            member_decl_name_loc = info.getInfo();

            Expr *base = member_expr->getBase();
            QualType base_type = base->getType();
            std::string object_name = base_type.getAsString();

            DeclContext *decl_ctxt = dynamic_cast<DeclContext*>(obj_decl->getDefinition());
            std::string obj_decl_kind = decl_ctxt->getDeclKindName();

            if (!obj_decl_kind.compare("CXXRecord"))
              {
                RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();

                // Get compound statement start line num
                for (RecordDecl::field_iterator fit = definition->field_begin();
                     fit != definition->field_end();
                     fit++)
                  {
                    FieldDecl *field_decl = *fit;
                    std::string field_name = field_decl->getNameAsString();
                    if (!member_name.compare(field_name))
                      {
                        SourceRange field_src_range = field_decl->getSourceRange();
                        SourceLocation field_begin_loc = field_src_range.getBegin();
                        SourceLocation field_end_loc = field_src_range.getEnd();

                        SourceRange member_range(member_start, member_end);
                        SourceRange member2_range(member2_start, member2_end);
                        SourceRange member3_range(member3_start, member3_end);
                        SourceRange field_range(field_begin_loc, field_end_loc);
                        std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
                        std::string member2_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member2_range), src_mgr, LangOptions(), 0);
                        std::string member3_tokens;
                        if (numArgs == 3)
                          member3_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member3_range), src_mgr, LangOptions(), 0);
                        std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);
                            
			if (!arg1_is_literal)
			  emitDiagAndFix(diag_engine,
					 call_start, call_end,
					 numArgs == 3 ? CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCPY : CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCPY,
					 member_tokens, member2_tokens, member3_tokens, 
					 field_begin_loc, field_end_loc,
					 member_name, object_name, field_name, field_tokens);
			else
			  emitDiagAndFix(diag_engine,
					 call_start, call_end,
					 numArgs == 3 ? CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCPY : CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCPY,
					 member2_tokens, member_tokens, member3_tokens, 
					 field_begin_loc, field_end_loc,
					 member_name, object_name, field_name, field_tokens);
                      }
                  }
              }
            else
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
                          &obj_decl_kind);
                break;
              }
          }
        while (0);
      }      

      /**
       * checkStrcat
       *
       * @brief check the strcat call is ok to be processed
       *
       * Find the member definition and check if it can be replaced by
       * a string.
       *
       * @param src_mgr		Source manager
       * @param diag_engine	Diagnostics engine
       * @param call_kind	The kind of the called function ("strcat")
       * @param strcat_call	The CallExpr AST node for strcat call
       * @param result		The matched result
       */
      void
      CCharToCXXString::checkStrcat(SourceManager &src_mgr,
                                    DiagnosticsEngine &diag_engine,
                                    CallExpr* strcat_call,
                                    const MatchFinder::MatchResult &result)
      {
        do
          {
            const ConstantArrayType *arraytype = result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
            const CXXRecordDecl *obj_decl = result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
            const MemberExpr *member_expr = result.Nodes.getNodeAs<MemberExpr>("member_expr");
                
            // Get start/end locations for the statement
            SourceLocation call_start = strcat_call->getLocStart();
            SourceLocation call_end = strcat_call->getLocEnd();

            /* 
             * Handle unexpected cases
             */

            // Missing constant array type AST nodes
            if (!arraytype)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                break;
              }

            // Missing constant array member
            if (!obj_decl)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                break;
              }

            // Missing structure owning constant array 
            if (!obj_decl->hasDefinition())
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                break;
              }

            // Missing constant constant array member expr
            if (!member_expr)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                break;
              }

            unsigned int numArgs = strcat_call->getNumArgs();

            const Expr *arg1 = strcat_call->getArg(0);
            SourceLocation member_start = arg1->getLocStart();
            SourceLocation member_end = arg1->getLocEnd();

	    bool arg1_is_literal = isa<StringLiteral>(arg1->IgnoreImpCasts());

            const Expr *arg2 = strcat_call->getArg(1);
            SourceLocation member2_start = arg2->getLocStart();
            SourceLocation member2_end = arg2->getLocEnd();

            const Expr *arg3 = nullptr;
            SourceLocation member3_start;
            SourceLocation member3_end;
            if (numArgs == 3)
              {
                arg3 = strcat_call->getArg(2);
                member3_start = arg3->getLocStart();
                member3_end = arg3->getLocEnd();
              }
                
            std::string member_name;
            SourceLocation member_loc;
            DeclarationNameLoc member_decl_name_loc;

            std::string member2_name;
            SourceLocation member2_loc;
            DeclarationNameLoc member2_decl_name_loc;
                
            std::string member3_name;
            SourceLocation member3_loc;
            DeclarationNameLoc member3_decl_name_loc;
                
            const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
                    
            member_name = info.getName().getAsString();
            member_loc = info.getLoc();
            member_decl_name_loc = info.getInfo();

            Expr *base = member_expr->getBase();
            QualType base_type = base->getType();
            std::string object_name = base_type.getAsString();

            DeclContext *decl_ctxt = dynamic_cast<DeclContext*>(obj_decl->getDefinition());
            std::string obj_decl_kind = decl_ctxt->getDeclKindName();

            if (!obj_decl_kind.compare("CXXRecord"))
              {
                RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();

                // Get compound statement start line num
                for (RecordDecl::field_iterator fit = definition->field_begin();
                     fit != definition->field_end();
                     fit++)
                  {
                    FieldDecl *field_decl = *fit;
                    std::string field_name = field_decl->getNameAsString();
                    if (!member_name.compare(field_name))
                      {
                        SourceRange field_src_range = field_decl->getSourceRange();
                        SourceLocation field_begin_loc = field_src_range.getBegin();
                        SourceLocation field_end_loc = field_src_range.getEnd();

                        SourceRange member_range(member_start, member_end);
                        SourceRange member2_range(member2_start, member2_end);
                        SourceRange member3_range(member3_start, member3_end);
                        SourceRange field_range(field_begin_loc, field_end_loc);
                        std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
                        std::string member2_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member2_range), src_mgr, LangOptions(), 0);
                        std::string member3_tokens;
                        if (numArgs == 3)
                          member3_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member3_range), src_mgr, LangOptions(), 0);
                        std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);
                            
			if (!arg1_is_literal)
			  emitDiagAndFix(diag_engine,
					 call_start, call_end,
					 numArgs == 3 ? CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCAT : CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCAT,
					 member_tokens, member2_tokens, member3_tokens, 
					 field_begin_loc, field_end_loc,
					 member_name, object_name, field_name, field_tokens);
			else
			  emitDiagAndFix(diag_engine,
					 call_start, call_end,
					 numArgs == 3 ? CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRNCAT : CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRCAT,
					 member2_tokens, member_tokens, member3_tokens, 
					 field_begin_loc, field_end_loc,
					 member_name, object_name, field_name, field_tokens);
                      }
                  }
              }
            else
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
                          &obj_decl_kind);
                break;
              }
          }
        while (0);
      }      

      /**
       * checkStrlen
       *
       * @brief check the strlen call is ok to be processed
       *
       * Find the member definition and check if it can be replaced by
       * a string.
       *
       * @param src_mgr		Source manager
       * @param diag_engine	Diagnostics engine
       * @param call_kind	The kind of the called function ("strlen")
       * @param strlen_call	The CallExpr AST node for strlen call
       * @param result		The matched result
       */
      void
      CCharToCXXString::checkStrlen(SourceManager &src_mgr,
                                    DiagnosticsEngine &diag_engine,
                                    CallExpr* strlen_call,
                                    const MatchFinder::MatchResult &result)
      {
        do
          {
	    // Get our bounded vars
            const ConstantArrayType *arraytype = result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
            const CXXRecordDecl *obj_decl = result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
            const MemberExpr *member_expr = result.Nodes.getNodeAs<MemberExpr>("member_expr");

            // Get start/end locations for the statement
            SourceLocation call_start = strlen_call->getLocStart();
            SourceLocation call_end = strlen_call->getLocEnd();

            /* 
             * Handle unexpected cases
             */

            // Missing constant array type AST nodes
            if (!arraytype)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                break;
              }

            // Missing constant array member
            if (!obj_decl)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                break;
              }

            // Missing structure owning constant array 
            if (!obj_decl->hasDefinition())
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                break;
              }

            // Missing constant constant array member expr
            if (!member_expr)
              {
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                break;
              }

	    // Get argument of strlen
            const Expr *arg1 = strlen_call->getArg(0);
	    // Then get start/end locations for getting tokens
            SourceLocation member_start = arg1->getLocStart();
            SourceLocation member_end = arg1->getLocEnd();

	    // Get declaration name info (allows getting decl type info)
            const DeclarationNameInfo& info = member_expr->getMemberNameInfo();

	    // Get member name, ...
            std::string member_name = info.getName().getAsString();

	    // Get the base type for the member
            Expr *base = member_expr->getBase();
	    // Qualified type,
            QualType base_type = base->getType();
	    // And name of the object(struct??)
            std::string object_name = base_type.getAsString();

	    // Get the decl context
            DeclContext *decl_ctxt = dynamic_cast<DeclContext*>(obj_decl->getDefinition());
	    // Get the kind fo object decl
            std::string obj_decl_kind = decl_ctxt->getDeclKindName();

	    // Check that the definition is a CXXRecord
            if (!obj_decl_kind.compare("CXXRecord"))
              {
		// Access definition record decl
                RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();

                // Get each field of the structure
		    // And try to find the one we want ...
                for (RecordDecl::field_iterator fit = definition->field_begin();
                     fit != definition->field_end();
                     fit++)
                  {
		    // Get field decl from iterator
                    FieldDecl *field_decl = *fit;
		    // and get its name
                    std::string field_name = field_decl->getNameAsString();
		    // Is it the good one ?
                    if (!member_name.compare(field_name))
                      {
			// If yes, find locations where it is defined, its range
                        SourceRange field_src_range = field_decl->getSourceRange();
			// ... start 
                        SourceLocation field_begin_loc = field_src_range.getBegin();
			// ... and end
                        SourceLocation field_end_loc = field_src_range.getEnd();

			// Now let's get tokens for rewriting code
			// Create a range for the member expression
                        SourceRange member_range(member_start, member_end);
			// and for the field expr
                        SourceRange field_range(field_begin_loc, field_end_loc);
			// Get tokens for member expr in strlen
                        std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
			// No second & third members (empty)
                        std::string member2_tokens;
                        std::string member3_tokens;
			// And the field one
                        std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);

			// Emit diag and replacement code
                        emitDiagAndFix(diag_engine,
                                       call_start, call_end,
				       CCharToCXXString::CCHAR_2_CXXSTRING_CALL_STRLEN,
                                       member_tokens, member2_tokens, member3_tokens, 
                                       field_begin_loc, field_end_loc,
                                       member_name, object_name, field_name, field_tokens);
                      }
                  }
              }
            else
              {
		// An error occured: cannot use kind of AST node we got (missing impl)
                emitError(diag_engine,
                          call_start,
                          CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
                          &obj_decl_kind);
                break;
              }
          }
        while (0);
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
      CCharToCXXString::check(const MatchFinder::MatchResult &result) 
      {
        // Get root bounded variables
        CallExpr *strcmp_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strcmp_call");
        CallExpr *strncmp_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strncmp_call");
        CallExpr *strcpy_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strcpy_call");
        CallExpr *strncpy_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strncpy_call");
        CallExpr *strcat_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strcpat_call");
        CallExpr *strncat_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strncat_call");
        CallExpr *strlen_call = (CallExpr *) result.Nodes.getNodeAs<CallExpr>("strlen_call");

        // Get the source manager
        SourceManager &src_mgr = result.Context->getSourceManager();
        // And diagnostics engine
        DiagnosticsEngine &diag_engine = result.Context->getDiagnostics();

        /*
         * Handle strcmp calls
         */
        if (handle_strcmp && strcmp_call)
	  checkStrcmp(src_mgr,
		      diag_engine,
		      strcmp_call,
		      result);

        else if (handle_strcmp && strncmp_call)
	  checkStrcmp(src_mgr,
		      diag_engine,
		      strncmp_call,
		      result);

        /*
         * Handle strcpy calls
         */
        else if (handle_strcpy && strcpy_call)
	  checkStrcpy(src_mgr,
		      diag_engine,
		      strcpy_call,
		      result);

        else if (handle_strcpy && strncpy_call)
	  checkStrcpy(src_mgr,
		      diag_engine,
		      strncpy_call,
		      result);

        /*
         * Handle strcat calls
         */
        else if (handle_strcat && strcat_call)
	  checkStrcat(src_mgr,
		      diag_engine,
		      strcat_call,
		      result);

        else if (handle_strcat && strncat_call)
	  checkStrcat(src_mgr,
		      diag_engine,
		      strncat_call,
		      result);

        /*
         * Handle strlen calls
         */
        else if (handle_strlen && strlen_call)
	  checkStrlen(src_mgr,
		      diag_engine,
		      strlen_call,
		      result);
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
