//===--- ArgumentNamingCheck.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ArgumentNamingCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Regex.h"

using namespace clang::ast_matchers;
using namespace llvm;

namespace clang 
{
  namespace tidy 
  {
    namespace nagravision
    {
      ArgumentNamingCheck::ArgumentNamingCheck(StringRef Name,
					       ClangTidyContext *Context)
	: ClangTidyCheck(Name, Context),
	  xRe("^x[_A-Z][_A-Za-z0-9]*$"),
	  pxRe("^px[_A-Z][_A-Za-z0-9]*$"),
	  ppxRe("^ppx[_A-Z][_A-Za-z0-9]*$")
      {}

      void 
      ArgumentNamingCheck::registerMatchers(MatchFinder *Finder) 
      {
	Finder->addMatcher(functionDecl().bind("function"), this);
	Finder->addMatcher(cxxMethodDecl().bind("method"), this);
        Finder->addMatcher(declRefExpr(hasAncestor(functionDecl().bind("functionparent"))).bind("reference"), this);
      }

      void
      ArgumentNamingCheck::emitDiagAndFix(Regex *re,
                                          const DeclRefExpr *DRE,
                                          const ParmVarDecl *PVD,
                                          const std::string &str)
      {
        if (re->match(PVD->getName()))
          return;
        
        std::string name = PVD->getNameAsString();
        name[0] = toupper(name[0]);
        DiagnosticBuilder Diag = diag(DRE ? DRE->getLocEnd() : PVD->getLocEnd(),
                                      "parameter name '%0' does not follow naming rules") << PVD->getNameAsString();
        Diag << FixItHint::CreateReplacement(DRE ? DRE->getLocEnd() : PVD->getLocEnd(), str + name);  
      }

      void
      ArgumentNamingCheck::check(const MatchFinder::MatchResult &Result) 
      {
        const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("function");
        const CXXMethodDecl *MD = Result.Nodes.getNodeAs<CXXMethodDecl>("method");
        const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("reference");
        ArrayRef<ParmVarDecl*> params;

        if (DRE)
          {
            const FunctionDecl *PFD = Result.Nodes.getNodeAs<FunctionDecl>("functionparent");
            if (PFD)
              {
                const ValueDecl *vdecl = DRE->getDecl();
                if (isa<ParmVarDecl>(*vdecl))
                  {
                    const ParmVarDecl *PVD = static_cast<const ParmVarDecl *>(vdecl);
                    QualType T = PVD->getType();

                    if (PVD->getNameAsString().length())
                      {
                        if (T.getTypePtr()->isPointerType())
                          {
                            const QualType pointee = T.getTypePtr()->getPointeeType();
                            if (pointee.getTypePtr()->isPointerType())
                              emitDiagAndFix(&ppxRe, DRE, PVD, "ppx");

                            else
                              emitDiagAndFix(&pxRe, DRE, PVD, "px");
                          }
                        else
                          emitDiagAndFix(&xRe, DRE, PVD, "x");
                      }
                  }
              }
          }
        else
          {
            if (FD)
              params = FD->parameters();

            if (MD)
              params = MD->parameters();

            if (!params.empty())
              {
                for (ArrayRef<ParmVarDecl *>::size_type idx = 0; idx < params.size(); idx++)
                  {
                    const ParmVarDecl *PVD = params[idx];
                    const QualType T = PVD->getType();

                    if (PVD->getNameAsString().length())
                      {
                        if (T.getTypePtr()->isPointerType())
                          {
                            const QualType pointee = T.getTypePtr()->getPointeeType();
                            if (pointee.getTypePtr()->isPointerType())
                              emitDiagAndFix(&ppxRe, DRE, PVD, "ppx");

                            else
                              emitDiagAndFix(&pxRe, DRE, PVD, "px");
                          }
                        else
                          emitDiagAndFix(&xRe, DRE, PVD, "x");
                      }
                  }
              }
          }
      }
      
    } // namespace nagravision
  } // namespace tidy
} // namespace clang
