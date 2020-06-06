#include "TypeErasureWriter.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Preprocessor.h"

#include <numeric>

namespace clang
{
    namespace type_erasure
    {
        namespace
        {
            boost::filesystem::path getDetailFile(const Config& Configuration, const std::string& Detail)
            {
                const auto FileName = boost::filesystem::path(Configuration.SourceFile).filename();
                std::string Name = FileName.c_str();
                Name = Name.substr(0, Name.size() - std::string(FileName.extension().c_str()).size());
                Name += "_" + Detail + ".h";
                return boost::filesystem::path(Configuration.DetailDir) /=
                       boost::filesystem::path(Name);
            }

            boost::filesystem::path getInterfaceFile(const Config& Configuration)
            {
                return boost::filesystem::path(Configuration.TargetDir) /=
                       boost::filesystem::path(Configuration.SourceFile).filename();
            }

            boost::filesystem::path getRelativePath(const boost::filesystem::path& Path,
                                                    const boost::filesystem::path& IncludePath)
            {
                const auto PathEnd = Path.end();
                const auto PathBegin = std::mismatch(Path.begin(), PathEnd,
                                                     IncludePath.begin(), IncludePath.end()).first;
                return std::accumulate(PathBegin, PathEnd,
                                       boost::filesystem::path(),
                                       [](boost::filesystem::path Path, const boost::filesystem::path& Dir )
                {
                   return Path /= Dir;
                });
            }

            std::vector<std::string> getInterfaceIncludes(const Config& Configuration)
            {
                if(!Configuration.CustomFunctionTable)
                    return {};
                const auto Open = std::string("#include <");
                const auto Close = std::string(">");
                return { Open + getRelativePath(getTableFile(Configuration), Configuration.IncludeDir).c_str() + Close };
            }
        }

        boost::filesystem::path getTableFile(const Config& Configuration)
        {
            return getDetailFile(Configuration, "table");
        }

        TypeErasureGenerator::TypeErasureGenerator(ASTContext& Context,
                                                   Preprocessor& PP,
                                                   const Config& Configuration)
            : InterfaceGeneration(getInterfaceFile(Configuration).c_str(),
                                  Context,
                                  PP,
                                  Configuration,
                                  getInterfaceIncludes(Configuration))
        {
            if(Configuration.CustomFunctionTable)
                TableGeneration = std::make_unique<TableGenerator>(getTableFile(Configuration).c_str(),
                                                                   Context,
                                                                   PP,
                                                                   Configuration);
        }

        bool TypeErasureGenerator::VisitNamespaceDecl(NamespaceDecl* Declaration)
        {
            if(TableGeneration)
                TableGeneration->VisitNamespaceDecl(Declaration);
            InterfaceGeneration.VisitNamespaceDecl(Declaration);
            return true;
        }

        bool TypeErasureGenerator::VisitCXXRecordDecl(CXXRecordDecl* Declaration)
        {
            if(TableGeneration)
                TableGeneration->VisitCXXRecordDecl(Declaration);
            InterfaceGeneration.VisitCXXRecordDecl(Declaration);
            return true;
        }

        bool TypeErasureGenerator::VisitVarDecl(VarDecl* Declaration)
        {
            return InterfaceGeneration.VisitVarDecl(Declaration);
        }

        bool TypeErasureGenerator::VisitTypedefDecl(TypedefDecl* Declaration)
        {
            return InterfaceGeneration.VisitTypedefDecl(Declaration);
        }

        bool TypeErasureGenerator::VisitTypeAliasDecl(TypeAliasDecl* Declaration)
        {
            return InterfaceGeneration.VisitTypeAliasDecl(Declaration);
        }

        bool TypeErasureGenerator::VisitFunctionDecl(FunctionDecl* Declaration)
        {
            return InterfaceGeneration.VisitFunctionDecl(Declaration);
        }

        bool TypeErasureGenerator::VisitFunctionTemplateDecl(FunctionTemplateDecl* Declaration)
        {
            return InterfaceGeneration.VisitFunctionTemplateDecl(Declaration);
        }


        class TypeErasureConsumer : public ASTConsumer
        {
        public:
            explicit TypeErasureConsumer(ASTContext& Context,
                                         Preprocessor& PP,
                                         const Config& Configuration)
                : Visitor(Context, PP, Configuration)
            {}

            void HandleTranslationUnit(ASTContext &Context) override
            {
                Visitor.TraverseDecl(Context.getTranslationUnitDecl());
            }

        private:
            TypeErasureGenerator Visitor;
        };


        class TypeErasureAction : public SyntaxOnlyAction
        {
        public:
            explicit TypeErasureAction(const Config& Configuration)
                : Configuration(Configuration)
            {}

            std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& Compiler, llvm::StringRef) override
            {
                return std::unique_ptr<ASTConsumer>(new TypeErasureConsumer(Compiler.getASTContext(),
                                                                            Compiler.getPreprocessor(),
                                                                            Configuration));
            }

        private:
            Config Configuration;
        };


        TypeErasureActionFactory::TypeErasureActionFactory(const type_erasure::Config& Configuration)
            : Configuration(Configuration)
        {}

        std::unique_ptr<FrontendAction> TypeErasureActionFactory::create()
        {
            return std::make_unique<type_erasure::TypeErasureAction>(Configuration);
        }
    }
}
