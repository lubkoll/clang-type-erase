#include "Utils.h"

#include "Config.h"

#include "clang/AST/DeclCXX.h"
#include "clang/AST/PrettyPrinter.h"

#include <regex>
#include <sstream>

namespace clang
{
    namespace type_erasure
    {
        const PrintingPolicy& printingPolicy()
        {
            static PrintingPolicy PrintUnqualified{LangOptions{}};
            PrintUnqualified.adjustForCPlusPlus();
            PrintUnqualified.SuppressScope = true;
            return PrintUnqualified;
        }

        namespace utils
        {
            namespace
            {
                bool ContainsClassName(const std::string& Str,
                                       const std::string& ClassName)
                {
                    return std::regex_match(Str, std::regex("(.*\\s+|.*::|)(" + ClassName + ")(\\s+.*|)"));
                }

                std::string replaceClassNameInParam(const std::string& Str,
                                                    const std::string& ClassName,
                                                    const std::string& NewClassName)
                {
                    return std::regex_replace(Str, std::regex(ClassName), NewClassName);
                }


                bool isMovable(const std::string& Type)
                {
                    return std::regex_match(Type, std::regex(".*&&\\s*")) ||
                            !std::regex_match(Type, std::regex(".*(&|\\*)\\s*"));
                }

                std::string adjustArgument(const std::string& ArgType,
                                           const std::string& ArgName,
                                           const std::string& ClassName)
                {
                    if(!ContainsClassName(ArgType, ClassName))
                        return ArgName;

                    const auto IsPtr = std::regex_match(ArgType, std::regex(".*\\*"));
                    return (IsPtr ? "& " : "") + ArgName + ".template get<Impl>()";
                }

                std::string adjustArgumentForInterface(const std::string& ArgType,
                                                       const std::string& ArgName,
                                                       const std::string& ClassName,
                                                       const Config& Configuration)
                {
                    if(ContainsClassName(ArgType, ClassName))
                        return ArgName + "." + Configuration.StorageObject;
                    return ArgName;
                }
            }


            void writeNoOverwriteWarning(std::ostream& OS,
                                         const Config& Configuration)
            {
                if(Configuration.NoOverwriteWarning)
                    return;
                OS << "// This file was automatically generated using clang-type-erase.\n"
                   << "// Please do not modify.\n\n";
            }


            bool returnsClassNameRef(const CXXMethodDecl& Method,
                                     const std::string& ClassName)
            {
                return std::regex_match(Method.getReturnType().getAsString(printingPolicy()),
                                        std::regex("(.*\\s+|.*::|)(" + ClassName + ")(\\s*&\\s*)" ));
            }


            std::string getFunctionName(const CXXMethodDecl& Method)
            {
                auto Name = Method.getNameAsString();
                std::stringstream Stream;
                if( std::regex_match(Name, std::regex("operator\\S+")) )
                {
                    if(Name == "operator()")
                        Stream << "call";
                    else if(Name == "operator=")
                        Stream << "assign";
                    else if(Name == "operator+=")
                        Stream << "add";
                    else if(Name == "operator-=")
                        Stream << "subtract";
                    else if(Name == "operator*=")
                        Stream << "multiply";
                    else if(Name == "operator/)")
                        Stream << "divide";
                    else if(Name == "operator-")
                        Stream << "negate";
                    else if(Name == "operator==")
                        Stream << "compare";
                }
                else Stream << Name;

                std::for_each(Method.param_begin(),
                              Method.param_end(),
                              [&Stream](const auto& Param)
                {
                    auto ParamType = Param->getType().getAsString(printingPolicy());
                    ParamType = std::regex_replace(ParamType, std::regex("(&)"), "_ref");
                    ParamType = std::regex_replace(ParamType, std::regex("(\\*)"), "_ptr");
                    ParamType = std::regex_replace(ParamType, std::regex("(\\s+|::|\\(|\\)|<|>|\\[|\\])"), "_");
                    ParamType = std::regex_replace(ParamType, std::regex("(__+)"), "_");
                    Stream << '_' << ParamType;
                });

                return Stream.str();
            }


            std::string getFunctionArguments(const CXXMethodDecl& Method,
                                             const std::string& ClassName,
                                             const std::string& Storage,
                                             bool PrintNames)
            {
                std::stringstream Stream;
                Stream << (Method.isConst() ? "const " : "") << Storage << " & " << (PrintNames ? " data" : "");
                if(!Method.param_empty())
                    std::for_each(Method.param_begin(),
                                  Method.param_end(),
                                  [&Stream,&ClassName,&PrintNames,&Storage](const auto& Param)
                    {
                        Stream << " , " << replaceClassNameInParam(Param->getType().getAsString(printingPolicy()), ClassName, Storage)
                               << (PrintNames ? (' ' + Param->getNameAsString()).c_str() : "");
                    });

                return Stream.str();
            }


