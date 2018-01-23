//===--- ExecSQLCommon.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <sstream>
#include <string>

#include "ExecSQLCommon.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Regex.h"

// These are usefull only for unitary tests
//#define ACTIVATE_TRACES 
//#define ACTIVATE_RESULT_TRACES 

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using emplace_ret_t = std::pair<std::set<std::string>::iterator, bool>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      /**
       * decodeHostVars
       *
       * This function decode an input string of host variables (and indicators). It
       * parse the string and return a data structure made of maps containing host
       * variables. 
       * It supports pointers and struct dereferencing and returns values of record 
       * or struct variables, members, indicators.
       * It trims record and member values.
       * The first level of map contains the number of the host variable as key and a map
       * for the host variable description.
       *
       *  #1 -> map host var #1
       *  #2 -> map host var #2
       *  #3 -> map host var #3
       *  ...
       *  #n -> map host var #n
       *
       * Each host var map contains some string keys for each variable component.
       * They are:
       *  - "full": the full match for this host variable (contains spaces and is the exact value matched)
       *  - "hostvar": the complete host var referencing expr (spaces were trimmed)
       *  - "hostrecord": the value of the host variable record/struct variable value.
       *  - "hostmember": the value of the record/struct member for this host variable expr.
       *  - "deref": Dereferencing operator for this host variable. Empty if not a dereferencing expr.  
       * 
       * For indicators the same field/keys are available with an 'i' appended for 'indicators'.
       * Unitary tests are availables in test/decode_host_vars.cpp
       * For example: the expr ':var1:Ivar1, :var2:Ivar2'
       * returns the following maps
       *
       * var #1                          var #2
       *     full = ':var1'                 full = ':var2'
       *     fulli = ':Ivar1, '             fulli = ':Ivar2'      
       *     hostvar = 'var1'               hostvar = 'var2'         
       *     hostvari = 'Ivar1'             hostvari = 'Ivar2'          
       *     hostrecord = 'var1'            hostrecord = 'var2'            
       *     hostrecordi = 'Ivar1'          hostrecordi = 'Ivar2'             
       *     hostmember = 'var1'            hostmember = 'var2'         
       *     hostmemberi = 'Ivar1'          hostmemberi = 'Ivar2'              
       *     deref = ''                     deref = ''      
       *     derefi = ''                    derefi = ''         
       *                             
       * @param[in]  hostVarList the host vars expr to parse/decode
       *
       * @return a map containing the host parsed variables expr
       */
      map_host_vars 
      decodeHostVars(const std::string &hostVarList)
      {
        map_host_vars retmap;
        StringRef hostVars(hostVarList);
        Regex hostVarsRe(PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE);
        SmallVector<StringRef, 8> matches;

        int n = 0;
        bool indicator = false;
        std::map<std::string, std::string> var;
        while (hostVarsRe.match(hostVars, &matches))
          {
#ifdef ACTIVATE_TRACES
            std::cout << "N matches = " << matches.size() << "\n";
            for (unsigned int ni = 0; ni < matches.size(); ni++)
              std::cout << "matches[" << ni << "] = " << matches[ni].str() << "\n";            
#endif // !ACTIVATE_TRACES
            if (! indicator)
              {
                size_t derefpos;
                var["full"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_FULLMATCH];
                var["hostvar"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTVAR];
                var["hostmember"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTMEMBER];
                if ((derefpos = var["hostvar"].find("->")) != std::string::npos)
                  var["deref"] = "->";
                else if ((derefpos = var["hostvar"].find(".")) != std::string::npos)
                  var["deref"] = ".";
                if (!var["deref"].empty())
                  var["hostrecord"] = var["hostvar"].substr(0, derefpos);
                else
                  var["hostrecord"] = var["hostmember"];
                Regex trimIdentifierRe(PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE);
                SmallVector<StringRef, 8> idmatches;
                if (trimIdentifierRe.match(var["hostrecord"], &idmatches))
                  var["hostrecord"] = idmatches[PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE_IDENTIFIER];
              }
            else
              {
                size_t derefipos;
                var["fulli"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_FULLMATCH];
                var["hostvari"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTVAR];
                var["hostmemberi"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTMEMBER];
                if ((derefipos = var["hostvari"].find("->")) != std::string::npos)
                  var["derefi"] = "->";
                else if ((derefipos = var["hostvari"].find(".")) != std::string::npos)
                  var["derefi"] = ".";
#ifdef ACTIVATE_TRACES
                std::cout << "compare hosti/memberi = " << var["hostmemberi"].compare(var["hostvari"]) << "\n";
                std::cout << "derefi not empty = " << (!var["derefi"].empty() ? "yes" : "no") << "\n";
#endif // !ACTIVATE_TRACES
                if (!var["derefi"].empty())
                  var["hostrecordi"] = var["hostvari"].substr(0, derefipos);
                else
                  var["hostrecordi"] = var["hostmemberi"];
                Regex trimIdentifierRe(PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE);
                SmallVector<StringRef, 8> idmatches;
                if (trimIdentifierRe.match(var["hostrecordi"], &idmatches))
                  var["hostrecordi"] = idmatches[PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE_IDENTIFIER];
              }

#ifdef ACTIVATE_RESULT_TRACES
            std::cout << "match #" << n << " " << (indicator ? "HostVarIndicator" : "HostVar") << "\n";
            std::cout << "full : '" << var["full"] << "'\n";
            std::cout << "var : '" << var["hostvar"] << "'\n";
            std::cout << "member : '" << var["hostmember"] << "'\n";
            std::cout << "record : '" << var["hostrecord"] << "'\n";
            std::cout << "deref : '" << var["deref"] << "'\n";
            std::cout << "fullI : '" << var["fulli"] << "'\n";
            std::cout << "varI : '" << var["hostvari"] << "'\n";
            std::cout << "memberI : '" << var["hostmemberi"] << "'\n";
            std::cout << "recordI : '" << var["hostrecordi"] << "'\n";
            std::cout << "derefI : '" << var["derefi"] << "'\n";
            std::cout << "varindic comma : '" << matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_VARINDIC].str() << "'\n";
#endif // !ACTIVATE_TRACES
            if (matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_VARINDIC].empty())
              {
                indicator = true;
              }
            else
              {
                retmap.emplace(++n, var);
                var.clear();
                indicator = false;
              }
            size_t startpos = hostVars.find(matches[0]);
            size_t poslength = matches[0].size();
            hostVars = hostVars.substr(startpos + poslength);
          }
        if (var.size() > 0)
          retmap.emplace(++n, var);

