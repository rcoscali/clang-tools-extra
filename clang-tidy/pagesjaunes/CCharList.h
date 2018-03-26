//===--- CCharList.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARLIST_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARLIST_H

#include "../ClangTidy.h"
#include "llvm/Support/Regex.h"

using namespace llvm;
using namespace clang::ast_matchers;
using namespace clang;

using declmap_t = std::map<std::string, std::map<std::string, std::string>>;
using occmap_t = std::vector<std::map<std::string, std::string>>;
using declocc_t = std::map<std::string, std::vector<std::map<std::string, std::string>>>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      /// @brief Checks that argument name match parameter name rules.
      ///
      /// These options are supported:
      ///   * `Include-Comment-Regex`: The regular expression to use for parsing comment
      ///     default is 1 (process strcpy)
      ///   * `Handle-strcmp`: actually process or not strcpy
      ///     default is 1 (process strcmp)
      ///   * `Handle-strlen`: actually process or not strcpy
      ///     default is 1 (process strlen)
      ///
      class CCharList : public ClangTidyCheck 
      {
      public:
	CCharList(StringRef Name, ClangTidyContext *Context);

        // Called when Start of translation unit
        void onStartOfTranslationUnit();
        // Called when End of translation unit
        void onEndOfTranslationUnit();
	// Store check options
	void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
	// Register the AST matchers used for finding nodes
	void registerMatchers(ast_matchers::MatchFinder *Finder) override;
	// Callback calkled with each found node
	void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

      private:
        
        struct VarDeclOccurence
        {
          const DeclRefExpr *declref;
          const VarDecl *vardecl;
          SourceRange *codeRange;
        };

        std::vector<struct VarDeclOccurence *> m_varDeclOccCollector;

	class FindDeclOccurenceMatcher : public MatchFinder::MatchCallback
	{
	public:
	  /// Explicit constructor taking the parent instance as param
	  explicit FindDeclOccurenceMatcher(CCharList *parent)
	    : m_parent(parent)
	  {}

	  /// The run method adding all calls in the collection vector
	  virtual void
	  run(const MatchFinder::MatchResult &result)
	  {
	    struct VarDeclOccurence *record = new(struct VarDeclOccurence);
	    record->declref = result.Nodes.getNodeAs<DeclRefExpr>("declref");
	    record->vardecl = result.Nodes.getNodeAs<VarDecl>("vardecl");
            record->codeRange = new SourceRange(record->declref->getLocStart(), record->declref->getLocEnd());
	    m_parent->m_varDeclOccCollector.push_back(record);
	  }

	private:
	  // Parent CCHarList instance
	  CCharList *m_parent;
	};
        
      private:

        occmap_t searchOccurencesVarDecl(SourceManager &, std::string&);

        occmap_t searchOccurencesArrayVarDecl(SourceManager &, std::string&);

        occmap_t searchOccurencesPtrVarDecl(SourceManager &, std::string&);

        void emitDiagAndFix(DiagnosticsEngine &,
			    const SourceLocation&, 
			    std::string&);
	
	enum CCharListErrorKind
	  {
	    CCHAR_LIST_ERROR_NO_ERROR = 0,
	    CCHAR_LIST_ERROR_ARRAY_TYPE_NOT_FOUND,
	    CCHAR_LIST_ERROR_RECORD_DECL_NOT_FOUND,
	    CCHAR_LIST_ERROR_MEMBER_HAS_NO_DEF,
	    CCHAR_LIST_ERROR_MEMBER_NOT_FOUND,
	    CCHAR_LIST_ERROR_MEMBER2_NOT_FOUND,
	    CCHAR_LIST_ERROR_UNEXPECTED_AST_NODE_KIND
	  };
		
	void emitError(DiagnosticsEngine &,
		       const SourceLocation& err_loc,
		       enum CCharListErrorKind,
		       std::string *msg = nullptr);

	// Context instance
	ClangTidyContext *TidyContext;

        // Option for report file inclusion
        const std::string file_inclusion_regex;

        // Option for handling var decl
        const bool handle_var_decl;
        // Option for handling field decl
        const bool handle_field_decl;
        // Option for handling parm decl
        const bool handle_parm_decl;
        // Option for handling char 
        const bool handle_char_decl;
        // Option for handling char []
        const bool handle_char_array_decl;
        // Option for handling char *
        const bool handle_char_ptr_decl;
        // Results output csv file
        const std::string output_csv_file_pathname;

        // Stats for the files
        static int vardecl_num;
        static int arrayvardecl_num;
        static int ptrvardecl_num;
        static int fielddecl_num;
        static int arrayfielddecl_num;
        static int ptrfielddecl_num;
        static int parmdecl_num;
        static int arrayparmdecl_num;
        static int ptrparmdecl_num;

        // Maps for handling stats unicity
        static declmap_t vardecl_map;
        static declocc_t vardecl_occmap;
        static declmap_t arrayvardecl_map;
        static declocc_t arrayvardecl_occmap;
        static declmap_t ptrvardecl_map;
        static declocc_t ptrvardecl_occmap;
        static declmap_t fielddecl_map;
        static declocc_t fielddecl_occmap;
        static declmap_t arrayfielddecl_map;
        static declocc_t arrayfielddecl_occmap;
        static declmap_t ptrfielddecl_map;
        static declocc_t ptrfielddecl_occmap;
        static declmap_t parmdecl_map;
        static declocc_t parmdecl_occmap;
        static declmap_t arrayparmdecl_map;
        static declocc_t arrayparmdecl_occmap;
        static declmap_t ptrparmdecl_map;
        static declocc_t ptrparmdecl_occmap;
       
      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_CCHARLIST_H
