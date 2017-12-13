//===- clang-apply-replacements/ApplyReplacementsTest.cpp
//----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang-apply-replacements/Tooling/ApplyReplacements.h"
#include "gtest/gtest.h"

using namespace clang::replace;
using namespace llvm;

namespace clang {
namespace tooling {

static TUDiagnostics
makeTUDiagnostics(const std::string &MainSourceFile, StringRef DiagnosticName,
                  const DiagnosticMessage &Message,
                  const StringMap<clang::tooling::Replacements> &xReplacements,
                  StringRef BuildDirectory) {
  SmallVector<DiagnosticMessage, 1> notes;
  Diagnostic d(DiagnosticName,
	       (DiagnosticMessage &)Message,
	       (StringMap<clang::tooling::Replacements> &)xReplacements,
	       notes,
	       Diagnostic::Warning,
	       BuildDirectory);
  struct TranslationUnitDiagnostics *tud = (struct TranslationUnitDiagnostics *)malloc(sizeof(struct TranslationUnitDiagnostics));
  tud->MainSourceFile = MainSourceFile;
  tud->Diagnostics.push_back(d);
  TUDiagnostics TUs;
  TUs.push_back(*tud);
  
  return TUs;
}

// Test to ensure diagnostics with no fixes, will be merged correctly
// before applying.
TEST(ApplyReplacementsTest, mergeDiagnosticsWithNoFixes) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts(new DiagnosticOptions());
  DiagnosticsEngine Diagnostics(
      IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), DiagOpts.get());
  FileManager Files((FileSystemOptions()));
  SourceManager SM(Diagnostics, Files);
  TUDiagnostics TUs =
    makeTUDiagnostics("path/to/source.cpp", "diagnostic", {}, {}, "path/to");
  FileToReplacementsMap ReplacementsMap;

  EXPECT_TRUE(mergeAndDeduplicate(TUs, ReplacementsMap, SM));
  EXPECT_TRUE(ReplacementsMap.empty());
}

} // end namespace tooling
} // end namespace clang
