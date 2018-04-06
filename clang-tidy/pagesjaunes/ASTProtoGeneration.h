//===--- ASTProtoGeneration.h - clang-tidy --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_ASTPROTOGENERATION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_ASTPROTOGENERATION_H

#include "../ClangTidy.h"
#include "llvm/Support/Regex.h"

using namespace llvm;
using namespace clang::ast_matchers;
using namespace clang;

using class_vector_t = std::vector<std::map<std::string, std::string>>;

namespace clang 
{
  namespace tidy 
  {
    namespace pagesjaunes
    {

      /// @brief Generation of proto files for LLVM AST classes
      ///
      /// This class will browse LLVM AST classes for getting all members of interest. For each
      /// member, a corresponding proto member will be produced
      ///
      class ASTProtoGeneration : public ClangTidyCheck 
      {
      public:
	ASTProtoGeneration(StringRef Name, ClangTidyContext *Context);

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
	  explicit FindDeclOccurenceMatcher(ASTProtoGeneration *parent)
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
	  ASTProtoGeneration *m_parent;
	};
        
      private:

        occmap_t searchOccurencesVarDecl(SourceManager &, std::string&);

        occmap_t searchOccurencesArrayVarDecl(SourceManager &, std::string&);

        occmap_t searchOccurencesPtrVarDecl(SourceManager &, std::string&);

        void emitDiagAndFix(DiagnosticsEngine &,
			    const SourceLocation&, 
			    std::string&);
	
	enum ASTProtoGenerationErrorKind
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
		       enum ASTProtoGenerationErrorKind,
		       std::string *msg = nullptr);

	// Context instance
	ClangTidyContext *TidyContext;

        // Option for class inclusion
        const class_vector_t ast_class_list;

        // Option for the target proto version (1, 2 or 3)
        const unsigned int target_proto_version;
        // Results output proto file
        const std::string output_proto_file_pathname;

      };

    } // namespace pagesjaunes
  } // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PAGESJAUNES_ASTPROTOGENERATION_H
