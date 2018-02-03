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

#include <vector>
#include <map>
#include <string>

#include "../ClangTidy.h"
#include "clang/Tooling/Tooling.h"

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
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_EXECSQL 1
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_REQNAME 2
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_INTO 3
#define PAGESJAUNES_REGEX_EXEC_SQL_FETCH_REQ_RE_INTONAMES 4

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
  "([Ff][Rr][Oo][Mm])[[:space:]]*(.*)[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_FMTD_REQ_RE_PREPARE 1 
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_FMTD_REQ_RE_REQ_NAME 2
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_FMTD_REQ_RE_FROM 3
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_FMTD_REQ_RE_FROM_VARS 4 

#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_REQ_RE                       \
  "EXEC[[:space:]]+SQL[[:space:]]+([Pp][Rr][Ee][Pp][Aa][Rr][Ee])[[:space:]]+([_A-Za-z][A-Za-z0-9_]+)[[:space:]]+" \
  "([Ff][Rr][Oo][Mm])[[:space:]]*(.*)[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_REQ_RE_REQ_PREPARE 1
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_REQ_RE_REQ_NAME 2
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_REQ_RE_REQ_FROM 3
#define PAGESJAUNES_REGEX_EXEC_SQL_PREPARE_REQ_RE_FROM_VARS 4

#define PAGESJAUNES_REGEX_EXEC_SQL_ALL_FILELINE \
  "^(.*)#([0-9]+)$"

#define PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_STARTSTR \
  "(EXEC[[:space:]]+SQL[[:space:]]+"
#define PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_SPACE_RPLTSTR \
  "[[:space:]]*"
#define PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_COMMA_RPLTSTR \
  ",[[:space:]]*"
#define PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_ENDSTR \
  ")[[:space:]]*;"
#define PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_COMMENT_GROUP 1

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
	/*
	 * sprintf finder collector in case of a prepare request
	 * Literal must be directly copied in the :reqName var
	 */
	struct VarDeclMatchRecord
	{
	  const VarDecl *varDecl;
	  unsigned linenum;
	  char dummy1[16];
	  char dummy2[16];
	};

      // Create a param list formatted as a definition
      std::string
      createParamsDef(const std::string&,
                      const std::string&,
                      const std::string&,
                      const std::string&);

      // Create a patram list formatted as a EXEC SQL declare section
      std::string
      createParamsDeclareSection(const std::string&,
                                 const std::string&,
                                 const std::string&,
                                 const std::string&,
                                 const std::string&);

      // Create a param list formatted as a declaration
      std::string
      createParamsDecl(const std::string&,
                       const std::string&,
                       const std::string&);
        
      // Create a list of params formatted for a function call
      std::string
      createParamsCall(const std::string&);
        
      // Create a host list var formatted for a using clause of a request
      std::string
      createHostVarList(const std::string&, bool);
        
      // Find a symbol, its definition and line number in the current function
      const VarDecl *
      findSymbolInFunction(MatchFinder::MatchCallback&,
                           ClangTool*,
                           std::string&,
                           const FunctionDecl*,
                           std::vector<struct clang::tidy::pagesjaunes::VarDeclMatchRecord *>&);

      // Find a symbol defined in a function
      string2_map
      findDeclInFunction(const FunctionDecl *, const std::string&);

      // Find a member of a struct/class defined in the translation unit
      string2_map
      findCXXRecordMemberInTranslationUnit(const TranslationUnitDecl *transUnit,
                                           const std::string& cxxRecordName,
                                           const std::string& memberName);
      
      // This function decode an input string of host variables (and indicators).
      map_host_vars
      decodeHostVars(const std::string &);

      // Create a backup file for file pathname provided
      void
      createBackupFile(const std::string&);

      // Split a buffer in a vector of lines
      std::vector<std::string>
      bufferSplit(char *, std::vector<std::string>::size_type& linesnr, std::vector<std::string>::size_type reserve = 0, bool start_at_0 = true);

      // Read a text file and returns size & contents
      const char *
      readTextFile(const char *, std::size_t&);
        
      // CB called at start of processing of translation unit
      void
      onStartOfTranslationUnit(map_comment_map_replacement_values &);

      // CB called at end of processing of translation unit
      void
      onEndOfTranslationUnit(map_comment_map_replacement_values &, const std::string &, bool);
      
    } // !namespace pagesjaunes
  } // !namespace tidy 
}// !namespace clang

#endif /* _EXECSQLCOMMON_H_ */
