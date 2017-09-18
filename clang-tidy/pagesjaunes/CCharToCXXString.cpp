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
	  member_has_no_def_diag_id(Context->
				    getCustomDiagID(DiagnosticsEngine::Error,
						    "Member has no definition !")),
	  member_not_found_diag_id(Context->
				   getCustomDiagID(DiagnosticsEngine::Error,
						   "Could not bind the member expression !"))
      {}

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

      void
      CCharToCXXString::emitDiagAndFix(const SourceLocation& call_start, const SourceLocation& call_end,
				       std::string function_name,
				       const SourceLocation& def_start, const SourceLocation& def_end,
				       std::string member_name)
      {
	SourceRange call_range(call_start, call_end);
	SourceRange def_range(def_start, def_end);

	DiagnosticBuilder mydiag = diag(call_start,
					"This string manipulation call shall be replaced with std::string equivalent");

	std::string replt_code;
	std::string replt_def;
	replt_code.append("strcmp");
	replt_def.append("std::string ");
	mydiag << FixItHint::CreateReplacement(call_range, replt_code);
	mydiag << FixItHint::CreateReplacement(def_range, replt_def);
      }

      void
      CCharToCXXString::emitError(DiagnosticsEngine &diag_engine,
				  const SourceLocation& err_loc,
				  enum CCharToCXXStringErrorKind kind,
				  std::string *msg)
      {
	switch (kind)
	  {
	  default:
	    {
	      DiagnosticBuilder diag = diag_engine.Report(err_loc, unexpected_diag_id);
	      if (!msg->empty())
		diag.AddString(*msg);
	    }
	    break;

	  case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_NO_ERROR:
	    {
	      DiagnosticBuilder diag = diag_engine.Report(err_loc, no_error_diag_id);
	      if (!msg->empty())
		diag.AddString(*msg);
	    }
	    break;

	  case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF:
	    {
	      DiagnosticBuilder diag = diag_engine.Report(err_loc, member_has_no_def_diag_id);
	      if (!msg->empty())
		diag.AddString(*msg);
	    }
	    break;

	  case CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND:
	    {
	      DiagnosticBuilder diag = diag_engine.Report(err_loc, member_not_found_diag_id);
	      if (!msg->empty())
		diag.AddString(*msg);
	    }
	    break;
	  }
      }

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

	if (strcmp_call)
	  {
	    const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
	    const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
	    const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

	    // Get start/end locations for the statement
	    SourceLocation call_start = strcmp_call->getLocStart();
	    SourceLocation call_end = strcmp_call->getLocEnd();

	    // Get file ID in src mgr
	    FileID start_fid = src_mgr.getFileID(call_start);
	    // Get compound statement start line num
	    unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));
	    unsigned start_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_start));
	    unsigned end_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_end));
	    unsigned end_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_end));

	    outs() << "Found a strcmp call statement from #"
		   << start_line_num << ":" << start_col_num
		   << " to "
		   << end_line_num << ":" << end_col_num << "\n";

	    if (arraytype)
	      {
		QualType element_type = arraytype->getElementType();
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   ... an array of " << element_type.getAsString() << " elements\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	      }

	    std::string member_name;
	    SourceLocation member_loc;
	    DeclarationNameLoc member_decl_name_loc;

	    if (obj_decl)
	      {
		if (obj_decl->hasDefinition())
		  {
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

			if (member_expr)
			  {
			    const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
			    
			    member_name = info.getName().getAsString();
			    member_loc = info.getLoc();
			    member_decl_name_loc = info.getInfo();
			    
			    const SourceLocation spelling_member_loc = src_mgr.getImmediateSpellingLoc(member_loc);
			    FileID member_loc_fid = src_mgr.getFileID(spelling_member_loc);
			    unsigned member_loc_line_num = src_mgr.getLineNumber(member_loc_fid, src_mgr.getFileOffset(spelling_member_loc));
			    
			    outs() << "Member name: " << member_name << "\n";
			    outs() << "    declared in " << src_mgr.getFilename(spelling_member_loc) << " at line #" << member_loc_line_num << "\n";
			    outs() << "    base expr is:\n";

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
				    emitDiagAndFix(call_start, call_end,
						   std::string("strcmp"),
						   field_begin_loc, field_end_loc,
						   member_name);
				  }
			      }
			  }
			else
			  {
			    outs() << "Error: No member could be found !!\n";
			    emitError(diagEngine,
				      call_start,
				      CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_NOT_FOUND);
			  }
		      }
		    else
		      {
			outs() << "Error: Member has no definition !!\n";
			emitError(diagEngine,
				  call_start,
				  CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
		      }
		  }
		else
		  {
		    outs() << "Error: Member has no definition !!\n";
		    emitError(diagEngine,
			      call_start,
			      CCharToCXXString::CCHAR_2_CXXSTRING_ERROR_MEMBER_HAS_NO_DEF);
		  }
	      }

	    //strcmp_call->dump();
	    outs() << "\n";
	  }
	else if (strcpy_call)
	  {
	    const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
	    const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
	    const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

	    // Get start/end locations for the statement
	    SourceLocation call_start = strcpy_call->getLocStart();
	    SourceLocation call_end = strcpy_call->getLocEnd();

	    // Get file ID in src mgr
	    FileID start_fid = src_mgr.getFileID(call_start);
	    // Get compound statement start line num
	    unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));
	    unsigned start_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_start));
	    unsigned end_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_end));
	    unsigned end_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_end));

	    outs() << "Found a strcpy call statement from #"
		   << start_line_num << ":" << start_col_num
		   << " to "
		   << end_line_num << ":" << end_col_num << "\n";

	    if (arraytype)
	      {
		QualType element_type = arraytype->getElementType();
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   ... an array of " << element_type.getAsString() << " elements\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	      }

	    std::string member_name;
	    SourceLocation member_loc;
	    DeclarationNameLoc member_decl_name_loc;
	    if (member_expr)
	      {
		const DeclarationNameInfo& info = member_expr->getMemberNameInfo();

		member_name = info.getName().getAsString();
		member_loc = info.getLoc();
		member_decl_name_loc = info.getInfo();

		const SourceLocation spelling_member_loc = src_mgr.getImmediateSpellingLoc(member_loc);
		FileID member_loc_fid = src_mgr.getFileID(spelling_member_loc);
		unsigned member_loc_line_num = src_mgr.getLineNumber(member_loc_fid, src_mgr.getFileOffset(spelling_member_loc));
		
		outs() << "Member name: " << member_name << "\n";
		outs() << "    declared in " << src_mgr.getFilename(spelling_member_loc) << " at line #" << member_loc_line_num << "\n";
		outs() << "    base expr is:\n";
	      }

	    if (obj_decl)
	      {
		if (obj_decl->hasDefinition())
		  {
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
				emitDiagAndFix(call_start, call_end,
					       std::string("strcpy"),
					       field_begin_loc, field_end_loc,
					       member_name);
			      }
			  }
		      }
		  }
	      }

	    //strcpy_call->dump();
	    outs() << "\n";
	  }
	else if (strlen_call)
	  {
	    const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
	    const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
	    const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

	    // Get start/end locations for the statement
	    SourceLocation call_start = strlen_call->getLocStart();
	    SourceLocation call_end = strcpy_call->getLocEnd();

	    // Get file ID in src mgr
	    FileID start_fid = src_mgr.getFileID(call_start);
	    // Get compound statement start line num
	    unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));
	    unsigned start_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_start));
	    unsigned end_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_end));
	    unsigned end_col_num = src_mgr.getColumnNumber(start_fid, src_mgr.getFileOffset(call_end));

	    outs() << "Found a strlen call statement from #"
		   << start_line_num << ":" << start_col_num
		   << " to "
		   << end_line_num << ":" << end_col_num << "\n";

	    if (arraytype)
	      {
		QualType element_type = arraytype->getElementType();
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   ... an array of " << element_type.getAsString() << " elements\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	      }

	    std::string member_name;
	    SourceLocation member_loc;
	    DeclarationNameLoc member_decl_name_loc;
	    if (member_expr)
	      {
		const DeclarationNameInfo& info = member_expr->getMemberNameInfo();

		member_name = info.getName().getAsString();
		member_loc = info.getLoc();
		member_decl_name_loc = info.getInfo();

		const SourceLocation spelling_member_loc = src_mgr.getImmediateSpellingLoc(member_loc);
		FileID member_loc_fid = src_mgr.getFileID(spelling_member_loc);
		unsigned member_loc_line_num = src_mgr.getLineNumber(member_loc_fid, src_mgr.getFileOffset(spelling_member_loc));
		
		outs() << "Member name: " << member_name << "\n";
		outs() << "    declared in " << src_mgr.getFilename(spelling_member_loc) << " at line #" << member_loc_line_num << "\n";
		outs() << "    base expr is:\n";
	      }

	    if (obj_decl)
	      {
		if (obj_decl->hasDefinition())
		  {
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
				emitDiagAndFix(call_start, call_end,
					       std::string("strlen"),
					       field_begin_loc, field_end_loc,
					       member_name);
			      }
			  }
		      }
		  }
	      }

	    //strlen_call->dump();
	    outs() << "\n";
	  }
      }
      
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