#ifdef ACTIVATE_TRACES
        std::cout << "Total vars = " << retmap.size() << "\n";
        for (auto it = retmap.begin(); it != retmap.end(); it++)
          {
            int n = it->first;
            auto v = it->second;
            std::cout << "var #" << n << "\n";
            std::cout << "    full = '" << v["full"] << "'\n";
            std::cout << "    fulli = '" << v["fulli"] << "'\n";
            std::cout << "    hostvar = '" << v["hostvar"] << "'\n";
            std::cout << "    hostvari = '" << v["hostvari"] << "'\n";
            std::cout << "    hostrecord = '" << v["hostrecord"] << "'\n";
            std::cout << "    hostrecordi = '" << v["hostrecordi"] << "'\n";
            std::cout << "    hostmember = '" << v["hostmember"] << "'\n";
            std::cout << "    hostmemberi = '" << v["hostmemberi"] << "'\n";
            std::cout << "    deref = '" << v["deref"] << "'\n";
            std::cout << "    derefi = '" << v["derefi"] << "'\n";
            
          }
#endif // !ACTIVATE_TRACES
        
        return retmap;
      }

      /**
       * onStartOfTranslationUnit
       *
       * @brief called at start of processing of translation unit
       *
       * @param[in] replacement_per_comment   a map containing all key/value pairs for 
       *                                      replacing in original .pc file
       *
       * Override to be called at start of translation unit
       */
      void
      onStartOfTranslationUnit(map_comment_map_replacement_values &replacement_per_comment)
      {
        // Clear the map for comments location in original file
        replacement_per_comment.clear();
      }
      
      /**
       * onEndOfTranslationUnit
       *
       * @brief called at end of processing of translation unit
       *
       * @param[in] replacement_per_comment   a map containing all key/value pairs for 
       *                                      replacing in original .pc file
       *
       * Override to be called at end of translation unit
       */
      void
      onEndOfTranslationUnit(map_comment_map_replacement_values &replacement_per_comment,
                             const std::string &generation_report_modification_in_dir,
                             bool generation_do_keep_commented_out_exec_sql)
      {
        // Get data from processed requests
        for (auto it = replacement_per_comment.begin(); it != replacement_per_comment.end(); ++it)
          {
            std::string execsql;
            std::string fullcomment;
            std::string funcname;
            std::string originalfile;
            std::string filename;
            unsigned int line = 0;
            std::string reqname;
            std::string rpltcode;
            unsigned int pcLineNumStart = 0;
            unsigned int pcLineNumEnd = 0;
            std::string pcFilename;
            bool has_pcFilename = false;
            bool has_pcLineNum = false;
            bool has_pcFileLocation = false;
        
            std::string comment = it->first;
            auto map_for_values = it->second;

            for (auto iit = map_for_values.begin(); iit != map_for_values.end(); ++iit)
              {
                std::string key = iit->first;
                std::string val = iit->second;

                if (key.compare("execsql") == 0)
                  execsql = val;

                else if (key.compare("fullcomment") == 0)
                  fullcomment = val;

                else if (key.compare("funcname") == 0)
                  funcname = val;

                else if (key.compare("originalfile") == 0)
                  {
                    Regex fileline(PAGESJAUNES_REGEX_EXEC_SQL_ALL_FILELINE);
                    SmallVector<StringRef, 8> fileMatches;

                    if (fileline.match(val, &fileMatches))
                      {
                        originalfile = fileMatches[1];
                        std::istringstream isstr;
                        std::stringbuf *pbuf = isstr.rdbuf();
                        pbuf->str(fileMatches[2]);
                        isstr >> line;
                      }

                    else
                      originalfile = val;

                  }
                else if (key.compare("reqname") == 0)
                  reqname = val;

                else if (key.compare("rpltcode") == 0)
                  rpltcode = val;

                else if (key.compare("pclinenumstart") == 0)
                  {
                    std::istringstream isstr;
                    std::stringbuf *pbuf = isstr.rdbuf();
                    pbuf->str(val);
                    isstr >> pcLineNumStart;
                    has_pcLineNum = true;
                  }
                else if (key.compare("pclinenumend") == 0)
                  {
                    std::istringstream isstr;
                    std::stringbuf *pbuf = isstr.rdbuf();
                    pbuf->str(val);
                    isstr >> pcLineNumEnd;
                    has_pcLineNum = true;
                  }
                else if (key.compare("pcfilename") == 0)
                  {
                    pcFilename = val;
                    has_pcFilename = true;
                  }                
              }

            if (has_pcLineNum && has_pcFilename)
              has_pcFileLocation = true;

            char* buffer = nullptr;
            StringRef *Buffer;
            std::string NewBuffer;

            // If #line are not present, need to find the line in file with regex
            if (!has_pcFileLocation)
              {
                // Without #line, we need to compute the filename
                pcFilename = generation_report_modification_in_dir;
                pcFilename.append("/");
                std::string::size_type dot_c_pos = originalfile.find('.');
                pcFilename.append(originalfile.substr(0, dot_c_pos +1));
                pcFilename.append("pc");

                std::ifstream ifs (pcFilename, std::ifstream::in);
                if (ifs.is_open())
                  {
                    std::filebuf* pbuf = ifs.rdbuf();
                    std::size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);

                    if (!size)
                      // TODO: add reported llvm error
                      outs() << originalfile << ":" << line
                             << ":1: warning: Your original file in which to report modifications is empty !\n";

                    else
                      {
                        pbuf->pubseekpos (0,ifs.in);
                        // allocate memory to contain file data
                        buffer=new char[size];
                        
                        // get file data
                        pbuf->sgetn (buffer,size);
                        ifs.close();
                        
                        Buffer = new StringRef(buffer);
                        std::string allocReqReStr = "(EXEC SQL[[:space:]]+";

                        size_t pos = 0;
                        while ((pos = execsql.find(" ")) != std::string::npos )
                          execsql.replace(pos, 1, "[[:space:]]*");
                        pos = -1;
                        while ((pos = execsql.find(",", pos+1)) != std::string::npos )
                          execsql.replace(pos, 1, ",[[:space:]]*");
                        
                        allocReqReStr.append(execsql);
                        allocReqReStr.append(")[[:space:]]*;");
                        Regex allocReqRe(allocReqReStr.c_str(), Regex::NoFlags);
                        SmallVector<StringRef, 8> allocReqMatches;
                        
                        if (allocReqRe.match(*Buffer, &allocReqMatches))
                          {
                            if (generation_do_keep_commented_out_exec_sql)
                              {
                                std::string comment = allocReqMatches[1];
                                comment.append("\n");
                                comment.append(rpltcode);
                                StringRef RpltCode(comment);
                                NewBuffer = allocReqRe.sub(RpltCode, *Buffer);                                
                              }
                            else
                              {
                                StringRef RpltCode(rpltcode);
                                NewBuffer = allocReqRe.sub(RpltCode, *Buffer);
                              }
                          }
                        else
                          {
                            outs() << originalfile << ":" << line
                                   << ":1: warning: Couldn't find 'EXEC SQL " << execsql
                                   << ";' statement to replace with '" << rpltcode
                                   << "' in original '" << pcFilename
                                   << "' file ! Already replaced ?\n";
                            NewBuffer = Buffer->str();
                          }

                        std::ofstream ofs(pcFilename, std::ios::out | std::ios::trunc);
                        if (ofs.is_open())
                          {
                            ofs.write(NewBuffer.c_str(), NewBuffer.size());
                            ofs.flush();
                            ofs.close();
                          }
                        
                        else
                          // TODO: add reported llvm error
                          outs() << originalfile << ":" << line
                                 << ":1: warning: Cannot overwrite file " << pcFilename << " !\n";
                        
                      }
                  }

                else
                  // TODO: add reported llvm error
                  outs() << originalfile << ":" << line
                         << ":1: warning: Cannot open original file in which to report modifications: " << pcFilename
                         << "\n";
              }
            // Line # are generated by preprocessor, let's use it
            else
              {
                std::ifstream ifs (pcFilename, std::ifstream::in);
                if (ifs.is_open())
                  {
                    std::filebuf* pbuf = ifs.rdbuf();
                    std::size_t size = pbuf->pubseekoff (0,ifs.end,ifs.in);
                    pbuf->pubseekpos (0,ifs.in);

                    if (!size)
                      // TODO: add reported llvm error
                      outs() << originalfile << ":" << line
                             << ":1: warning: Original file in which to report modifications is empty !\n";

                    else
                      {
                        // allocate memory to contain file data
                        buffer=new char[size];
                        
                        // get file data
                        pbuf->sgetn (buffer, size);
                        ifs.close();
                        
                        Buffer = new StringRef(buffer);
                        SmallVector<StringRef, 40000> linesbuf;
                        Buffer->split(linesbuf, '\n');

                        // We start counting at 0, file lines are generally counted from 1
                        pcLineNumStart--;
                        pcLineNumEnd--;
                        unsigned int pcStartLineNum = pcLineNumStart;
                        unsigned int pcEndLineNum = pcLineNumEnd;
                        unsigned int totallines = linesbuf.size();

                        if (pcEndLineNum > pcStartLineNum)
                          for (unsigned int n = pcStartLineNum; n <= pcEndLineNum; n++)
                            outs() << n << " " << linesbuf[n].str() << "\n";

                        StringRef firstline = linesbuf[pcStartLineNum];
                        StringRef lastline = linesbuf[pcEndLineNum];
                        size_t startpos = firstline.find(StringRef("EXEC"));
                        size_t endpos = lastline.rfind(';');
                        StringRef indent = firstline.substr(0, startpos);
                        std::string *newline = nullptr;
                        if (startpos != StringRef::npos)
                          {
                            if (generation_do_keep_commented_out_exec_sql)
                              {
                                std::string newline = linesbuf[pcEndLineNum].str();
                                newline.append("\n");
                                newline.append(rpltcode);
                                linesbuf[pcEndLineNum] = newline;
                              }
                            else
                              {
                                newline = new std::string(indent.str());
                                newline->append(rpltcode);
                                if (endpos != StringRef::npos)
                                  newline->append(lastline.substr(endpos+1, StringRef::npos));
                                linesbuf[pcStartLineNum] = std::move(StringRef(newline->c_str()));
                                if (pcEndLineNum > pcStartLineNum)
                                  for (unsigned int n = pcStartLineNum+1; n <= pcEndLineNum; n++)
                                    linesbuf[n] = std::move(StringRef(indent));
                              }
                          }

                        else
                          outs() << originalfile << ":" << line
                                 << ":1: warning: Couldn't find 'EXEC SQL " << execsql
                                 << ";' statement to replace with '" << rpltcode
                                 << "' in original '" << pcFilename
                                 << "' file! Already replaced ?\n";
                          
                        std::ofstream ofs(pcFilename, std::ios::out | std::ios::trunc);
                        if (ofs.is_open())
                          {
                            for (unsigned int n = 1; n < totallines; n++)
                              {
                                ofs.write(linesbuf[n-1].str().c_str(), linesbuf[n-1].str().length());
                                ofs.write("\n", 1);
                              }
                                                        
                            ofs.flush();
                            ofs.close();
                          }
                        
                        else
                          // TODO: add reported llvm error
                          outs() << originalfile << ":" << line
                                 << ":1: warning: Cannot overwrite file " << pcFilename << " !\n";
                        
                        delete newline;
                      }
                  }
              }

            delete Buffer;
            delete buffer;
          }
      }
      
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
