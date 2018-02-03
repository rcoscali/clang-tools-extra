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
#include <math.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <iomanip>

#include "ExecSQLCommon.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "llvm/Support/Regex.h"

#ifndef ACTIVATE_TRACES_DEFINED
// These are usefull only for unitary tests
//#define ACTIVATE_TRACES 
//#define ACTIVATE_RESULT_TRACES 
#endif // !ACTIVATE_TRACES_DEFINED

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
      /*
       * createParamsDef
       *
       * @brief Format a string for providing params definition
       *
       * This method allows to format a string containing one parameter
       * definition.
       *
       * @param[in] type	the type of the parameter
       * @param[in] elemtype	the element type in case of contant array
       * @param[in] size	the size of the constant array
       * @param[in] name	the name of the parameter
       *
       * @return the string formatted for providing params definition
       */
      std::string
      createParamsDef(const std::string& type,
                      const std::string& elemtype,
                      const std::string& size,
                      const std::string& name)
      {
        std::string ret;

#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "type = '" << type << "'\n";
        llvm::outs() << "elemtype = '" << elemtype << "'\n";
        llvm::outs() << "size = '" << size << "'\n";
        llvm::outs() << "name = '" << name << "'\n";
#endif

        if (! (elemtype.empty() || size.empty()))
          {
            ret.assign(elemtype);
            ret.append(" &");
            ret.append(name);
            ret.append("[");
            ret.append(size);
            ret.append("]");
          }
        else
          {
            ret.assign(type);
            ret.append(" &");
            ret.append(name);
          }
        
        ret.append(", ");
        
#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "createParamsDef ret = '" << ret << "'\n";
#endif
        return std::move(ret);
      }

      /*
       * createParamsDeclareSection
       *
       * @brief Format a string for providing the declare section
       *
       * This method allows to format a string containing one parameter
       * and one host variable in a declare section
       *
       * @param[in] type	the type of the parameter/host variable
       * @param[in] elemtype	the element type in case of contant array
       * @param[in] size	the size of the constant array
       * @param[in] name	the name of the host variable
       * @param[in] paramname	the name of the parameter
       *
       * @return the string formatted for providing params/host vars
       *         declare section
       */
      std::string
      createParamsDeclareSection(const std::string& type,
                                 const std::string& elemtype,
                                 const std::string& size,
                                 const std::string& name,
                                 const std::string& paramname)
      {
        std::string ret;

#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "type = '" << type << "'\n";
        llvm::outs() << "elemtype = '" << elemtype << "'\n";
        llvm::outs() << "size = '" << size << "'\n";
        llvm::outs() << "name = '" << name << "'\n";
#endif

        ret.assign("    ");
        if (! (elemtype.empty() || size.empty()))
          {
            ret.append(elemtype);
            ret.append("* ");
            ret.append(name);
            // ret.append("[");
            // ret.append(size);
            // ret.append("] = ");
            ret.append(" = ");
            ret.append(paramname);
          }
        else
          {
            ret.append(type);
            ret.append(" ");
            ret.append(name);
            ret.append(" = ");
            ret.append(paramname);
          }
        
        ret.append(";\n");
        
#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "createParamsDeclareSection ret = '" << ret << "'\n";
#endif
        return std::move(ret);
      }

      /*
       * createParamsDecl
       *
       * @brief Format a string for providing params declaration
       *
       * This method allows to format a string containing one parameter
       * declaration.
       *
       * @param[in] type	the type of the parameter
       * @param[in] elemtype	the element type in case of contant array
       * @param[in] size	the size of the constant array
       *
       * @return the string formatted for providing params declaration
       */
      std::string
      createParamsDecl(const std::string& type,
                       const std::string& elemtype,
                       const std::string& size)
      {
        std::string ret;

#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "type = '" << type << "'\n";
        llvm::outs() << "elemtype = '" << elemtype << "'\n";
        llvm::outs() << "size = '" << size << "'\n";
#endif
        if (! (elemtype.empty() || size.empty()))
          {
            ret.assign(elemtype);
            ret.append("[");
            ret.append(size);
            ret.append("]");
          }
        else
            ret.assign(type);

        ret.append("&, ");
        
#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "createParamsDecl ret = '" << ret << "'\n";
#endif
        return std::move(ret);
      }
      
      /*
       * createParamsCall
       *
       * @brief Format a string for providing function call arguments
       *
       * This method allows to format a string containing one parameter
       * for function call.
       *
       * @param[in] name	the name of the parameter
       *
       * @return the string formatted for providing params declaration
       */
      std::string
      createParamsCall(const std::string& name)
      {
        std::string ret;

#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "name = '" << name << "'\n";
#endif
        ret.append(name);
        ret.append(", ");
        
#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "createParamsCall ret = '" << ret << "'\n";
#endif
        return std::move(ret);
      }

      /*
       * createHostVarList
       *
       * @brief Format a string for host var list for the request
       *
       * This method allows to format a string containing one host var
       * for the request.
       *
       * @param[in] name	the name of the host variable
       *
       * @return the string formatted for providing host var list for the request
       */
      std::string
      createHostVarList(const std::string& name, bool isIndicator = false)
      {
        std::string ret;

#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "name = '" << name << "'\n";
#endif
        if (!name.empty())
          {
            ret.append(":");
            ret.append(name);
          }
        if (isIndicator)
          ret.append(", ");
        
#ifdef ACTIVATE_RESULT_TRACES
        llvm::outs() << "createHostVarList ret = '" << ret << "'\n";
#endif
        return std::move(ret);
      }

      /**
       * findSymbolInFunction
       *
       * @brief Find a symbol, its definition and line number in the current function
       *
       * This method search the AST from the current function for a given symbol. When
       * found it return a record struct having pointer to AST nodes of interrest.
       *
       * @param[in] tool	The clang::tooling::Tool instance
       * @param[in] varName	The name of the symbol to find
       * @param[in] func	The AST node of the function to search into
       *
       * @return The pointer to the varDecl node instance for the searched symbol
       */
      const VarDecl *
      findSymbolInFunction(MatchFinder::MatchCallback& vdMatcher,
                           ClangTool *tool,
                           std::string& varName,
                           const FunctionDecl *func,
                           std::vector<struct clang::tidy::pagesjaunes::VarDeclMatchRecord *>& collector)
      {
      	const VarDecl *ret = nullptr;
        
	DeclarationMatcher m_matcher
	  // find a sprintf call expr
	  = varDecl(hasName(varName.c_str()),
		    // and this decl is in the current function (call bound to varDecl))
		    hasAncestor(functionDecl(hasName(func->getNameAsString().insert(0, "::"))))).bind("varDecl");
	
	// The matcher implementing visitor pattern
	MatchFinder finder;

	// Add our matcher to our processing class
	finder.addMatcher(m_matcher, &vdMatcher);

	// Clear collector vector (the result filled with found patterns)
	collector.clear();

	// Run the visitor pattern for collecting matches
	tool->run(newFrontendActionFactory(&finder).get());

	if (!collector.empty()) ret = collector[0]->varDecl;

        return ret;
      }

      /**
       * findDeclInFunction
       *
       * @brief Find a declaration of a symbol in the context of a function by using the function
       * DeclContext iterators until the symbol is found.
       * This method do not update AST. It only browse known declarations in the context of a function.
       * On successfull completion the map will contain:
       * - a key "symName" with the symbol name provided
       * - a key "typeName" with the symbol type name found
       * - a key "elementType" with the type of the element if the type
       *   found is a constant array type
       * - a key "elemntSize" with the number of element if the type
       *   found is a constant array type
       *
       * @param[in] func 	the function of browse for finding the symbol
       * @param[in] symName 	the name of the symbol to find
       *
       * @return the map containing informations on the found symbol. The map is empty is no symbol
       *         with the same name were found.
       */
      string2_map
      findDeclInFunction(const FunctionDecl *func, const std::string& symName)
      {
        string2_map ret;
        
        for (auto declit = func->decls_begin(); declit != func->decls_end(); ++declit)
          {
            const Decl *aDecl = *declit;
            std::string typeName;
            std::string varName;
            QualType qtype;
            if (isa<VarDecl>(*aDecl))
              {
                const VarDecl *varDecl = dyn_cast<VarDecl>(aDecl);
                qtype = varDecl->getType();
                SplitQualType qt_split = qtype.split();
                typeName = QualType::getAsString(qt_split);
              }
            if (isa<NamedDecl>(*aDecl))
              {
                const NamedDecl *namedDecl = dyn_cast<NamedDecl>(aDecl);
                varName = namedDecl->getNameAsString();
              }
            if (varName.compare(symName) == 0)
              {
                ret["symName"] = varName;
                ret["typeName"] = typeName;
                if (qtype->isConstantArrayType())
                  {
                    const ConstantArrayType *catype = aDecl->getASTContext().getAsConstantArrayType(qtype);
                    ret["elementType"] = catype->getElementType().getAsString();
                    ret["elementSize"] = catype->getSize().toString(10, false);
                  }

#ifdef ACTIVATE_RESULT_TRACES
                outs() << "findDeclInFunction returned map:\n";
                for (auto mit = ret.begin(); mit != ret.end(); ++mit)
                  outs() << "key " << mit->first << " = '" << mit->second << "'\n"; 
#endif // !ACTIVATE_RESULT_TRACES
                
                return std::move(ret);
              }
          }
        
#ifdef ACTIVATE_RESULT_TRACES
        outs() << "findDeclInFunction returned empty map.\n";
#endif // !ACTIVATE_RESULT_TRACES
        
        return std::move(ret);
      }

      /**
       * findCXXRecordMemberInTranslationUnit
       *
       * @brief This function will browse a translation unit and search for a specific 
       *        named CXXRecord and a named member of it
       * 
       * @param[in]	the translation unit declaration context to browse for the record
       * @param[in] 	the struct/union/class name to find in the context of the translation
       *                unit
       * @param[in] 	the member name to find in the class/struct
       *
       * @return a map containing informations about the found member. if no record/member were 
       *         found, the returned map is empty
       */
      string2_map
      findCXXRecordMemberInTranslationUnit(const TranslationUnitDecl *transUnit,
                                           const std::string& cxxRecordName,
                                           const std::string& memberName)
      {
#ifdef ACTIVATE_TRACES
        outs() << "findCXXRecordMemberInTranslationUnit searching  '" << cxxRecordName << "' '" << memberName << "'\n";
#endif // !ACTIVATE_TRACES
        string2_map ret;
        // Let's first search is root context
        DeclContext *declCtxt = TranslationUnitDecl::castToDeclContext(transUnit);
        SmallVector<DeclContext *, 10> declCtxts;
        // Then in all semantically connected contexts (ex. namespaces)
        const_cast<TranslationUnitDecl *>(transUnit)->collectAllContexts(declCtxts);
        // Add root context in vector for having one loop only
        // There is better chances to find in connected context instead of root one, then add root at end
        declCtxts.push_back(declCtxt);
        
        for (auto ctxtsit = declCtxts.begin(); ctxtsit != declCtxts.end(); ++ctxtsit)
          {
            DeclContext *aDeclCtxt = *ctxtsit;
            for (auto declit = aDeclCtxt->decls_begin(); declit != aDeclCtxt->decls_end(); ++declit)
              {
                std::string recordName;
                const Decl *aDecl = *declit;
                if (isa<NamedDecl>(*aDecl))
                  {
                    const NamedDecl *namedDecl = dyn_cast<NamedDecl>(aDecl);
                    recordName = namedDecl->getNameAsString();
                  }
                if (isa<CXXRecordDecl>(*aDecl) &&
                    !recordName.empty() &&
                    (cxxRecordName.find(recordName) != std::string::npos ||
                     std::string("struct ").append(recordName).compare(cxxRecordName) == 0))
                  {
                    const RecordDecl *recordDecl = dyn_cast<RecordDecl>(aDecl);
                    // iterate on record member fields
                    for (auto fieldsit = recordDecl->field_begin(); fieldsit != recordDecl->field_end(); ++fieldsit)
                      {
                        std::string fieldName;
                        std::string fieldTypeName;
                        const FieldDecl *fieldDecl = *fieldsit;
                        if (isa<NamedDecl>(*fieldDecl))
                          {
                            const NamedDecl *namedDecl = dyn_cast<NamedDecl>(fieldDecl);
                            fieldName = namedDecl->getNameAsString();
                          }
                        if (fieldName.compare(memberName) == 0 && isa<ValueDecl>(*fieldDecl))
                          {
                            const ValueDecl *valueDecl = dyn_cast<ValueDecl>(fieldDecl);
                            QualType qtype = valueDecl->getType();
                            SplitQualType qt_split = qtype.split();
                            fieldTypeName = QualType::getAsString(qt_split);
                            ret["recordName"] = recordName;
                            ret["fieldName"] = fieldName;
                            ret["fieldTypeName"] = fieldTypeName;
                            if (qtype->isConstantArrayType())
                              {
                                const ConstantArrayType *catype = fieldDecl->getASTContext().getAsConstantArrayType(qtype);
                                ret["elementType"] = catype->getElementType().getAsString();
                                ret["elementSize"] = catype->getSize().toString(10, false);
                              }

#ifdef ACTIVATE_RESULT_TRACES
                            // Debug output
                            outs() << "findCXXRecordMemberInTranslationUnit returned map:\n";
                            for (auto mit = ret.begin(); mit != ret.end(); ++mit)
                              outs() << "key " << mit->first << " = '" << mit->second << "'\n"; 
#endif // !ACTIVATE_RESULT_TRACES
                            return std::move(ret);
                          }
                      }
                  }
              }
          }
        
#ifdef ACTIVATE_TRACES
        // Debug empty output        
        outs() << "findCXXRecordMemberInTranslationUnit returned empty map\n";
#endif // !ACTIVATE_TRACES
        
        return std::move(ret);
      }

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
                // First trim out all unwanted spaces in case there is some
                var["full"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_FULLMATCH].trim(" \n\t");
                var["hostvar"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTVAR].trim(" \n\t");
                var["hostmember"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTMEMBER].trim(" \n\t");
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
                  var["hostrecord"] = idmatches[PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE_IDENTIFIER].trim(" \n\t");
              }
            else
              {
                size_t derefipos;
                var["fulli"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_FULLMATCH].trim(" \n\t");
                var["hostvari"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTVAR].trim(" \n\t");
                var["hostmemberi"] = matches[PAGESJAUNES_REGEX_HOSTVAR_DECODE_RE_HOSTMEMBER].trim(" \n\t");
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
                  var["hostrecordi"] = idmatches[PAGESJAUNES_REGEX_TRIM_IDENTIFIER_RE_IDENTIFIER].trim(" \n\t");
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
       * createBackupFile
       *
       * @brief Create a backup file for file pathname provided
       *
       * @param[in] pathame	the pathname of the file we want to create a backup for
       */
      void
      createBackupFile(const std::string& pathname)
      {
        struct stat buffer;   

        // Open the file to backup
        std::ifstream  src(pathname.c_str(), std::ios::binary);
        if (src.is_open())
          {
            std::string backupPathname(pathname.c_str());
            backupPathname.append(".bak");
            
            // First find if this bak is free (do not already exist)
            bool do_exist = true;
            int baknum = 0;
            do
              if ((do_exist = (stat (backupPathname.c_str(), &buffer)) == 0))
                {
                  backupPathname.assign(pathname);
                  int nbaknum = (int)(floor(log10(baknum)));
#ifdef ACTIVATE_TRACES
                  llvm::outs() << "baknum = " << baknum << "\n";
                  llvm::outs() << "nbaknum = " << nbaknum << "\n";
                  llvm::outs() << "floor(log10(baknum)) = " << nbaknum << "\n";
#endif // !ACTIVATE_TRACES
                  if (baknum == 0)
                    backupPathname.append("-0.bak");

                  else
                    {
                      int baknum_temp = baknum;
                      char digit = '0';
                      backupPathname.append("-");
                      for (int n  = nbaknum; n >= 0; n--)
                        {
#ifdef ACTIVATE_TRACES
                          llvm::outs() << "n = " << n << "\n";
#endif // !ACTIVATE_TRACES
                          digit = (char)(floor(baknum_temp / exp10(n))) + '0';
                          baknum_temp -= (floor(baknum_temp / exp10(n)))*exp10(n);
#ifdef ACTIVATE_TRACES
                          llvm::outs() << "digit = floor(baknum / exp10(n)) = " << digit << "\n";
                          llvm::outs() << "baknum_temp = " << baknum_temp << "\n";
#endif // !ACTIVATE_TRACES
                          backupPathname.append(1, digit);
                        }
                      backupPathname.append(".bak");
                    }
#ifdef ACTIVATE_TRACES
                  llvm::outs() << "trying new bak path = " << backupPathname << "\n";
#endif // !ACTIVATE_TRACES
                  baknum++;
                }
            while (do_exist);
            
#ifdef ACTIVATE_TRACES
            llvm::outs() << "found free bak path = " << backupPathname << "\n";
#endif // !ACTIVATE_TRACES
            // Ok we got a free backup file name, then let's go
            std::ofstream  dst(backupPathname.c_str(), std::ios::binary);
            if (dst.is_open())
              {
                dst << src.rdbuf();
                dst.close();
              }
            src.close();
          }
      }

      /*
       * bufferSplit
       *
       * @brief split a buffer of char in a vector of lines
       *
       * @params[in] buffer	the buffer of char to split in lines
       * @params[in] start_at_0 a boolean indicating the returned vector of lines
       *                        starts at index 0 or index 1. If it starts at index
       *                        1, the index 0 line is empty. Default is false.
       * @return the vector with lines
       */
      std::vector<std::string>
      bufferSplit(char *buffer, std::vector<std::string>::size_type& nlines, std::vector<std::string>::size_type reserve, bool start_at_0)
      {
        nlines = 0;
        std::vector<std::string> ret;
        ret.clear();
        if (reserve > 0)
          ret.reserve(reserve);
        std::string buf(buffer);
        if (!start_at_0)
          {
            ret.push_back("");
            nlines++;
          }

        while(!buf.empty())
          {
            size_t crpos = buf.find_first_of("\n\0");
            std::string newline = buf.substr(0, crpos);
#ifdef ACTIVATE_TRACES
            llvm::outs() << "Adding line #" << nlines << ": " << newline << "\n";
#endif // !ACTIVATE_TRACES
            ret.push_back(newline);
            buf.erase(0, crpos + (crpos == std::string::npos ? 0 : 1));
            nlines++;
          }
#ifdef ACTIVATE_TRACES
        llvm::outs() << "Lines read from files: " << nlines << " lines\n";
#endif // !ACTIVATE_TRACES
        return ret;
      }

      /**
       * readTextFile
       *
       * @brief read a text file and correctly append 0 at end of read string
       *
       * @param[in]    filename	the file pathname of the file to read
       * @param[inout] filesize	the size of the file read
       *
       * @return a new allocated buffer (to deallocate by caller) containing
       *         file contents 
       */
      const char *
      readTextFile(const char *filename, std::size_t& filesize)
      {
        char *ret = nullptr;
        std::size_t size = 0;

        std::ifstream ifs (filename, std::ifstream::in|std::ios::binary);
        if (ifs.is_open())
          {
            std::filebuf* pbuf = ifs.rdbuf();
            size = pbuf->pubseekoff (0,ifs.end,ifs.in);

            if (!size)
              {
#ifdef ACTIVATE_TRACES
                llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): PC file is empty!\n";
                llvm::errs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): PC file is empty!\n";
#endif // !ACTIVATE_TRACES
                // TODO: add reported llvm error
                llvm::errs() << filename <<
                  ":1: warning: Your original file in which to report modifications is empty !\n";
              }
            else
              {
                pbuf->pubseekpos(0, ifs.in);
                // allocate memory to contain file data
                ret = new char[size + 1];
                assert(ret);
                        
                // get file data
                pbuf->sgetn(ret, size);
                ret[size] = '\0';
                ifs.close();
              }
          }
        else
          {
#ifdef ACTIVATE_TRACES
            llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Cannot open file " << filename << "!\n";
            llvm::errs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Cannot open file " << filename << "!\n";
#endif // !ACTIVATE_TRACES
            // TODO: add reported llvm error
            llvm::errs() << filename <<
              ":1: warning: Cannot open file!\n";
          }

        filesize = size;

        return const_cast<const char *>(ret);
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
#ifdef ACTIVATE_TRACES
        llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): ENTRY!\n";
        llvm::outs() << "    generation_report_modification_in_dir = '" << generation_report_modification_in_dir << "'\n";
        llvm::outs() << "    generation_do_keep_commented_out_exec_sql = " << (generation_do_keep_commented_out_exec_sql? "TRUE":"FALSE") << "\n";
