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
#define PAGESJAUNES_REGEX_EXEC_SQL_ALLOCATE_REQ_RE                         \
"EXEC[[:space:]]+SQL[[:space:]]+([Aa][Ll][Ll][Oo][Cc][Aa][Tt][Ee])[[:space:]]*(:[[:space:]]*([_A-Za-z][_A-Za-z0-9]+))[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_ALLOCATE_REQ_RE_REQNAME 2

#define PAGESJAUNES_REGEX_EXEC_SQL_FREE_REQ_RE                         \
"EXEC[[:space:]]+SQL[[:space:]]+([Ff][Rr][Ee][Ee])[[:space:]]*(:[[:space:]]*([_A-Za-z][_A-Za-z0-9]+))[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_FREE_REQ_RE_CURSORNAME 2

#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE                         \
  "EXEC[[:space:]]+SQL[[:space:]]+([Ff][Ee][Tt][Cc][Hh])[[:space:]]*(:?[[:space:]]*[_A-Za-z][A-Za-z0-9_]+)[[:space:]]+([Ii][Nn][Tt][Oo])?[[:space:]]*(.*)[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_LOB_CREATE_REQ_RE                   \
  "EXEC[[:space:]]+SQL[[:space:]]+([Ll][Oo][Bb])[[:space:]]+([Cc][Rr][Ee][Aa][Tt][Ee])[[:space:]]+([Tt][Ee][Mm][Pp][Oo][Rr][Aa][Rr][Yy])[[:space:]]*(.*)[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_LOB_FREE_REQ_RE                   \
  "EXEC[[:space:]]+SQL[[:space:]]+([Ll][Oo][Bb])[[:space:]]+([Ff][Rr][Ee][Ee])[[:space:]]+([Tt][Ee][Mm][Pp][Oo][Rr][Aa][Rr][Yy])[[:space:]]*(.*)[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_LOB_OPEN_REQ_RE                   \
  "EXEC[[:space:]]+SQL[[:space:]]+([Ll][Oo][bB])[[:space:]]+([Oo][Pp][Ee][Nn])[[:space:]]+:([A-Za-z0-9]+)[[:space:]]*([Rr][Ee][Aa][Dd] [Oo][Nn][Ll][Yy])?[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_LOB_READ_REQ_RE                      \
  "EXEC[[:space:]]+SQL[[:space:]]+([Ll][Oo][Bb])[[:space:]]+([Rr][Ee][Aa][Dd])[[:space:]]+:([A-Za-z0-9]+)" \
  "[[:space:]]+([Ff][Rr][Oo][Mm])[[:space:]]+:([A-Za-z0-9]+)"           \
  "[[:space:]]+([Ii][Nn][Tt][Oo])[[:space:]]+:([A-Za-z0-9]+)"           \
  "[[:space:]]+([Ww][Ii][Tt][Hh] [Ll][Ee][Nn][Gg][Tt][Hh])[[:space:]]*(.*)[[:space:]]*;"
                               
#define PAGESJAUNES_REGEX_EXEC_SQL_LOB_CLOSE_REQ_RE                   \
  "EXEC[[:space:]]+SQL[[:space:]]+([Ll][Oo][Bb])[[:space:]]+([Cc][Ll][Oo][Ss][Ee])[[:space:]]*(.*)[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_OPEN_REQ_RE                         \
  "EXEC[[:space:]]+SQL[[:space:]]+([Oo][Pp][Ee][Nn])[[:space:]]*([[:space:]]*[_A-Za-z][A-Za-z0-9_]+)[[:space:]]*([Uu][Ss][Ii][Nn][Gg])?[[:space:]]*(.*)[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_OPEN_REQ_RE_REQNAME 2
#define PAGESJAUNES_REGEX_EXEC_SQL_OPEN_REQ_RE_HOSTVARS 4

#define PAGESJAUNES_REGEX_EXEC_SQL_CLOSE_REQ_RE                         \
  "EXEC[[:space:]]+SQL[[:space:]]+([Cc][Ll][Oo][Ss][Ee])[[:space:]]*([[:space:]]*[_A-Za-z][A-Za-z0-9_]+)[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_DECLARE_REQ_RE                       \
  "EXEC[[:space:]]+SQL[[:space:]]+([Dd][Ee][Cc][Ll][Aa][Rr][Ee])[[:space:]]+([_A-Za-z][A-Za-z0-9_]+)[[:space:]]+([Cc][Uu][Rr][Ss][Oo][Rr])" \
  "[[:space:]]+([Ff][Oo][Rr])[[:space:]]+([[:space:]]*[_A-Za-z][A-Za-z0-9_]+)[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_FMTD_REQ_RE                       \
  "EXEC[[:space:]]+SQL[[:space:]]+([Pp][Rr][Ee][Pp][Aa][Rr][Ee])[[:space:]]+([_A-Za-z][A-Za-z0-9_]+)[[:space:]]+" \
  "([Ff][Rr][Oo][Mm])[[:space:]]*(:?[[:space:]]*([_A-Za-z][A-Za-z0-9_]+)[[:space:]]*,?[[:space:]]*)+[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_REQ_RE                       \
  "EXEC[[:space:]]+SQL[[:space:]]+([Pp][Rr][Ee][Pp][Aa][Rr][Ee])[[:space:]]+([_A-Za-z][A-Za-z0-9_]+)[[:space:]]+" \
  "([Ff][Rr][Oo][Mm])[[:space:]]*(:?[[:space:]]*([_A-Za-z][A-Za-z0-9_]+)[[:space:]]*,?[[:space:]]*)+[[:space:]]*;"

#define PAGESJAUNES_REGEX_EXEC_SQL_ALL_FILELINE \
  "^(.*)#([0-9]+)$"

#define PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE \
  "(:([[:space:]]*(([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*(->|[.])?[[:space:]]*)+)(,?)[[:space:]]*)"
#define PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_FULLMATCH 0
#define PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTVAR 2
#define PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTMEMBER 3
#define PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_DEREF 5
#define PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_VARINDIC 6

#define PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE \
  "[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*"
#define PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE_IDENTIFIER  1

#define PAGESJAUNES_REGEX_EXEC_SQL_ALL_TMPL_REPEAT_RE                   \
  "@repeat[[:blank:]]+on[[:blank:]]+([[:alpha:]][[:alnum:]_-]+)[[:blank:]]*{[[:blank:]]*([[:alpha:]][[:alnum:]_-]+)[[:blank:]]*,(.+)*}"

#define PAGESJAUNES_REGEX_EXEC_SQL_ALL_TMPL_REPEAT_MEMBERS_RE           \
  "[[:blank:]]*(,[[:blank:]]*([[:alpha:]][[:alnum:]_-]+)[[:blank:]]*)+"

#define PAGESJAUNES_REGEX_EXEC_SQL_ALL_TMPL_REPEAT_MEMBERS_RE2          \
  ",(([^,]+)|(?R))*$"   // Using PCRE group recursion: don't run with llvm::Regex 

#define PAGESJAUNES_REGEX_EXEC_SQL_ALL_LINE_DEFINE_RE   \
  "^#line ([0-9]+) \"(.*)\"$"

// Common types
using namespace clang;
using namespace clang::ast_matchers;
using string2_map = std::map<std::string, std::string>;
using ushort_string_map = std::map<unsigned short, std::map<std::string, std::string>>;  
using map_vector_string = std::map<std::string, std::vector<std::string>>;
using map_replacement_values = std::map<std::string, std::string>;
using map_comment_map_replacement_values = std::map<std::string, std::map<std::string, std::string>>;
using map_host_vars = std::map<int, std::map<std::string, std::string>>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {
      map_host_vars
      decodeHostVars(const std::string &);
      
      void
      onStartOfTranslationUnit(map_comment_map_replacement_values &);
      
      void
      onEndOfTranslationUnit(map_comment_map_replacement_values &, const std::string &, bool);
      
    } // !namespace pagesjaunes
  } // !namespace tidy 
}// !namespace clang

#endif /* _EXECSQLCOMMON_H_ */
