#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Preprocessor.h"

#include "Config.h"

#include <fstream>
#include <stack>
#include <string>
#include <vector>

namespace clang
{
    namespace type_erasure
    {
        class TableGenerator : public RecursiveASTVisitor<TableGenerator>
        {
        public:
            TableGenerator(const char* FileName,
                           ASTContext& Context,
                           Preprocessor& PP,
                           const Config& Configuration);

            ~TableGenerator();

            TableGenerator(const TableGenerator&) = delete;
            TableGenerator& operator=(const TableGenerator&) = delete;
            TableGenerator(TableGenerator&&) = delete;
            TableGenerator& operator=(TableGenerator&&) = delete;

            bool VisitNamespaceDecl(NamespaceDecl* Declaration);

            bool VisitCXXRecordDecl(CXXRecordDecl* Declaration);

            std::ofstream& getFileStream();

        private:
            std::ofstream TableFile;
            ASTContext& Context;
            Config Configuration;
            std::stack<std::string> OpenNamespaces;
        };
    }
}