#endif // !ACTIVATE_TRACES

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
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'execsql' value processing...\n";
                    llvm::outs() << "    execsql = '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                    execsql = val;
                  }

                else if (key.compare("fullcomment") == 0)
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'fullcomment' value processing...\n";
                    llvm::outs() << "    fullcomment = '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                    fullcomment = val;
                  }

                else if (key.compare("funcname") == 0)
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'funcname' value processing...\n";
                    llvm::outs() << "    funcname = '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                    funcname = val;
                  }

                else if (key.compare("originalfile") == 0)
                  {
                    Regex fileline(PAGESJAUNES_REGEX_EXEC_SQL_ALL_FILELINE);
                    SmallVector<StringRef, 8> fileMatches;

#ifdef ACTIVATE_TRACES
                        llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'originalfile' value processing...\n";
                        llvm::outs() << "    Matching fileline on 'originalfile' value: '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                    if (fileline.match(val, &fileMatches))
                      {
                        originalfile = fileMatches[1];
                        std::istringstream isstr;
                        std::stringbuf *pbuf = isstr.rdbuf();
                        pbuf->str(fileMatches[2]);
                        isstr >> line;
#ifdef ACTIVATE_TRACES
                        llvm::outs() << "    originalfile = '" << originalfile << "'\n";
                        llvm::outs() << "    line         = " << line << "\n";
#endif // !ACTIVATE_TRACES
                      }

                    else
                      {
#ifdef ACTIVATE_TRACES
                        llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): no match: 'originalfile' value processing...\n";
                        llvm::outs() << "    originalfile = '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                        originalfile = val;
                      }

                  }
                else if (key.compare("reqname") == 0)
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'reqname' value processing...\n";
                    llvm::outs() << "    reqname = '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                    reqname = val;
                  }

                else if (key.compare("rpltcode") == 0)
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'rpltcode' value processing...\n";
                    llvm::outs() << "    rpltcode = '" << val << "'\n";
