#include "InterfaceWriter.h"

#include "clang/AST/Comment.h"
#include "clang/Frontend/PreprocessorOutputOptions.h"
#include "clang/Lex/Token.h"

#include "Utils.h"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <regex>
#include <sstream>
#include <tuple>

namespace clang
{
    namespace type_erasure
    {
        namespace
        {
            const auto ConstructorPlaceholder = "[[CONSTRUCTOR_PLACEHOLDER]]";
            const auto ConstructorPlaceholderRegex = std::regex("\\[\\[CONSTRUCTOR_PLACEHOLDER\\]\\]");

            std::string getAliasesAndStaticMemberPlaceholderImpl(const std::string& ClassName)
            {
                return  ClassName + "_ALIAS_AND_STATIC_MEMBER_PLACEHOLDER";
            }

            std::string getAliasesAndStaticMemberPlaceholder(const std::string& ClassName)
            {
                return  "[[" + getAliasesAndStaticMemberPlaceholderImpl(ClassName) + "]]";
            }

            std::regex getAliasesAndStaticMemberPlaceholderRegex(const std::string& ClassName)
            {
                return  std::regex("\\[\\[" + getAliasesAndStaticMemberPlaceholderImpl(ClassName) + "\\]\\]");
            }


            template <class T>
            std::string getClassPlaceholderImpl(T Index)
            {
                return  "PLACEHOLDER" + std::to_string(Index);
            }

            template <class T>
            std::string getClassPlaceholder(T Index)
            {
                return  "[[" + getClassPlaceholderImpl(Index) + "]]";
            }

            template <class T>
            std::regex getClassPlaceholderRegex(T Index)
            {
                return  std::regex("\\[\\[" + getClassPlaceholderImpl(Index) + "\\]\\]");
            }


            std::tuple<std::string,int,int>
            getFilenameAndPosition(const std::string& Location)
            {
                const auto FirstSeparatorPos = Location.find_first_of(':');
                const auto SecondSeparatorPos = Location.find_last_of(':');
                const auto LocBegin = begin(Location);
                return std::make_tuple(std::string(LocBegin, LocBegin+FirstSeparatorPos),
                                       stoi(std::string(LocBegin+FirstSeparatorPos+1, LocBegin+SecondSeparatorPos)),
                                       stoi(std::string(LocBegin+SecondSeparatorPos+1, end(Location))));
            }


            void copy(std::ostream& File,
                       const std::string& StartLocation,
                       const std::string& EndLocation)
            {
                std::string FileName;
                int FirstLine = -1;
                int FirstColumn = -1;
                int LastLine = -1;
                int LastColumn = -1;
                std::tie(FileName, FirstLine, FirstColumn) = getFilenameAndPosition(StartLocation);
                std::tie(std::ignore, LastLine, LastColumn) = getFilenameAndPosition(EndLocation);
                assert(FirstLine > 0);
                assert(FirstColumn > 0);
                assert(LastLine > 0);
                assert(LastColumn > 0);

                llvm::outs() << "Copying range " << FirstLine << ":" << FirstColumn << " - " << LastLine << ":" << LastColumn << "\n";

                std::ifstream InputFile(FileName);
                std::string Buffer;
                int Counter = 0;
                while(!InputFile.eof())
                {
                    ++Counter;
                    getline(InputFile, Buffer);

                    std::size_t LineBegin = 0;
                    std::size_t LineEnd = Buffer.size();
                    if(Counter == FirstLine)
                        LineBegin = FirstColumn-1;
//                    if(Counter == LastLine)
//                        LineEnd = LastColumn;

                    Buffer = std::string(begin(Buffer) + LineBegin,
                                         begin(Buffer) + LineEnd);

                    if(Counter >= FirstLine &&
                       Counter <= LastLine)
                    {
                        llvm::outs() << "copying line " << Counter << ": " << Buffer << "\n";
                        File << Buffer << '\n';
                    }
                }
            }

