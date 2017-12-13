//===--- ArgumentNamingCheck.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NAGRAVISION_ARGUMENTNAMINGCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NAGRAVISION_ARGUMENTNAMINGCHECK_H

#include "../ClangTidy.h"
#include "llvm/Support/Regex.h"

namespace clang 
{
  namespace tidy 
  {
    namespace nagravision 
    {

      /// \brief Checks that argument name match parameter name rules.
      class ArgumentNamingCheck : public ClangTidyCheck 
      {
      public:
	ArgumentNamingCheck(StringRef Name, ClangTidyContext *Context);

	void registerMatchers(ast_matchers::MatchFinder *Finder) override;
	void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

      private:
	llvm::Regex xRe;
	llvm::Regex pxRe;
	llvm::Regex ppxRe;

        void emitDiagAndFix(llvm::Regex *re, const DeclRefExpr *DRE, const ParmVarDecl *PVD, const std::string &str);
      };

    } // namespace nagravision
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NAGRAVISION_ARGUMENTNAMINGCHECK_H