#endif // !ACTIVATE_TRACES
                    rpltcode = val;
                  }

                else if (key.compare("pclinenumstart") == 0)
                  {
                    std::istringstream isstr;
                    std::stringbuf *pbuf = isstr.rdbuf();
                    pbuf->str(val);
                    isstr >> pcLineNumStart;
                    has_pcLineNum = true;
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'pclinenumstart' value processing...\n";
                    llvm::outs() << "    pcLineNumStart = '" << pcLineNumStart << "'\n";
                    llvm::outs() << "    has_pcLineNum = TRUE'\n";
#endif // !ACTIVATE_TRACES
                  }
                else if (key.compare("pclinenumend") == 0)
                  {
                    std::istringstream isstr;
                    std::stringbuf *pbuf = isstr.rdbuf();
                    pbuf->str(val);
                    isstr >> pcLineNumEnd;
                    has_pcLineNum = true;
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'pclinenumend' value processing...\n";
                    llvm::outs() << "    pcLineNumEnd = '" << pcLineNumEnd << "'\n";
                    llvm::outs() << "    has_pcLineNum = TRUE'\n";
#endif // !ACTIVATE_TRACES
                  }
                else if (key.compare("pcfilename") == 0)
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): 'pcfilename' value processing...\n";
                    llvm::outs() << "    pcFilename = '" << val << "'\n";
                    llvm::outs() << "    has_pcFilename = TRUE'\n";