            void copyComment(std::ostream& File,
                             const std::string& StartLocation,
                             const std::string& EndLocation)
            {
                std::string FileName;
                int FirstLine = -1;
                int FirstColumn = -1;
                int LastLine = -1;
                int LastColumn = -1;
                std::tie(FileName, FirstLine, FirstColumn) = getFilenameAndPosition(StartLocation);
                std::tie(std::ignore, LastLine, LastColumn) = getFilenameAndPosition(EndLocation);
                assert(FirstLine > 0);
                assert(FirstColumn > 0);
                assert(LastLine > 0);
                assert(LastColumn > 0);

                std::ifstream InputFile(FileName);
                std::string Buffer;
                int Counter = 0;
                while(!InputFile.eof())
                {
                    ++Counter;
                    getline(InputFile, Buffer);
                    if(Counter < FirstLine || Counter > LastLine)
                        continue;

                    std::size_t LineBegin = Counter == FirstLine ? FirstColumn-1 : 0;
                    std::size_t LineEnd = Counter == LastLine ? std::size_t(LastColumn) : Buffer.size();
                    assert(LineBegin <= LineEnd);

                    if(Counter == FirstLine || Counter == LastLine)
                        Buffer = Buffer.substr(LineBegin, LineEnd - LineBegin);

                    File << "///" << std::regex_replace(Buffer,
                                                        std::regex("\\*/|\\s*(\\*\\s|///|//\\!)"),
                                                        "") << '\n';
                }
            }

            template <class Decl>
            void copy(std::ostream& File,
                      const Decl& Declaration,
                      const SourceManager& SM)
            {
                copy(File,
                     Declaration.getSourceRange().getBegin().printToString(SM),
                     Declaration.getSourceRange().getEnd().printToString(SM));
                File << '\n';
            }

            void copyComment(std::ostream& File,
                             const comments::FullComment& Declaration,
                             const SourceManager& SM)
            {
                copyComment(File,
                            Declaration.getSourceRange().getBegin().printToString(SM),
                            Declaration.getSourceRange().getEnd().printToString(SM));
            }

            std::string decayed(const std::string& Type, const Config& Configuration)
            {
                if(Configuration.CppStandard >= 14)
                    return "std::decay_t<" + Type + ">";
                return "typename std::decay<" + Type + ">::type";
            }

            std::string enable_if(const std::string& Type,
                                  const std::string& ClassName,
                                  const std::string& DetailNamespace,
                                  const Config& Configuration)
            {
                std::stringstream Stream;
                if(Configuration.CppStandard >= 14)
                    Stream << "std::enable_if_t";
                else
                    Stream << "typename std::enable_if";

                Stream << "<" << DetailNamespace << "::Concept<" << ClassName << ", "
                       << decayed(Type, Configuration) << ">::value>";

                if(Configuration.CppStandard < 14)
                    Stream << "::type";

                Stream << "* = nullptr";
                return Stream.str();
            }

            void writeConstructors(std::ostream& File,
                                   const std::string& ClassName,
                                   const Config& Configuration)
            {
                // default constructor
                File << ClassName << "() noexcept = default;\n\n";

                auto decT = decayed("T", Configuration);
                // construct from implementation
                File << "template <class T,\n"
                     << enable_if("T", ClassName, ClassName + "Detail", Configuration) << ">\n"
                     << ClassName << "(T&& value)\n"
                     << ": " << Configuration.FunctionTableObject << "( {\n";
                File << ConstructorPlaceholder << "} )";
                File << ", \n" << Configuration.StorageObject << "(std::forward<T>(value))\n{}" << "\n\n";
            }

            void writeOperators(std::ostream& File,
                                const std::string& ClassName,
                                const Config& Configuration)
            {
                // assignment
                File << "template <class T,\n"
                     << enable_if("T", ClassName, ClassName + "Detail", Configuration) << ">\n"
                     << ClassName << "& operator=(T&& value)\n{\n"
                     << "return * this = " << ClassName << " ( std::forward<T>(value) );\n"
                     << "}\n\n";

                // operator bool
                File << "explicit operator bool () const noexcept\n{\n"
                     << "return bool(" << Configuration.StorageObject << ");\n}\n\n";
            }

            void writeCasts(std::ostream& File,
                             const Config& Configuration)
            {
                auto Write = [&File,&Configuration](bool IsConst)
                {
                    File << "template <class T>\n"
                         << (IsConst ? "const " : "") << "T* " << Configuration.CastName << "() "
                         << (IsConst ? "const " : "") << "noexcept\n"
                         << "{\n"
                         << "return " << Configuration.StorageObject << ".template target<T>();\n"
                         << "}\n"
                         << '\n';
                };

                Write(false);
                Write(true);
            }

            void writePrivateSection(std::ostream& File,
                                     const std::string& ClassName,
                                     const Config& Configuration)
            {
                File << "private:\n"
                     << ClassName << "Detail::" << Configuration.FunctionTableType << "<" << ClassName
                     << "> " << Configuration.FunctionTableObject << ";\n"
                     << Configuration.StorageType << " " << Configuration.StorageObject << ";\n";
            }

