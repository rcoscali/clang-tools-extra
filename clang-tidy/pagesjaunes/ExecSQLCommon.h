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

// Common types
using namespace clang;
using namespace clang::ast_matchers;
using string2_map = std::map<std::string, std::string>;
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
