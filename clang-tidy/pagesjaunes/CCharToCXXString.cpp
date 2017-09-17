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
	: ClangTidyCheck(Name, Context)
      {}

      void 
      CCharToCXXString::registerMatchers(MatchFinder *Finder) 
      {
	// Bind a variable declaration (ex. local variable)
	Finder->addMatcher(varDecl(hasParent(declStmt().bind("decl")),
				   hasType(constantArrayType(hasElementType(builtinType().bind("chartype"),
									    isAnyCharacter())).bind("arraytype"))
				   ).bind("var")
			   , this);
	// Bind a strcmp call with a constant char array
	Finder->addMatcher(callExpr(callee(functionDecl(hasName("strcmp"))),
				    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType().bind("chartype"),
												      isAnyCharacter())).bind("arraytype")),
							     hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
				    ).bind("strcmp_call")
			   , this);
	// Bind a strcpy call with a constant char array
	Finder->addMatcher(callExpr(callee(functionDecl(hasName("strcpy"))),
				    hasDescendant(memberExpr(hasType(constantArrayType(hasElementType(builtinType().bind("chartype"),
												      isAnyCharacter())).bind("arraytype")),
							     hasObjectExpression(hasType(cxxRecordDecl().bind("obj_decl")))).bind("member_expr"))
				    ).bind("strcpy_call")
			   , this);
      }

      void
      CCharToCXXString::emitDiagAndFix()
      {
      }

      void
      CCharToCXXString::check(const MatchFinder::MatchResult &Result) 
      {
	// Get root bounded variables
	const DeclStmt *decl =  Result.Nodes.getNodeAs<DeclStmt>("decl");
	const CallExpr *strcmp_call = Result.Nodes.getNodeAs<CallExpr>("strcmp_call");
	const CallExpr *strcpy_call = Result.Nodes.getNodeAs<CallExpr>("strcpy_call");

	// Get the source manager
	SourceManager &src_mgr = Result.Context->getSourceManager();

	// We got a Decl
	if (decl)
	  {
	    const QualType *chartype = Result.Nodes.getNodeAs<QualType>("chartype");
	    const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
	    const VarDecl *var = Result.Nodes.getNodeAs<VarDecl>("var");
	    
	    // Get start/end locations for the statement
	    SourceLocation stmt_start = decl->getLocStart();

	    // Get file ID in src mgr
	    FileID start_fid = src_mgr.getFileID(stmt_start);
	    // Get compound statement start line num
	    unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(stmt_start));

	    outs() << "Found a var decl statement at line #" << start_line_num << "\n";

	    if (chartype)
	      {
		outs() << "Element Qualified Type is '" << chartype->getAsString() << "'\n"; 
	      }

	    if (arraytype)
	      {
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	      }

	    if (var)
	      {
		outs() << "Var is named '" << var->getNameAsString() << "'\n";
		var->print(llvm::outs(), 4, true);
	      }

	    decl->dump();
	    outs() << "\n";
	  }
	else if (strcmp_call)
	  {
	    const QualType *chartype = Result.Nodes.getNodeAs<QualType>("chartype");
	    const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
	    const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
	    const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

	    // Get start/end locations for the statement
	    SourceLocation call_start = strcmp_call->getLocStart();

	    // Get file ID in src mgr
	    FileID start_fid = src_mgr.getFileID(call_start);
	    // Get compound statement start line num
	    unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));

	    outs() << "Found a strcmp call statement at line #" << start_line_num << "\n";

	    if (chartype)
	      {
		outs() << "Element Qualified Type is '" << chartype->getAsString() << "'\n"; 
	      }

	    if (arraytype)
	      {
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	      }

	    if (obj_decl)
	      {
		if (obj_decl->hasDefinition())
		  outs() << "Object decl: " << ((Decl *)(obj_decl->getDefinition()))->getDeclKindName() << "\n";
	      }

	    if (member_expr)
	      {
		const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
		const Expr* base_expr = member_expr->getBase();
		
		outs() << "Member name: " << info.getName().getAsString() << "\n";
	      }

	    strcmp_call->dump();
	    outs() << "\n";
	  }
	else if (strcpy_call)
	  {
	    const QualType *chartype = Result.Nodes.getNodeAs<QualType>("chartype");
	    const ConstantArrayType *arraytype = Result.Nodes.getNodeAs<ConstantArrayType>("arraytype");
	    const CXXRecordDecl *obj_decl = Result.Nodes.getNodeAs<CXXRecordDecl>("obj_decl");
	    const MemberExpr *member_expr = Result.Nodes.getNodeAs<MemberExpr>("member_expr");

	    // Get start/end locations for the statement
	    SourceLocation call_start = strcpy_call->getLocStart();

	    // Get file ID in src mgr
	    FileID start_fid = src_mgr.getFileID(call_start);
	    // Get compound statement start line num
	    unsigned start_line_num = src_mgr.getLineNumber(start_fid, src_mgr.getFileOffset(call_start));

	    outs() << "Found a strcpy call statement at line #" << start_line_num << "\n";

	    if (chartype)
	      {
		outs() << "Element Qualified Type is '" << chartype->getAsString() << "'\n"; 
	      }

	    if (arraytype)
	      {
		outs() << "Array Type is '" << arraytype->getTypeClassName() << "'\n";
		outs() << "   and its size is " << arraytype->getSize() << "\n";
	      }

	    if (obj_decl)
	      {
		if (obj_decl->hasDefinition())
		  outs() << "Object decl: " << ((Decl *)(obj_decl->getDefinition()))->getDeclKindName() << "\n";
	      }

	    if (member_expr)
	      {
		const DeclarationNameInfo& info = member_expr->getMemberNameInfo();
		const Expr* base_expr = member_expr->getBase();
		
		outs() << "Member name: " << info.getName().getAsString() << "\n";
	      }

	    strcpy_call->dump();
	    outs() << "\n";
	  }
      }
      
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
