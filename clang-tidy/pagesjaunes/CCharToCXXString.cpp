//===--- CCharToCXXString.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CCharToCXXString.h"
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
          unexpected_diag_id(Context->
                             getCustomDiagID(DiagnosticsEngine::Warning,
                                             "Unexpected error occured?!")),
          no_error_diag_id(Context->
                           getCustomDiagID(DiagnosticsEngine::Ignored,
                                           "No error")),
          array_type_not_found_diag_id(Context->
                                       getCustomDiagID(DiagnosticsEngine::Error,
                                                       "Constant Array type was not found!")),
          record_decl_not_found_diag_id(Context->
                                        getCustomDiagID(DiagnosticsEngine::Error,
                                                        "Could not bind the Structure Access expression!")),
          member_has_no_def_diag_id(Context->
                                    getCustomDiagID(DiagnosticsEngine::Error,
                                                    "Member has no definition!")),
          member_not_found_diag_id(Context->
                                   getCustomDiagID(DiagnosticsEngine::Error,
                                                   "Could not bind the member expression!")),
          member2_not_found_diag_id(Context->
                                    getCustomDiagID(DiagnosticsEngine::Error,
                                                    "Could not bind the second member expression!")),
          unexpected_ast_node_kind_diag_id(Context->
					   getCustomDiagID(DiagnosticsEngine::Error,
							   "Could not process member owning record kind!"))
      {}

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
        // Bind a strcpy call with a constant char array
        Finder->addMatcher(callExpr(callee(functionDecl(hasName("strcpy"))),
                                    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType(),
                                                                                                      isAnyCharacter())).bind("arraytype")),
                                                             hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
                                    ).bind("strcpy_call")
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
       * @param def_start       The member definition start
       * @param def_end         The member definition end
       * @param member_name     The member name we will change the stype (using std::string)
       * @param object_name     The object name holding the member we will change type
       * @param field_name	The structure (object) field name
       * @param field_tokens    The structure (object) field tokens
       */
      void
      CCharToCXXString::emitDiagAndFix(DiagnosticsEngine &diag_engine,
				       const SourceLocation& call_start, const SourceLocation& call_end,
                                       std::string& function_name,
                                       std::string& member_tokens,
                                       std::string& member2_tokens,
                                       const SourceLocation& def_start, const SourceLocation& def_end,
                                       std::string& member_name, std::string& object_name,
                                       std::string& field_name, std::string& field_tokens)
      {
	// Source range for statement & definition to rewrite
        SourceRange call_range(call_start, call_end);
        SourceRange def_range(def_start, def_end);

	// Target rewritten code
        std::string replt_4call;
        std::string replt_4def;

        // Write new code for strcmp: 'member1.compare(member2) == 0'
        // and for definition: std::string member1
        // TODO: get operator and literal from CallExpr
        if (function_name.compare("strcmp") == 0)
          {
            replt_4call.append(member_tokens);
            replt_4call.append(".compare(");
            replt_4call.append(member2_tokens);
            replt_4call.append(");");
            replt_4def.append("std::string ");
            replt_4def.append(field_name);
            replt_4def.append(";");
          }
        // Write new code for strcpy: member1.assign(member2)
        // and for definition: std::string member1
        else if (function_name.compare("strcpy") == 0)
          {
            replt_4call.append(member_tokens);
            replt_4call.append(".assign(");
            replt_4call.append(member2_tokens);
            replt_4call.append(");");
            replt_4def.append("std::string ");
            replt_4def.append(field_name);
            replt_4def.append(";");
          }
        // Write new code for strlen: member.length()
        // and for definition: std::string member1
        else if (function_name.compare("strlen") == 0)
          {
            replt_4call.append(member_tokens);
            replt_4call.append(".length();");
            replt_4def.append("std::string ");
            replt_4def.append(field_name);
            replt_4def.append(";");
          }

	{
	  // Create diag msg for the call expr rewrite
	  DiagnosticBuilder mydiag = diag(call_start,
					  "This call to '%0' shall be replaced with std::string '%1' equivalent")
	    << function_name << replt_4call;
	  // Emit replacement for call expr
	  mydiag << FixItHint::CreateReplacement(call_range, replt_4call);
	}
	{
	  // Create diag msg for the call expr rewrite
	  DiagnosticBuilder mydiag = diag(def_start,
					    "The member '%0' of structure '%1' shall be replaced with 'std::string %2' equivalent")
	    << member_name << object_name << field_name;
        
	  mydiag << FixItHint::CreateReplacement(def_range, replt_4def);
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
            diag_id = unexpected_diag_id;
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_NO_ERROR:
            diag_id = no_error_diag_id;
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND:
            diag_id = array_type_not_found_diag_id;
            break;
            
          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND:
            diag_id = record_decl_not_found_diag_id;
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF:
            diag_id = member_has_no_def_diag_id;
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND:
            diag_id = member_not_found_diag_id;
            break;

          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER2_NOT_FOUND:
            diag_id = member2_not_found_diag_id;
            break;
            
          case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND:
            diag_id = unexpected_ast_node_kind_diag_id;
            break;
          }
        DiagnosticBuilder diag = diag_engine.Report(err_loc, diag_id);
        if (msg && !msg->empty())
          diag.AddString(*msg);
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
       * @param Result          The match result provided by the recursive visitor
       *                        allowing us to access AST nodes bound to variables
       */
      void
      CCharToCXXString::check(const MatchFinder::MatchResult &Result) 
      {
        // Get root bounded variables
        const CallExpr *strcmp_call = Result.Nodes.getNodeAs<CallExpr>("strcmp_call");
        const CallExpr *strcpy_call = Result.Nodes.getNodeAs<CallExpr>("strcpy_call");
        const CallExpr *strlen_call = Result.Nodes.getNodeAs<CallExpr>("strlen_call");

        // Get the source manager
        SourceManager &src_mgr = Result.Context->getSourceManager();
        // And diagnostics engine
        DiagnosticsEngine &diagEngine = Result.Context->getDiagnostics();

        /*
         * Handle strcmp calls
         */
        if (strcmp_call)
          {
            do
              {
                std::string call_name("strcmp");
		
		// Get nodes bound to variables 
                const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
                const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
                const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

                // Get start/end locations for the statement
                SourceLocation call_start = strcmp_call->getLocStart();
                SourceLocation call_end = strcmp_call->getLocEnd();

                /* 
                 * Handle unexpected cases
                 */

                // Missing constant array type AST nodes
                if (!arraytype)
                  {
                    outs() << "Error: No member could be found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                    break;
                  }

                // Missing constant array member
                if (!obj_decl)
                  {
                    outs() << "Error: Member has not found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                    break;
                  }

                // Missing structure owning constant array 
                if (!obj_decl->hasDefinition())
                  {
                    outs() << "Error: Member has no definition !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                    break;
                  }

                // Missing constant constant array member expr
                if (!member_expr)
                  {
                    outs() << "Error: No member could be found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                    break;
                  }

                // Get file ID in src mgr
                FileID start_fid = src_mgr.getFileID(call_start);
                // Get compound statement start line num
                unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));
                unsigned start_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_start));
                unsigned end_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_end));
                unsigned end_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_end));

		const Expr *arg1 = strcmp_call->getArg(0);
		SourceLocation member_start = arg1->getLocStart();
		SourceLocation member_end = arg1->getLocEnd();
		outs() << "arg1 dump:\n";
		arg1->IgnoreImpCasts()->dump();

		const Expr *arg2 = strcmp_call->getArg(1);
		SourceLocation member2_start = arg2->getLocStart();
		SourceLocation member2_end = arg2->getLocEnd();
		outs() << "arg2 dump:\n";
		arg2->IgnoreImpCasts()->dump();

		outs() << "member_expr dump:\n";
		member_expr->dump();

		SourceLocation member_expr_start = member_expr->getLocStart();
		SourceLocation member_expr_end = member_expr->getLocEnd();

		// if (arg1 == member_expr)
		//   {
		//     outs() << "Not switching args ...\n";
		//     const Expr *tmp = arg2;
		//     arg2 = (const Expr *)member_expr->IgnoreImpCasts();
		//     member_expr = static_cast<const MemberExpr *>(tmp);
		//   }
		// else
		//   {
		//     SourceLocation tmploc = member_expr_start;
		//     member_expr_start = member_expr_end;
		//     member_expr_end = tmploc;
		//     outs() << "Switching args ...\n";
		//     const Expr *tmp = arg1;
		//     arg1 = (const Expr *)member_expr->IgnoreImpCasts();
		//     member_expr = static_cast<const MemberExpr *>(tmp);
		//   }

                outs() << "Found a strcmp call statement from #"
                       << start_line_num << ":" << start_col_num
                       << " to "
                       << end_line_num << ":" << end_col_num << "\n";
                
                QualType element_type = arraytype->getElementType();
                outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
                outs() << "   ... an array of " << element_type.getAsString() << " elements\n";
                outs() << "   and its size is " << arraytype->getSize() << "\n";
                
                std::string member_name;
                SourceLocation member_loc;
                DeclarationNameLoc member_decl_name_loc;

                std::string member2_name;
                SourceLocation member2_loc;
                DeclarationNameLoc member2_decl_name_loc;
      
		ExprValueKind member_expr_value_kind = member_expr->getValueKind();
		
		outs() << "Member expr value kind is: ";
		switch (member_expr_value_kind)
		  {
		  case clang::VK_RValue:
		    outs() << "RValue\n";
		    break;
		    
		  case clang::VK_LValue:
		    outs() << "LValue\n";
		    break;
      
		  case clang::VK_XValue:
		    break;
		  }
     		
		ExprObjectKind member_expr_object_kind = member_expr->getObjectKind();

		outs() << "Member expr object kind: ";
		switch (member_expr_object_kind)
		  {
		  case clang::OK_Ordinary:
		    outs() << "Ordinary\n";
		    break;
		    
		  case clang::OK_BitField:
		    outs() << "BitField\n";
		    break;
      
		  case clang::OK_VectorComponent:
		    outs() << "VectorComponent\n";
		    break;
      
		  case clang::OK_ObjCProperty:
		    outs() << "ObjCProperty\n";
		    break;
      
		  case clang::OK_ObjCSubscript:
		    outs() << "ObjCSubscript\n";
		    break;
      
		  }
		
		Expr *base = member_expr->getBase();
		QualType base_type = base->getType();
		std::string object_name = base_type.getAsString();
		QualType member_type = member_expr->getType();

		outs() << " Base Type name = '" << base_type.getAsString() << "'\n";
		outs() << " Member Type name = '" << member_type.getAsString() << "'\n";
		
                std::string obj_decl_kind = ((Decl *)(obj_decl->getDefinition()))->getDeclKindName();
                outs() << "Object decl: " << obj_decl_kind << "\n";
                if (obj_decl_kind.compare("CXXRecord") == 0)
                  {
                    RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();
                    const SourceLocation decl_type_loc = ((const TypeDecl *)obj_decl)->getLocStart();
                    // Get compound statement start line num
                    const SourceLocation spelling_decl_type_loc = src_mgr.getImmediateSpellingLoc(decl_type_loc);
                    FileID decl_type_loc_fid = src_mgr.getFileID(spelling_decl_type_loc);
                    unsigned decl_type_loc_line_num = src_mgr.getLineNumber(decl_type_loc_fid, src_mgr.getFileOffset(spelling_decl_type_loc));
                    outs() << "Object decl type defined in "
                           << src_mgr.getFilename(spelling_decl_type_loc)
                           << " at line "
                           << decl_type_loc_line_num
                           << "\n";
                    
                    const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
                    
                    member_name = info.getName().getAsString();
                    member_loc = info.getLoc();
                    member_decl_name_loc = info.getInfo();
                    
                    const SourceLocation spelling_member_loc = src_mgr.getImmediateSpellingLoc(member_loc);
                    FileID member_loc_fid = src_mgr.getFileID(spelling_member_loc);
                    unsigned member_loc_line_num = src_mgr.getLineNumber(member_loc_fid, src_mgr.getFileOffset(spelling_member_loc));
                    
                    outs() << "Member name: " << member_name << "\n";
                    outs() << "    declared in " << src_mgr.getFilename(spelling_member_loc) << " at line #" << member_loc_line_num << "\n";
                    
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
                            const SourceLocation spelling_field_end_loc = src_mgr.getImmediateSpellingLoc(field_end_loc);
                            FileID field_loc_fid = src_mgr.getFileID(spelling_field_begin_loc);
                            unsigned field_begin_loc_line_num = src_mgr.getLineNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_begin_loc));
                            unsigned field_begin_loc_col_num = src_mgr.getColumnNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_begin_loc));
                            unsigned field_end_loc_line_num = src_mgr.getLineNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_end_loc));
                            unsigned field_end_loc_col_num = src_mgr.getColumnNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_end_loc));
                            std::string field_file_name = src_mgr.getFilename(spelling_field_begin_loc);

                            outs() << "    defined in " << field_file_name << " starting at linecol #"
                                   << field_begin_loc_line_num << ":" << field_begin_loc_col_num
                                   << " ending at linecol # "
                                   << field_end_loc_line_num << ":" << field_end_loc_col_num
                                   << "\n";
                            
                            SourceRange member_expr_range(member_expr_start, member_expr_end);
                            SourceRange member_range(member_start, member_end);
                            SourceRange member2_range(member2_start, member2_end);
                            SourceRange field_range(field_begin_loc, field_end_loc);
			    std::string member_expr_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_expr_range), src_mgr, LangOptions(), 0);
			    std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
			    std::string member2_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member2_range), src_mgr, LangOptions(), 0);
			    std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);

			    outs() << "member_expr = '" << member_expr_tokens << "'\n";
			    outs() << "arg1 = '" << member_tokens << "'\n";
			    outs() << "arg2 = '" << member2_tokens << "'\n";
			    outs() << "field = '" << field_tokens << "'\n";
                            
			    //strcmp_call->dump();
			    outs() << "\n";

                            emitDiagAndFix(diagEngine,
					   call_start, call_end,
                                           call_name, member_tokens, member2_tokens, 
                                           field_begin_loc, field_end_loc,
                                           member_name, object_name, field_name, field_tokens);
                          }
                      }
                  }
                else
                  {
                    outs() << "Error: Unexpected AST node kind ! Please handle this new kind: '" << obj_decl_kind << "'!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
                              &obj_decl_kind);
                    break;
                  }
              }
            while(0);
          }
        /*
         * Handle strcpy calls
         */
        else if (strcpy_call)
          {
            do
              {
		std::string call_name("strcpy");
		const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
		const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
		const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");
		
		// Get start/end locations for the statement
		SourceLocation call_start = strcpy_call->getLocStart();
		SourceLocation call_end = strcpy_call->getLocEnd();

                /* 
                 * Handle unexpected cases
                 */

                // Missing constant array type AST nodes
                if (!arraytype)
                  {
                    outs() << "Error: No member could be found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                    break;
                  }

                // Missing constant array member
                if (!obj_decl)
                  {
                    outs() << "Error: Member has not found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                    break;
                  }

                // Missing structure owning constant array 
                if (!obj_decl->hasDefinition())
                  {
                    outs() << "Error: Member has no definition !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                    break;
                  }

                // Missing constant constant array member expr
                if (!member_expr)
                  {
                    outs() << "Error: No member could be found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                    break;
                  }

		// Get file ID in src mgr
		FileID start_fid = src_mgr.getFileID(call_start);
		// Get compound statement start line num
		unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));
		unsigned start_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_start));
		unsigned end_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_end));
		unsigned end_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_end));
		
		const Expr *arg1 = strcpy_call->getArg(0);
		SourceLocation member_start = arg1->getLocStart();
		SourceLocation member_end = arg1->getLocEnd();

		outs() << "arg1 dump:\n";
		arg1->IgnoreImpCasts()->dump();

		const Expr *arg2 = strcpy_call->getArg(1);
		SourceLocation member2_start = arg2->getLocStart();
		SourceLocation member2_end = arg2->getLocEnd();
                
		outs() << "arg2 dump:\n";
		arg2->IgnoreImpCasts()->dump();
                
		outs() << "member_expr dump:\n";
		member_expr->dump();

		outs() << "Found a strcpy call statement from #"
		       << start_line_num << ":" << start_col_num
		       << " to "
		       << end_line_num << ":" << end_col_num << "\n";
		
                QualType element_type = arraytype->getElementType();
                outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
                outs() << "   ... an array of " << element_type.getAsString() << " elements\n";
                outs() << "   and its size is " << arraytype->getSize() << "\n";
		
		std::string member_name;
		SourceLocation member_loc;
		DeclarationNameLoc member_decl_name_loc;

                std::string member2_name;
                SourceLocation member2_loc;
                DeclarationNameLoc member2_decl_name_loc;
                
		const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
		    
		member_name = info.getName().getAsString();
		member_loc = info.getLoc();
		member_decl_name_loc = info.getInfo();

		Expr *base = member_expr->getBase();
		QualType base_type = base->getType();
		std::string object_name = base_type.getAsString();
		QualType member_type = member_expr->getType();

		outs() << " Base Type name = '" << base_type.getAsString() << "'\n";
		outs() << " Member Type name = '" << member_type.getAsString() << "'\n";
		
		const SourceLocation spelling_member_loc = src_mgr.getImmediateSpellingLoc(member_loc);
		FileID member_loc_fid = src_mgr.getFileID(spelling_member_loc);
		unsigned member_loc_line_num = src_mgr.getLineNumber(member_loc_fid, src_mgr.getFileOffset(spelling_member_loc));
		
		outs() << "Member name: " << member_name << "\n";
		outs() << "    declared in " << src_mgr.getFilename(spelling_member_loc) << " at line #" << member_loc_line_num << "\n";

		DeclContext *decl_ctxt = dynamic_cast<DeclContext*>(obj_decl->getDefinition());
		std::string obj_decl_kind = decl_ctxt->getDeclKindName();
		outs() << "Object decl: " << obj_decl_kind << "\n";
		if (!obj_decl_kind.compare("CXXRecord"))
		  {
		    RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();
		    const SourceLocation decl_type_loc = ((const TypeDecl *)obj_decl)->getLocStart();
		    // Get compound statement start line num
		    const SourceLocation spelling_decl_type_loc = src_mgr.getImmediateSpellingLoc(decl_type_loc);
		    FileID decl_type_loc_fid = src_mgr.getFileID(spelling_decl_type_loc);
		    unsigned decl_type_loc_line_num = src_mgr.getLineNumber(decl_type_loc_fid, src_mgr.getFileOffset(spelling_decl_type_loc));
		    outs() << "Object decl type defined in "
			   << src_mgr.getFilename(spelling_decl_type_loc)
			   << " at line "
			   << decl_type_loc_line_num
			   << "\n";
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
			    const SourceLocation spelling_field_end_loc = src_mgr.getImmediateSpellingLoc(field_end_loc);
			    FileID field_loc_fid = src_mgr.getFileID(spelling_field_begin_loc);
			    unsigned field_begin_loc_line_num = src_mgr.getLineNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_begin_loc));
			    unsigned field_begin_loc_col_num = src_mgr.getColumnNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_begin_loc));
			    unsigned field_end_loc_line_num = src_mgr.getLineNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_end_loc));
			    unsigned field_end_loc_col_num = src_mgr.getColumnNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_end_loc));
			    std::string field_file_name = src_mgr.getFilename(spelling_field_begin_loc);
			    outs() << "    defined in " << field_file_name << " starting at linecol #"
				   << field_begin_loc_line_num << ":" << field_begin_loc_col_num
				   << " ending at linecol # "
				   << field_end_loc_line_num << ":" << field_end_loc_col_num
				   << "\n";

                            SourceRange member_range(member_start, member_end);
                            SourceRange member2_range(member2_start, member2_end);
                            SourceRange field_range(field_begin_loc, field_end_loc);
			    std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
			    std::string member2_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member2_range), src_mgr, LangOptions(), 0);
			    std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);
                            
			    //strcpy_call->dump();
			    outs() << "\n";
			    
			    emitDiagAndFix(diagEngine,
					   call_start, call_end,
                                           call_name, member_tokens, member2_tokens, 
                                           field_begin_loc, field_end_loc,
                                           member_name, object_name, field_name, field_tokens);
                          }
                      }
                  }
		else
		  {
                    outs() << "Error: Unexpected AST node kind ! Please handle this new kind: '" << obj_decl_kind << "'!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
                              &obj_decl_kind);
                    break;
		  }
              }
	    while (0);
          }
        /*
         * Handle strlen calls
         */
        else if (strlen_call)
          {
	    do
	      {
		std::string call_name("strlen");
		const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
		const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
		const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

		// Get start/end locations for the statement
		SourceLocation call_start = strlen_call->getLocStart();
		SourceLocation call_end = strlen_call->getLocEnd();

                /* 
                 * Handle unexpected cases
                 */

                // Missing constant array type AST nodes
                if (!arraytype)
                  {
                    outs() << "Error: No member could be found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_ARRAY_TYPE_NOT_FOUND);
                    break;
                  }

                // Missing constant array member
                if (!obj_decl)
                  {
                    outs() << "Error: Member has not found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_RECORD_DECL_NOT_FOUND);
                    break;
                  }

                // Missing structure owning constant array 
                if (!obj_decl->hasDefinition())
                  {
                    outs() << "Error: Member has no definition !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
                    break;
                  }

                // Missing constant constant array member expr
                if (!member_expr)
                  {
                    outs() << "Error: No member could be found !!\n";
                    emitError(diagEngine,
                              call_start,
                              CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
                    break;
                  }

		// Get file ID in src mgr
		FileID start_fid = src_mgr.getFileID(call_start);
		// Get compound statement start line num
		unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));
		unsigned start_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_start));
		unsigned end_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_end));
		unsigned end_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_end));

		const Expr *arg1 = strlen_call->getArg(0);
		SourceLocation member_start = arg1->getLocStart();
		SourceLocation member_end = arg1->getLocEnd();
                
		outs() << "arg1 dump:\n";
		arg1->IgnoreImpCasts()->dump();

		outs() << "member_expr dump:\n";
		member_expr->dump();

		outs() << "Found a strlen call statement from #"
		       << start_line_num << ":" << start_col_num
		       << " to "
		       << end_line_num << ":" << end_col_num << "\n";

		QualType element_type = arraytype->getElementType();
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   ... an array of " << element_type.getAsString() << " elements\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	    
		std::string member_name;
		SourceLocation member_loc;
		DeclarationNameLoc member_decl_name_loc;

		const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
	    
		member_name = info.getName().getAsString();
		member_loc = info.getLoc();
		member_decl_name_loc = info.getInfo();
	    
		const SourceLocation spelling_member_loc = src_mgr.getImmediateSpellingLoc(member_loc);
		FileID member_loc_fid = src_mgr.getFileID(spelling_member_loc);
		unsigned member_loc_line_num = src_mgr.getLineNumber(member_loc_fid, src_mgr.getFileOffset(spelling_member_loc));
            
		outs() << "Member name: " << member_name << "\n";
		outs() << "    declared in " << src_mgr.getFilename(spelling_member_loc) << " at line #" << member_loc_line_num << "\n";

		Expr *base = member_expr->getBase();
		QualType base_type = base->getType();
		std::string object_name = base_type.getAsString();
		QualType member_type = member_expr->getType();

		outs() << " Base Type name = '" << base_type.getAsString() << "'\n";
		outs() << " Member Type name = '" << member_type.getAsString() << "'\n";
		
		DeclContext *decl_ctxt = dynamic_cast<DeclContext*>(obj_decl->getDefinition());
		std::string obj_decl_kind = decl_ctxt->getDeclKindName();
		outs() << "Object decl: " << obj_decl_kind << "\n";
		if (!obj_decl_kind.compare("CXXRecord"))
		  {
		    RecordDecl *definition = (RecordDecl *)obj_decl->getDefinition();
		    const SourceLocation decl_type_loc = ((const TypeDecl *)obj_decl)->getLocStart();
		    // Get compound statement start line num
		    const SourceLocation spelling_decl_type_loc = src_mgr.getImmediateSpellingLoc(decl_type_loc);
		    FileID decl_type_loc_fid = src_mgr.getFileID(spelling_decl_type_loc);
		    unsigned decl_type_loc_line_num = src_mgr.getLineNumber(decl_type_loc_fid, src_mgr.getFileOffset(spelling_decl_type_loc));
		    outs() << "Object decl type defined in "
			   << src_mgr.getFilename(spelling_decl_type_loc)
			   << " at line "
			   << decl_type_loc_line_num
			   << "\n";
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
			    const SourceLocation spelling_field_end_loc = src_mgr.getImmediateSpellingLoc(field_end_loc);
			    FileID field_loc_fid = src_mgr.getFileID(spelling_field_begin_loc);
			    unsigned field_begin_loc_line_num = src_mgr.getLineNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_begin_loc));
			    unsigned field_begin_loc_col_num = src_mgr.getColumnNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_begin_loc));
			    unsigned field_end_loc_line_num = src_mgr.getLineNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_end_loc));
			    unsigned field_end_loc_col_num = src_mgr.getColumnNumber(field_loc_fid, src_mgr.getFileOffset(spelling_field_end_loc));
			    std::string field_file_name = src_mgr.getFilename(spelling_field_begin_loc);
			    outs() << "    defined in " << field_file_name << " starting at linecol #"
				   << field_begin_loc_line_num << ":" << field_begin_loc_col_num
				   << " ending at linecol # "
				   << field_end_loc_line_num << ":" << field_end_loc_col_num
				   << "\n";
			    
                            SourceRange member_range(member_start, member_end);
                            SourceRange field_range(field_begin_loc, field_end_loc);
			    std::string member_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(member_range), src_mgr, LangOptions(), 0);
			    std::string member2_tokens;
			    std::string field_tokens = Lexer::getSourceText(CharSourceRange::getTokenRange(field_range), src_mgr, LangOptions(), 0);
                            
			    //strlen_call->dump();
			    outs() << "\n";
			    
			    emitDiagAndFix(diagEngine,
					   call_start, call_end,
                                           call_name, member_tokens, member2_tokens, 
                                           field_begin_loc, field_end_loc,
                                           member_name, object_name, field_name, field_tokens);
			  }
		      }
		  }
		else
		  {
		    outs() << "Error: Unexpected AST node kind ! Please handle this new kind: '" << obj_decl_kind << "'!\n";
		    emitError(diagEngine,
			      call_start,
			      CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_UNEXPECTED_AST_NODE_KIND,
			      &obj_decl_kind);
		    break;
		  }
	      }
	    while (0);
	  }
      }
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
