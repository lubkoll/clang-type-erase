#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Preprocessor.h"

#include "Config.h"

#include <fstream>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace clang
{
    namespace type_erasure
    {
        class InterfaceGenerator
                : public RecursiveASTVisitor<InterfaceGenerator>
        {
            struct AliasAndStaticMemberEntry
            {
                AliasAndStaticMemberEntry(const std::string& ClassName,
                                          std::string&& Entry);
                std::string ClassName;
                std::string Entry;
            };

            struct Interface
            {
                Interface(const std::string& ClassName,
                          std::string&& Content);
                std::string ClassName;
                std::string Content;
            };

        public:
            InterfaceGenerator(const char* FileName,
                               ASTContext& Context,
                               Preprocessor& PP,
                               const Config& Configuration = Config(),
                               const std::vector<std::string>& Includes = {});

            ~InterfaceGenerator();

            bool VisitNamespaceDecl(NamespaceDecl* Declaration);

            bool VisitCXXRecordDecl(CXXRecordDecl* Declaration);

            bool VisitVarDecl(VarDecl* Declaration);

            bool VisitTypedefDecl(TypedefDecl* Declaration);

            bool VisitTypeAliasDecl(TypeAliasDecl* Declaration);

            bool VisitFunctionDecl(FunctionDecl* Declaration);

            bool VisitFunctionTemplateDecl(FunctionTemplateDecl* Declaration);

            std::ofstream& getFileStream();

        private:
            bool VisitSimpleCXXRecordDecl(CXXRecordDecl* Declaration);
            bool VisitCustomCXXRecordDecl(CXXRecordDecl* Declaration);

            std::ofstream InterfaceFile;
            std::stringstream InterfaceFileStream;
            ASTContext& Context;
            Preprocessor& PP;
            Config Configuration;
            std::stack<std::string> OpenNamespaces;
            std::vector<Interface> Interfaces;
            std::vector<AliasAndStaticMemberEntry> AliasesAndStaticMembers;
            std::string CurrentClass;
        };
    }
}
