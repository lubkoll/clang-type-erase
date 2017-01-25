#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "../utils/Config.h"

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
                           const Config& Configuration);

            ~TableGenerator();

            TableGenerator(const TableGenerator&) = default;
            TableGenerator& operator=(const TableGenerator&) = delete;
            TableGenerator(TableGenerator&&) = default;
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
