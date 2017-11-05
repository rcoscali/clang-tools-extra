//===--- DeIncludePreProC.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_UNINCLUDE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_UNINCLUDE_H

#include "../ClangTidy.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Regex.h"
#include "clang/Basic/Diagnostic.h"

#include <string>
#include <vector>

using namespace clang;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      // Checks that argument name match parameter name rules.
      class DeIncludePreProC : public ClangTidyCheck 
      {
      public:
	// Constructor
	DeIncludePreProC(StringRef, ClangTidyContext *);

	// Store check Options
	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	// Register matchers
	void registerMatchers(ast_matchers::MatchFinder *) override;
	// Check a found matched node
	void check(const ast_matchers::MatchFinder::MatchResult &) override;

      private:

	// Emit diagnostic and eventually fix it
        void emitDiagAndFix(const SourceLocation&,
			    const SourceLocation&,
			    const std::string&);

	// Kind of errors processed by emitError
	enum DeIncludePreProCErrorKind
	  {
	    // No error
	    DE_INCLUDE_PRE_PROC_ERROR_NO_ERROR = 0,
	    // Error while accessing memory buffer data
	    DE_INCLUDE_PRE_PROC_ERROR_ACCESS_CHAR_DATA,
	    // Error on finding an associated comment
	    DE_INCLUDE_PRE_PROC_ERROR_CANT_FIND_COMMENT_START,
	    // Error on parsing comment with regex
	    DE_INCLUDE_PRE_PROC_ERROR_COMMENT_DONT_MATCH,
	  };

	// Emit error
	void emitError(DiagnosticsEngine&,
		       const SourceLocation&,
		       enum DeIncludePreProCErrorKind);

	bool contain(std::vector<std::string>&, std::string&);

	std::vector<std::string> tokenizeString(const std::string&,
						const std::string&);

	unsigned int count_buffer_chars_number (const std::string&,
						const char*);
	// AST Context instance
	ClangTidyContext *TidyContext;

	// Headers to include in matches
	std::vector<std::string> toIncludeIn;
	
	// Headers to exclude from matches
	std::vector<std::string> toExcludeFrom;

	// Headers include dirs vector
	std::vector<std::string> headersDirectories;

	/*
	 * Check Options
	 */
	// The regex used for parsing 'include' comments
	const std::string comment_regex;
	// String with names of headers to include in
	const std::string headers_to_include_in;
	// String with names of headers to exclude from
	const std::string headers_to_exclude_from;
	// Vector for include dirs
	const std::string headers_directories;

      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_UNINCLUDE_H