            std::string useFunctionArguments(const CXXMethodDecl& Method,
                                             const std::string& ClassName)
            {
                std::stringstream Stream;
                const auto Writer = [&Stream,&ClassName](const auto& Param)
                {
                    const auto ParamType = Param->getType().getAsString(printingPolicy());
                    const auto IsMovable = isMovable(ParamType);
                    Stream << (IsMovable ? "std::move ( " : "")
                           << adjustArgument(ParamType, Param->getNameAsString(), ClassName)
                           << (IsMovable ? " )" : "");
                };

                if(!Method.param_empty())
                {
                    std::for_each(Method.param_begin(),
                                  Method.param_end() - 1,
                                  [&Writer,&Stream](const auto& Param)
                    {
                        Writer(Param);
                        Stream << " , ";
                    });
                    Writer(*(Method.param_end() - 1));
                }

                return Stream.str();
            }


            std::string useFunctionArgumentsInInterface(const CXXMethodDecl& Method,
                                                        const std::string& ClassName,
                                                        const Config& Configuration)
            {
                std::stringstream Stream;
                const auto Writer = [&Stream,&ClassName,&Configuration](const auto& Param)
                {
                    const auto ParamType = Param->getType().getAsString(printingPolicy());
                    const auto IsMovable = isMovable(ParamType);
                    Stream << (IsMovable ? "std::move ( " : "")
                           << adjustArgumentForInterface(ParamType,
                                                         Param->getNameAsString(),
                                                         ClassName,
                                                         Configuration)
                           << (IsMovable ? " )" : "");
                };

                if(!Method.param_empty())
                {
                    std::for_each(Method.param_begin(),
                                  Method.param_end() - 1,
                                  [&Writer,&Stream](const auto& Param)
                    {
                        Writer(Param);
                        Stream << " , ";
                    });
                    Writer(*(Method.param_end() - 1));
                }

                return Stream.str();
            }


            std::string useFunctionArgumentsInConcepts(const CXXMethodDecl& Method,
                                                       const std::string& ClassName)
            {
                std::stringstream Stream;
                const auto Writer = [&Stream,&ClassName](const auto& Param)
                {
                    Stream << "std::declval< "
                           << std::regex_replace(Param->getType().getAsString(printingPolicy()), std::regex(ClassName), "T")
                           << " >()";
                };

                if(!Method.param_empty())
                {
                    std::for_each(Method.param_begin(),
                                  Method.param_end() - 1,
                                  [&Writer,&Stream](const auto& Param)
                    {
                        Writer(Param);
                        Stream << " , ";
                    });
                    Writer(*(Method.param_end() - 1));
                }

                return Stream.str();
            }


            std::tuple<std::string,bool>
            replaceClassNameInReturnType(const CXXMethodDecl& Method,
                                         const std::string& ClassName,
                                         const Config& Configuration)
            {
                if( !returnsClassNameRef(Method, ClassName) )
                {
                    if(Method.getReturnType().getAsString(printingPolicy()) == ClassName)
                        return std::make_tuple(Configuration.InterfaceType, false);
                    return std::make_tuple(Method.getReturnType().getAsString(printingPolicy()),
                                           false);
                }

                const bool IsReference = std::regex_match(Method.getReturnType().getAsString(printingPolicy()), 
                                                          std::regex(".*&\\s*"));
                auto Str = (Method.getReturnType().isConstQualified() ? "const " : "") + 
                           std::string(Configuration.InterfaceType) +
                           (IsReference ? " &" : "");
                return std::make_tuple(std::move(Str),IsReference);
            }


            std::string getFunctionPointer(const CXXMethodDecl& Method,
                                           const std::string& ClassName,
                                           const Config& Configuration)
            {
                const auto ReturnType = replaceClassNameInReturnType(Method,
                                                                     ClassName,
                                                                     Configuration);
                std::stringstream Stream;
                Stream << std::get<0>(ReturnType) << " ( * ) ( ";
                if( std::get<1>(ReturnType) )
                    Stream << (Method.getReturnType().isConstQualified() ? "const " : "") << Configuration.InterfaceType << " & , ";
                Stream << getFunctionArguments(Method, ClassName, Configuration.StorageType) << " )";
                return Stream.str();
            }
        }
    }
}