            template <class Decl>
            bool isMember(const std::string& ClassName,
                          const Decl& Declaration)
            {
                return std::regex_match(Declaration.getQualifiedNameAsString(),
                                        std::regex("(|.*::)" + ClassName + "::" + Declaration.getNameAsString()));

            }
        }


        InterfaceGenerator::AliasAndStaticMemberEntry::AliasAndStaticMemberEntry(const std::string& ClassName,
                                                                                 std::string&& Entry)
            : ClassName(ClassName),
              Entry(std::move(Entry))
        {}

        InterfaceGenerator::Interface::Interface(const std::string& ClassName,
                                                 std::string&& Content)
            : ClassName(ClassName),
              Content(std::move(Content))
        {}


        InterfaceGenerator::InterfaceGenerator(const char* FileName,
                                               const ASTContext& Context,
                                               Preprocessor& PP,
                                               const Config& Configuration,
                                               const std::vector<std::string>& Includes)
            : InterfaceFile(FileName),
              Context(Context),
              PP(PP),
              Configuration(Configuration)
        {
            utils::writeNoOverwriteWarning(InterfaceFile, Configuration);
            InterfaceFile << "#pragma once\n\n";
            if(Configuration.CopyOnWrite)
                InterfaceFile << "#include <memory>\n\n";

            for(const auto& Include : Includes)
                InterfaceFile << Include << '\n';

            InterfaceFile << "#include " << Configuration.StorageInclude << "\n";
            InterfaceFile << '\n';
        }

        InterfaceGenerator::~InterfaceGenerator()
        {
            while(!OpenNamespaces.empty())
            {
                InterfaceFileStream << "}\n";
                OpenNamespaces.pop();
            }

            try
            {
                auto Content = InterfaceFileStream.str();
                using Index = decltype(Interfaces.size());
                for(Index I = 0, E = Interfaces.size(); I<E; ++I)
                {
                    Content = std::regex_replace(Content, getClassPlaceholderRegex(I), Interfaces[I].Content);
                    std::string AliasesAndStaticMemberString;
                    std::for_each(begin(AliasesAndStaticMembers), end(AliasesAndStaticMembers),
                                  [&AliasesAndStaticMemberString, ClassName = Interfaces[I].ClassName]
                                  (const auto& Entry)
                    {
                        if(ClassName == Entry.ClassName)
                            AliasesAndStaticMemberString += Entry.Entry + ";\n";
                    });
                    Content = std::regex_replace(Content,
                                                 getAliasesAndStaticMemberPlaceholderRegex(Interfaces[I].ClassName),
                                                 AliasesAndStaticMemberString);
                };

                InterfaceFile << Content;
            } catch (...) {}
        }

