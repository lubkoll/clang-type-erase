#include "TableWriter.h"

#include "PreprocessorCallback.h"
#include "Utils.h"

#include <algorithm>
#include <sstream>

namespace clang
{
    namespace type_erasure
    {
        namespace
        {
            void writeTable(std::ostream& Stream,
                            const CXXRecordDecl& Declaration,
                            const Config& Configuration)
            {
                Stream << "template < class " << Configuration.InterfaceType
                       << "> struct " << Configuration.FunctionTableType << " {\n";

                std::for_each(Declaration.method_begin(), Declaration.method_end(),
                              [&Stream,&Declaration,&Configuration](const auto& Method)
                {
                    if(!Method->isUserProvided())
                        return;
                    auto FunctionName = utils::getFunctionName(*Method, Configuration);
                    Stream << "using " << FunctionName << "_function = "
                           << utils::getFunctionPointer(*Method, Declaration.getName().str(), Configuration) << " ;\n"
                           << FunctionName << "_function " << FunctionName << " ;\n";
                });
                Stream << "};\n\n";
            }

            void writeWrapper(std::ostream& Stream,
                              const CXXRecordDecl& Declaration,
                              const Config& Configuration)
            {
                Stream << "template < class " << Configuration.InterfaceType << " , class Impl >\n"
                       << "struct execution_wrapper\n"
                       << "{\n";

                std::for_each(Declaration.method_begin(), Declaration.method_end(),
                              [&Declaration,&Configuration,&Stream](const auto& Method)
                {
                    if(!Method->isUserProvided())
                        return;
                    const auto ClassName = Declaration.getName().str();
                    const auto NewReturnType = utils::replaceClassNameInReturnType(*Method,
                                                                                   ClassName,
                                                                                   Configuration);
                    const auto ReturnsClassNameRef = utils::returnsClassNameRef(*Method, ClassName);
                    Stream << "static " << std::get<0>(NewReturnType) << ' ' << utils::getFunctionName(*Method, Configuration)
                           << " ( ";
                    if( std::get<1>(NewReturnType) )
                        Stream << (Method->getReturnType().isConstQualified() ? "const " : "") << Configuration.InterfaceType << " & "
                               << Configuration.InterfaceObject << ", ";
                    Stream << utils::getFunctionArguments(*Method, ClassName, Configuration.StorageType, true)
                           << " )\n{\n"
                           << (Method->getReturnType().getAsString(printingPolicy()) == "void" || ReturnsClassNameRef
                               ? "" : "return ")
                           << "data.template get<Impl>()."
                           << Method->getNameAsString() << " ( "
                           << utils::useFunctionArguments(*Method, ClassName, Configuration)
                           << " );\n"
                           << (ReturnsClassNameRef
                               ? std::string("return ") + Configuration.InterfaceObject + ";\n"
                               : std::string(""))
                           << "}\n\n";
                });
                Stream << "} ;\n\n";
            }

            void writeConcepts(std::ofstream& Stream,
                               const CXXRecordDecl& Declaration,
                               const Config& Configuration)
            {
                std::vector<std::string> Concepts;
                Concepts.reserve(std::distance(Declaration.method_begin(), Declaration.method_end()));

                std::for_each(Declaration.method_begin(),
                              Declaration.method_end(),
                              [&Stream,&Declaration,&Concepts, &Configuration]
                              (const auto& Method)
                {
                    if(!Method->isUserProvided())
                        return;
                    const auto FunctionName = utils::getFunctionName(*Method, Configuration);
                    const auto TryMemFnName = "TryMemFn_" + FunctionName;
                    const auto HasMemFnName = "HasMemFn_" + FunctionName;
                    Concepts.emplace_back(HasMemFnName);

                    Stream << "template < class T >\n"
                           << "using " << TryMemFnName << " = "
                           << "decltype( std::declval<T>()." << Method->getNameAsString() << "("
                           << utils::useFunctionArgumentsInConcepts(*Method, Declaration.getName().str())
                           << ") );\n"
                           << '\n'
                           << "template < class T , class = void >\n"
                           << "struct " << HasMemFnName << " : std::false_type"
                           << "{};\n"
                           << '\n'
                           << "template < class T >\n"
                           << "struct " << HasMemFnName
                           << "< T , type_erasure_table_detail::voider< " << TryMemFnName << " < T > > > : std::true_type"
                           << "{};\n\n";
                });

                Stream << "template < class T >\n"
                              << "using " << "ConceptImpl = type_erasure_table_detail::And< \n";
                for(const auto& Concept : Concepts)
                    Stream << Concept << "< type_erasure_table_detail::remove_reference_wrapper_t<T> >"
                           << (&Concepts.back() == &Concept ? "" : ",\n");
                Stream << ">;\n\n"
                       << "template <class Impl, class T, bool = std::is_base_of<Impl, T>::value>\n"
                       << "struct Concept : std::false_type\n"
                       << "{};\n\n"
                       << "template <class Impl, class T>\n"
                       << "struct Concept<Impl, T, false> : ConceptImpl <T>\n"
                       << "{};\n";
            }
        }

        TableGenerator::TableGenerator(const char* FileName,
                                       ASTContext& Context,
                                       Preprocessor& PP,
                                       const Config& Configuration)
            : TableFile(FileName),
              Context(Context),
              Configuration(Configuration)
        {
            PP.addPPCallbacks(std::make_unique<PreprocessorCallback>(TableFile, Context, PP));

            utils::writeNoOverwriteWarning(TableFile, Configuration);
            TableFile << "#pragma once\n\n"
                        << "#include " << Configuration.UtilInclude << '\n'
                        << "#include " << Configuration.StorageInclude << "\n\n";
            if(Configuration.CopyOnWrite)
                TableFile << "#include <memory>\n\n";
        }

        TableGenerator::~TableGenerator()
        {
            while(!OpenNamespaces.empty())
            {
                TableFile << "}\n";
                OpenNamespaces.pop();
            }
        }

        bool TableGenerator::VisitNamespaceDecl(NamespaceDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()))
                return true;
            utils::handleClosingNamespaces(TableFile, *Declaration, OpenNamespaces);
            TableFile << "namespace " << Declaration->getNameAsString() << " {\n";
            OpenNamespaces.push(Declaration->getNameAsString());
            return true;
        }

        bool TableGenerator::VisitCXXRecordDecl(CXXRecordDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()))
                return true;
            utils::handleClosingNamespaces(TableFile, *Declaration, OpenNamespaces);
            if(std::distance(Declaration->method_begin(), Declaration->method_end()) == 0)
                return true;

            TableFile << "namespace " << Declaration->getName().str() << "Detail {\n";

            writeTable(TableFile, *Declaration, Configuration);
            writeWrapper(TableFile, *Declaration, Configuration);
            writeConcepts(TableFile, *Declaration, Configuration);

            TableFile << "}\n\n";
            return true;
        }

        std::ofstream& TableGenerator::getFileStream()
        {
            return TableFile;
        }
    }
}
