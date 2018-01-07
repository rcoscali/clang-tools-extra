//===--- ExecSQLCommon.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _EXECSQLCOMMON_H_
#define _EXECSQLCOMMON_H_

#include <map>
#include <string>

#include "../ClangTidy.h"

// Common constants definitions
#define GENERATION_SOURCE_FILENAME_EXTENSION ".pc"
#define GENERATION_HEADER_FILENAME_EXTENSION ".h"

// Regex values
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE \
  "EXEC[[:space:]]+SQL[[:space:]]+(fetch|FETCH)[[:space:]]*(:?[[:space:]]*[A-Za-z0-9]+)[[:space:]]+(INTO|into)?[[:space:]]*(.*)[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_FILELINE \
  "^(.*)#([0-9]+)$"
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_TMPL_REPEAT_RE \
  "@repeat[[:blank:]]+on[[:blank:]]+([[:alpha:]][[:alnum:]_-]+)[[:blank:]]*{[[:blank:]]*([[:alpha:]][[:alnum:]_-]+)[[:blank:]]*,(.+)*}"
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_TMPL_REPEAT_MEMBERS_RE \
  "[[:blank:]]*(,[[:blank:]]*([[:alpha:]][[:alnum:]_-]+)[[:blank:]]*)+"
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_TMPL_REPEAT_MEMBERS_RE2 \
  ",(([^,]+)|(?R))*$"	// Using PCRE group recursion: don't run with llvm::Regex 

#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_LINE_DEFINE_RE \
  "^#line ([0-9]+) \"(.*)\"$"

// Common types
using namespace clang;
using namespace clang::ast_matchers;
using string2_map = std::map<std::string, std::string>;
using ushort_string_map = std::map<unsigned short, std::map<std::string, std::string>>;  
using map_vector_string = std::map<std::string, std::vector<std::string>>;
using map_replacement_values = std::map<std::string, std::string>;
using map_comment_map_replacement_values = std::map<std::string, std::map<std::string, std::string>>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

    } // !namespace pagesjaunes
  } // !namespace tidy 
}// !namespace clang

#endif /* _EXECSQLCOMMON_H_ */