        bool InterfaceGenerator::VisitNamespaceDecl(NamespaceDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()))
                return true;
            utils::handleClosingNamespaces(InterfaceFileStream, *Declaration, OpenNamespaces);
            InterfaceFileStream << "namespace " << Declaration->getNameAsString() << " {\n";
            OpenNamespaces.push(Declaration->getNameAsString());
            return true;
        }

        bool InterfaceGenerator::VisitCXXRecordDecl(CXXRecordDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()))
                return true;
            utils::handleClosingNamespaces(InterfaceFileStream, *Declaration, OpenNamespaces);
            if(std::distance(Declaration->method_begin(), Declaration->method_end()) == 0)
                return true;

            const auto ClassName = Declaration->getName().str();
            CurrentClass = ClassName;

            std::stringstream ClassStream;
            if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                copyComment(ClassStream, *Comment, Context.getSourceManager());
            ClassStream << "class " << ClassName << "\n"
                        << "{\n"
                        << "public:\n"
                        << getAliasesAndStaticMemberPlaceholder(CurrentClass) << "\n\n";
            writeConstructors(ClassStream, ClassName, Configuration);
            writeOperators(ClassStream, ClassName, Configuration);

            std::vector<std::string> FunctionNames;
            std::for_each(Declaration->method_begin(),
                          Declaration->method_end(),
                          [this,&Declaration,&FunctionNames,&ClassName,&ClassStream](const auto& Method)
            {
                if(auto Comment = Context.getCommentForDecl(Method, &PP))
                    copyComment(ClassStream, *Comment, Context.getSourceManager());

                FunctionNames.emplace_back(utils::getFunctionName(*Method));
                const auto ReturnType = Method->getReturnType().getAsString(printingPolicy());
                ClassStream << ReturnType << ' '
                            << Method->getNameAsString() << "(";
                if(!Method->param_empty())
                    std::for_each(Method->param_begin(),
                                  Method->param_end(),
                                  [&Method,&ClassStream](const auto& Param)
                    {
                        ClassStream << Param->getType().getAsString(printingPolicy()) << ' ' << Param->getNameAsString();
                        if(&(*(Method->param_end()-1)) != &Param)
                            ClassStream << ", ";
                    });


                ClassStream << ")" << (Method->isConst() ? " const" : "")
                            << "{\n"
                            << "assert(" << Configuration.StorageObject << ");\n"
                            << (ReturnType == "void" ? "" : "return ")
                            << Configuration.FunctionTableObject << "." << utils::getFunctionName(*Method)
                            << '('
                            << (utils::returnsClassNameRef(*Method, ClassName) ? "*this, " : "")
                            << Configuration.StorageObject
                            << (Method->param_empty() ? "" : ", ")
                            << utils::useFunctionArgumentsInInterface(*Method, ClassName, Configuration)
                            << ");\n"
                            << "}\n\n";
            });

            writeCasts(ClassStream, Configuration);
            writePrivateSection(ClassStream, ClassName, Configuration);
            ClassStream << "};\n";

            const auto Initializer = std::accumulate(begin(FunctionNames),
                                                     end(FunctionNames),
                                                     std::string(),
                                                     [&ClassName,&FunctionNames](std::string Initializer, const std::string& FunctionName)
            {
                return Initializer += "&" + ClassName + "Detail::execution_wrapper<" + ClassName +
                                      ", type_erasure_table_detail::remove_reference_wrapper_t<std::decay_t<T>>>::" +
                                      FunctionName + (&FunctionNames.back() != &FunctionName ? ", " : "");
            });

            InterfaceFileStream << getClassPlaceholder(Interfaces.size());
            Interfaces.emplace_back(CurrentClass,
                                    std::regex_replace(ClassStream.str(), ConstructorPlaceholderRegex, Initializer));
            return true;
        }

        bool InterfaceGenerator::VisitVarDecl(VarDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()) ||
                    dynamic_cast<ParmVarDecl*>(Declaration) != nullptr)
                return true;

            if(isMember(CurrentClass, *Declaration))
            {
                std::string Initializer;
                if(Declaration->hasInit())
                {
                    std::string Init;
                    llvm::raw_string_ostream StrStream(Init);
                    Declaration->getInit()->dump(StrStream);
                    StrStream.str();

                    const auto RInitEnd = std::find_if(Init.rbegin(), Init.rend(),
                                                       [](auto C) { return C=='\''; });
                    Initializer = std::accumulate(Init.rbegin(), RInitEnd,
                                                  std::string(" = "),
                                                  [this](std::string Str, auto C)
                    { return (C=='\n' || C == '\r' || C == '\t') ? Str : Str + C; });
                }

                AliasesAndStaticMembers.emplace_back(CurrentClass,
                                                     (Declaration->isStaticDataMember() ? "static " : "") +
                                                     Declaration->getType().getAsString(printingPolicy()) +
                                                     " " + Declaration->getNameAsString() + Initializer);
            }
            else
                copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        bool InterfaceGenerator::VisitTypedefDecl(TypedefDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()))
                return true;

            if(isMember(CurrentClass, *Declaration))
                AliasesAndStaticMembers.emplace_back(CurrentClass,
                                                     "typedef " + Declaration->getUnderlyingType().getAsString() +
                                                     " " + Declaration->getNameAsString());
            else
                copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        bool InterfaceGenerator::VisitTypeAliasDecl(TypeAliasDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()))
                return true;

            if(isMember(CurrentClass, *Declaration))
                AliasesAndStaticMembers.emplace_back(CurrentClass,
                                                     "using " + Declaration->getNameAsString() +
                                                     " = " + Declaration->getUnderlyingType().getAsString());
            else
                copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        bool InterfaceGenerator::VisitFunctionDecl(FunctionDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()) ||
                    dynamic_cast<CXXMethodDecl*>(Declaration) != nullptr ||
                    Declaration->getTemplatedKind() != FunctionDecl::TK_NonTemplate)
                return true;

            copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        bool InterfaceGenerator::VisitFunctionTemplateDecl(FunctionTemplateDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getLocStart()) ||
                    dynamic_cast<CXXMethodDecl*>(Declaration) != nullptr)
                return true;

            copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        std::ofstream& InterfaceGenerator::getFileStream()
        {
            return InterfaceFile;
        }
    }
}
