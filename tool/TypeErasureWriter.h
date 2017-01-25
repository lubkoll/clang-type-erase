#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"
#include "../utils/Config.h"

#include "TableWriter.h"
#include "InterfaceWriter.h"

#include <boost/filesystem.hpp>

#include <vector>

namespace clang
{
    class Preprocessor;

    namespace type_erasure
    {
        boost::filesystem::path getTableFile(const Config& Configuration);

        class TypeErasureGenerator : public RecursiveASTVisitor<TypeErasureGenerator>
        {
        public:
            TypeErasureGenerator(ASTContext& Context,
                                 Preprocessor& PP,
                                 const Config& Configuration);

            TypeErasureGenerator(const TypeErasureGenerator&) = default;
            TypeErasureGenerator& operator=(const TypeErasureGenerator&) = delete;
            TypeErasureGenerator(TypeErasureGenerator&&) = default;
            TypeErasureGenerator& operator=(TypeErasureGenerator&&) = delete;

            bool VisitNamespaceDecl(NamespaceDecl* Declaration);

            bool VisitCXXRecordDecl(CXXRecordDecl* Declaration);

            bool VisitVarDecl(VarDecl* Declaration);

            bool VisitTypedefDecl(TypedefDecl* Declaration);

            bool VisitTypeAliasDecl(TypeAliasDecl* Declaration);

            bool VisitFunctionDecl(FunctionDecl* Declaration);

            bool VisitFunctionTemplateDecl(FunctionTemplateDecl* Declaration);

        private:
            TableGenerator TableGeneration;
            InterfaceGenerator InterfaceGeneration;
        };


        class TypeErasureActionFactory : public tooling::FrontendActionFactory
        {
        public:
            explicit TypeErasureActionFactory(const type_erasure::Config& Configuration);

            FrontendAction* create() override;

        private:
            type_erasure::Config Configuration;
        };
    }
}
