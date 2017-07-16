//===--- NagravisionTidyModule.cpp - clang-tidy ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "ArgumentNamingCheck.h"

namespace clang 
{
  namespace tidy 
  {
    namespace nagravision 
    {

      class NagravisionModule : public ClangTidyModule 
      {
      public:
	void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override 
	{
	  CheckFactories.registerCheck<ArgumentNamingCheck> ("nagravision-argument-naming");
	}
      };

      // Register the NagravisionModule using this statically initialized variable.
      static ClangTidyModuleRegistry::Add<NagravisionModule>X("nagravision-module", 
							      "Adds Nagravision coding rules related checks.");
      
    } // namespace nagravision

    // This anchor is used to force the linker to link in the generated object file
    // and thus register the NagravisionModule.
    volatile int NagravisionModuleAnchorSource = 0;

  } // namespace tidy
} // namespace clang