#endif // !ACTIVATE_TRACES
                    pcFilename = val;
                    has_pcFilename = true;
                  }                
              }

            if (has_pcLineNum && has_pcFilename)
              has_pcFileLocation = true;

#ifdef ACTIVATE_TRACES
            llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): has_pcFileLocation...\n";
            llvm::outs() << "    has_pcFileLocation = has_pcLineNum && has_pcFilename = ";
            llvm::outs() << (has_pcLineNum ? "TRUE":"FALSE") << " && " << (has_pcFilename ? "TRUE":"FALSE");
            llvm::outs() << " = " << (has_pcFileLocation ? "TRUE":"FALSE") << "\n";
#endif // !ACTIVATE_TRACES

            char* buffer = nullptr;
            StringRef *Buffer = nullptr;
            std::string NewBuffer;

            // If #line are not present, need to find the line in file with regex
            if (!has_pcFileLocation)
              {
#ifdef ACTIVATE_TRACES
                llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Entering processing without pcFileLocation (then try to find it)...\n";
#endif // !ACTIVATE_TRACES
                // Without #line, we need to compute the filename
                pcFilename = generation_report_modification_in_dir;
                pcFilename.append("/");
                std::string::size_type dot_c_pos = originalfile.find('.');
                pcFilename.append(originalfile.substr(0, dot_c_pos +1));
                pcFilename.append("pc");

#ifdef ACTIVATE_TRACES
                llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Computed PC file location...\n";
                llvm::outs() << "    pcFilename = '" << pcFilename << "'\n";
#endif // !ACTIVATE_TRACES
              }

            // First let's create a backup if none exists
            clang::tidy::pagesjaunes::createBackupFile(pcFilename);

            // If #line are not present, need to find the line in file with regex
            if (!has_pcFileLocation)
              {
                std::size_t size;
                const char *buffer = readTextFile(pcFilename.c_str(), size);
                if (buffer)
                  {
                    Buffer = new StringRef(buffer);
                    assert(Buffer);
                    std::string execSqlReqReStr(PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_STARTSTR);
                    
                    size_t pos = 0;
                    while ((pos = execsql.find(" ")) != std::string::npos )
                      execsql.replace(pos, 1, PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_SPACE_RPLTSTR);
                    pos = -1;
                    while ((pos = execsql.find(",", pos+1)) != std::string::npos )
                      execsql.replace(pos, 1, PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_COMMA_RPLTSTR);
                    
                    execSqlReqReStr.append(execsql);
                    execSqlReqReStr.append(PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_ENDSTR);
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Computed execSqlReqRe string...\n";
                    llvm::outs() << "    execSqlReqRe string = '" << execSqlReqReStr << "'\n";
#endif // !ACTIVATE_TRACES
                    Regex execSqlReqRe(execSqlReqReStr.c_str(), Regex::NoFlags);
                    SmallVector<StringRef, 8> execSqlReqMatches;
                    
                    if (execSqlReqRe.match(*Buffer, &execSqlReqMatches))
                      {
                        if (generation_do_keep_commented_out_exec_sql)
                          {
                            std::string comment = execSqlReqMatches[PAGESJAUNES_REGEX_EXEC_SQL_REQ_RE_COMMENT_GROUP];
                            comment.append("\n");
                            comment.append(rpltcode);
                            StringRef RpltCode(comment);
                            NewBuffer = execSqlReqRe.sub(RpltCode, *Buffer);                                
                          }
                        else
                          {
                            StringRef RpltCode(rpltcode);
                            NewBuffer = execSqlReqRe.sub(RpltCode, *Buffer);
                          }
                      }
                    else
                      {
#ifdef ACTIVATE_TRACES
                        llvm::errs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): EXEC SQL not found!\n";
#endif // !ACTIVATE_TRACES
                        llvm::errs() << originalfile << ":" << line
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
                    
#ifdef ACTIVATE_TRACES
                    else
                      {
                        llvm::errs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Open file with TRUNC failed!\n";
                        // TODO: add reported llvm error
                        llvm::errs() << originalfile << ":" << line
                                     << ":1: warning: Cannot overwrite file " << pcFilename << " !\n";
                      }
#endif // !ACTIVATE_TRACES
                  }

                else
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Cannot open PC file '" << pcFilename << "'!\n";
                    llvm::errs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Cannot open PC file '" << pcFilename << "'!\n";
#endif // !ACTIVATE_TRACES
                    // TODO: add reported llvm error
                    llvm::errs() << originalfile << ":" << line
                                 << ":1: warning: Cannot open original file in which to report modifications: " << pcFilename
                                 << "\n";
                  }
              }
            //
            // Line # are generated by preprocessor, let's use it
            //
            else /* ! (!has_pcFileLocation) so congruent to has_pcFileLocation :) */
              {
#ifdef ACTIVATE_TRACES
                llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Entering processing with pcFileLocation (then use it)...\n";
                llvm::outs() << "    pcFilename = '" << pcFilename << "'\n";
#endif // !ACTIVATE_TRACES
                
                std::size_t size;
                const char *buffer = readTextFile(pcFilename.c_str(), size);
                if (buffer)
                  {
                    // Split buffer, read from file, in a vector of lines, starting at line #1
                    std::vector<std::string>::size_type totallines;
                    std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(buffer), totallines, 0, false);
                    
                    // We start counting at 0, file lines are generally counted from 1
                    //pcLineNumStart--;
                    //pcLineNumEnd--;
                    
                    // Let get our counters in the vector reference
                    unsigned int pcStartLineNum = pcLineNumStart;
                    unsigned int pcEndLineNum = pcLineNumEnd;
                    
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Dump of modified lines:\n";
                    if (pcEndLineNum > pcStartLineNum)
                      for (unsigned int n = pcStartLineNum; n <= pcEndLineNum; n++)
                        llvm::outs() << "    *--- " << n << " " << linesbuf[n] << "\n";
#endif // !ACTIVATE_TRACES
                    
                    std::string firstline = linesbuf[pcStartLineNum];
                    std::string lastline = linesbuf[pcEndLineNum];
                    size_t startpos = firstline.find("EXEC");
                    size_t endpos = lastline.rfind(';');
                    std::string indent = firstline.substr(0, startpos);
                    
                    std::string *newline = nullptr;
                    if (startpos != std::string::npos)
                      {
                        // We keep the number of lines invariant for dumping easily the vector
                        // Then we add new lines by having some lines in vector containing some cariage return
                        // Or for removing lines, we set empty lines (with indentation only)
                        if (generation_do_keep_commented_out_exec_sql)
                          {
                            linesbuf.data()[pcStartLineNum] = firstline.insert(firstline.find("EXEC"), "/* ");
                            linesbuf.data()[pcEndLineNum] = lastline.append(" */");
                            newline = new std::string(lastline);
                            assert(newline);
                            newline->append("\n");
                            newline->append(indent);
                            newline->append(rpltcode);
                            linesbuf.data()[pcEndLineNum] = std::string(*newline);
                          }
                        else
                          {
                            newline = new std::string(indent);
                            assert(newline);
                            newline->append(rpltcode);
                            if (endpos != std::string::npos)
                              newline->append(lastline.substr(endpos+1, std::string::npos));
                            linesbuf.data()[pcStartLineNum] = std::string(*newline);
                            if (pcEndLineNum > pcStartLineNum)
                              for (unsigned int n = pcStartLineNum+1; n <= pcEndLineNum; n++)
                                linesbuf.data()[n] = std::string(indent);
                          }
                      }
                    
#ifdef ACTIVATE_TRACES
                    else
                      llvm::errs() << originalfile << ":" << line
                                   << ":1: warning: Couldn't find 'EXEC SQL " << execsql
                                   << ";' statement to replace with '" << rpltcode
                                   << "' in original '" << pcFilename
                                   << "' file! Already replaced ?\n";
#endif // !ACTIVATE_TRACES
                    
                    delete newline;
                    newline = nullptr;
                    
                    std::ofstream ofs(pcFilename, std::ios::out | std::ios::trunc | std::ios::binary);
                    if (ofs.is_open())
                      {
                        linesbuf.shrink_to_fit();
                        unsigned int nwlines = 0;
                        for (auto lbit = linesbuf.begin(); lbit != linesbuf.end() && nwlines < totallines; ++lbit, ++nwlines)
                          {
                            if (nwlines > 0 && nwlines < totallines)
                              {
                                std::string line = *lbit;
#ifdef ACTIVATE_RESULT_TRACES
                                std::cout <<
                                  std::setfill('0') << std::setw(5) <<
                                  nwlines <<
                                  std::setfill(' ') << std::setw(0) <<
                                  " " << line << "\n";
#endif // !ACTIVATE_RESULT_TRACES
                                ofs.write(line.c_str(), line.length());
                                ofs.write("\n", 1);
                              }
                          }
#ifdef ACTIVATE_RESULT_TRACES
                        std::cout << "Total lines written/total lines: " << nwlines << "/" << totallines << "\n";
#endif // !ACTIVATE_RESULT_TRACES
                        
                        ofs.flush();
                        ofs.close();
                      }
                    
                    else
                      // TODO: add reported llvm error
                      llvm::outs() << originalfile << ":" << line
                                   << ":1: warning: Cannot overwrite file " << pcFilename << " !\n";                        
                  }
                else
                  {
#ifdef ACTIVATE_TRACES
                    llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Cannot open PC file '" << pcFilename << "'!\n";
                    llvm::errs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): Cannot open PC file '" << pcFilename << "'!\n";
#endif // !ACTIVATE_TRACES
                    // TODO: add reported llvm error
                    llvm::errs() << originalfile << ":" << line
                                 << ":1: warning: Cannot open original file in which to report modifications: " << pcFilename
                                 << "\n";
                  }
              }

            if (Buffer)
              delete Buffer;
            if (buffer)
              delete buffer;
          }

#ifdef ACTIVATE_TRACES
        llvm::outs() << "clang::tidy::pagesjaunes::onEndOfTranslationUnit(): EXIT!\n";
#endif // !ACTIVATE_TRACES        
      }
      
    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang
