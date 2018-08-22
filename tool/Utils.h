#pragma once

#include <ostream>
#include <regex>
#include <stack>
#include <string>
#include <tuple>

namespace clang
{
    struct PrintingPolicy;
    class CXXMethodDecl;
    class QualType;

    namespace type_erasure
    {
        struct Config;

        const PrintingPolicy& printingPolicy();

        namespace utils
        {
            void writeNoOverwriteWarning(std::ostream& OS,
                                         const Config& Configuration);

            bool returnsClassNameRef(const CXXMethodDecl& Method,
                                     const std::string& ClassName);

            std::string getFunctionName(const CXXMethodDecl& Method);

            std::string getFunctionArguments(const CXXMethodDecl& Method,
                                             const std::string& ClassName,
                                             const std::string& Storage,
                                             bool PrintNames=false);

            std::string useFunctionArguments(const CXXMethodDecl& Method,
                                             const std::string& ClassName);

            std::string useFunctionArgumentsInInterface(const CXXMethodDecl& Method,
                                                        const std::string& ClassName,
                                                        const Config& Configuration);

            std::string useFunctionArgumentsInConcepts(const CXXMethodDecl& Method,
                                                       const std::string& ClassName);

            std::tuple<std::string,bool>
            replaceClassNameInReturnType(const CXXMethodDecl& Type,
                                         const std::string& ClassName,
                                         const Config& Configuration);

            std::string getFunctionPointer(const CXXMethodDecl& Method,
                                           const std::string& ClassName,
                                           const Config& Configuration);

            template <class Decl>
            void handleClosingNamespaces(std::ostream& File,
                                         const Decl& Declaration,
                                         std::stack<std::string>& OpenNamespaces)
            {
                if(OpenNamespaces.empty())
                    return;
                auto name = Declaration.getQualifiedNameAsString();
                while(!OpenNamespaces.empty() &&
                      !std::regex_match(name,
                                        std::regex((".*(\\s|::|)" + OpenNamespaces.top() + "::" + Declaration.getNameAsString() + "|\\s+.*").c_str())))
                {
                    File << "}\n";
                    OpenNamespaces.pop();
                }
            }
        }
    }
}
