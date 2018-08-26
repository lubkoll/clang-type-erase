#include "SimpleInterfaceWriter.h"

#include "clang/AST/Comment.h"
#include "clang/Frontend/PreprocessorOutputOptions.h"
#include "clang/Lex/Token.h"

#include "PreprocessorCallback.h"
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
            const auto WRAPPER = "Wrapper";

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

                std::ifstream InputFile(FileName);
                std::string Buffer;
                int Counter = 0;
                while(!InputFile.eof())
                {
                    ++Counter;
                    getline(InputFile, Buffer);

                    const std::size_t LineBegin = Counter == FirstLine ? FirstColumn-1 : 0;
                    const std::size_t LineEnd = Counter == LastLine ? std::size_t(LastColumn) : Buffer.size();

                    Buffer = std::string(begin(Buffer) + LineBegin,
                                         begin(Buffer) + LineEnd);

                    if(Counter >= FirstLine &&
                       Counter <= LastLine)
                        File << Buffer << "\n";
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
                                                        "") << (Counter < LastLine ? "\n" : "");
                }
            }


            template <class Decl,
                      std::enable_if_t<!std::is_same<Decl, TypeAliasDecl>::value &&
                                       !std::is_same<Decl, TypedefDecl>::value>* = nullptr>
            void copy(std::ostream& File,
                      const Decl& Declaration,
                      const SourceManager& SM)
            {
                copy(File,
                     Declaration.getSourceRange().getBegin().printToString(SM),
                     Declaration.getSourceRange().getEnd().printToString(SM));
                File << '\n';
            }

            template <class Decl,
                      std::enable_if_t<std::is_same<Decl, TypeAliasDecl>::value ||
                                       std::is_same<Decl, TypedefDecl>::value>* = nullptr>
            void copy(std::ostream& File,
                      const Decl& Declaration,
                      const SourceManager& SM)
            {
                File << "using " << Declaration.getNameAsString() << " = "
                     << Declaration.getUnderlyingType().getAsString() << ";\n";
            }

            void copyComment(std::ostream& File,
                             const comments::FullComment& Declaration,
                             const SourceManager& SM)
            {
                copyComment(File,
                            Declaration.getSourceRange().getBegin().printToString(SM),
                            Declaration.getSourceRange().getEnd().printToString(SM));
                File << '\n';
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

                Stream << "<!std::is_same<" << decayed(Type, Configuration) << "," << ClassName << ">::value>";
//                Stream << "<" << DetailNamespace << "::Concept<" << ClassName << ", "
//                       << decayed(Type, Configuration) << ">::value>";

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
                     << ": " << Configuration.StorageObject << "(std::forward<T>(value))\n{}" << "\n\n";
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
                auto WriteCast = [&File,&Configuration](const char* ConstSpecifier)
                {
                    File << "template <class T>\n"
                         << ConstSpecifier << "T* " << Configuration.CastName << "() "
                         << ConstSpecifier << "noexcept\n"
                         << "{\n"
                         << "auto wrapper = dynamic_cast<" << ConstSpecifier << WRAPPER << "<T>*>(" << Configuration.StorageObject << ".get());"
                         << "if(wrapper)"
                         << "return &wrapper->impl;\n"
                         << "return nullptr;"
                         << "}\n"
                         << '\n';
                };

                WriteCast("");
                WriteCast("const ");
            }

            void writePrivateSection(std::ostream& File,
                                     const Config& Configuration)
            {
                File << "private:\n" << Configuration.StorageType;
                if(Configuration.CustomFunctionTable) {
                    File << " " << Configuration.StorageObject << ";\n";
                } else if(Configuration.SmallBufferOptimization) {
                    File << "<Interface, " << WRAPPER << "," << Configuration.BufferSize << "> " << Configuration.StorageObject << ";\n";
                } else {
                    File << "<Interface, " << WRAPPER << "> " << Configuration.StorageObject << ";\n";
                }
            }

            template <class Decl>
            bool isMember(const std::string& ClassName,
                          const Decl& Declaration)
            {
                return std::regex_match(Declaration.getQualifiedNameAsString(),
                                        std::regex("(|.*::)" + ClassName + "::" + Declaration.getNameAsString()));

            }
        }


        SimpleInterfaceGenerator::AliasAndStaticMemberEntry::AliasAndStaticMemberEntry(const std::string& ClassName,
                                                                                 std::string&& Entry)
            : ClassName(ClassName),
              Entry(std::move(Entry))
        {}

        SimpleInterfaceGenerator::Interface::Interface(const std::string& ClassName,
                                                 std::string&& Content)
            : ClassName(ClassName),
              Content(std::move(Content))
        {}


        SimpleInterfaceGenerator::SimpleInterfaceGenerator(const char* FileName,
                                               ASTContext& Context,
                                               Preprocessor& PP,
                                               const Config& Configuration,
                                               const std::vector<std::string>& Includes)
            : InterfaceFile(FileName),
              Context(Context),
              PP(PP),
              Configuration(Configuration)
        {
            PP.addPPCallbacks(std::make_unique<PreprocessorCallback>(InterfaceFile, Context, PP));

            utils::writeNoOverwriteWarning(InterfaceFile, Configuration);
            InterfaceFile << "#pragma once\n\n";

            for(const auto& Include : Includes)
                InterfaceFile << Include << '\n';
            InterfaceFile << '\n';

            InterfaceFile << "#include " << Configuration.StorageInclude << "\n\n";

            if(Configuration.CopyOnWrite || !Configuration.CustomFunctionTable) {
                InterfaceFile << "#include <memory>\n#include <type_traits>\n\n";
            }
        }

        SimpleInterfaceGenerator::~SimpleInterfaceGenerator()
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
                            AliasesAndStaticMemberString += Entry.Entry + (Entry.Entry[0] == '/' ? "" : "\n");
                    });
                    Content = std::regex_replace(Content,
                                                 getAliasesAndStaticMemberPlaceholderRegex(Interfaces[I].ClassName),
                                                 AliasesAndStaticMemberString);
                };

                InterfaceFile << Content;
            } catch (...) {}
        }

        bool SimpleInterfaceGenerator::VisitNamespaceDecl(NamespaceDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()))
                return true;

            utils::handleClosingNamespaces(InterfaceFileStream, *Declaration, OpenNamespaces);
            if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                copyComment(InterfaceFileStream, *Comment, Context.getSourceManager());
            InterfaceFileStream << "namespace " << Declaration->getNameAsString() << " {\n";
            OpenNamespaces.push(Declaration->getNameAsString());
            return true;
        }

        bool SimpleInterfaceGenerator::VisitCXXRecordDecl(CXXRecordDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()))
                return true;
            utils::handleClosingNamespaces(InterfaceFileStream, *Declaration, OpenNamespaces);
            if(std::distance(Declaration->method_begin(), Declaration->method_end()) == 0)
                return true;

            const auto ClassName = Declaration->getName().str();
            CurrentClass = ClassName;

            std::stringstream ClassStream;
            std::stringstream BaseImplStream;
            std::stringstream ForwardingStream;
            if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                copyComment(ClassStream, *Comment, Context.getSourceManager());
            ClassStream << "class " << ClassName << "\n"
                        << "{\n";
            ClassStream << "struct Interface { virtual ~Interface() = default; "
                        << "virtual " << (Configuration.CopyOnWrite || Configuration.SmallBufferOptimization
                                          ? "std::shared_ptr<Interface>"
                                          : "std::unique_ptr<Interface>")
                        << "clone() const = 0;";
            BaseImplStream << "template <class Impl> struct " << WRAPPER << " : Interface {"
                           << "template <class T> " << WRAPPER <<"(T&& t) : impl(std::forward<T>(t)){}\n\n";
            if(Configuration.CopyOnWrite || Configuration.SmallBufferOptimization)
                BaseImplStream << "std::shared_ptr<Interface> clone() const {"
                               << "return std::make_shared<" << WRAPPER << "<Impl>>(impl);";
            else
                BaseImplStream << "std::unique_ptr<Interface> clone() const {"
                               << "return std::make_unique<" << WRAPPER << "<Impl>>(impl);";
            BaseImplStream << "}\n\n";

            std::for_each(Declaration->method_begin(),
                          Declaration->method_end(),
                          [this,&Declaration,&ClassName,
                           &ClassStream,&BaseImplStream,&ForwardingStream](const auto& Method)
            {
                if(!Method->isUserProvided())
                    return;
                if(auto Comment = Context.getCommentForDecl(Method, &PP))
                    copyComment(ForwardingStream, *Comment, Context.getSourceManager());

                const auto ReturnType = Method->getReturnType().getAsString(printingPolicy());
                std::stringstream SignatureStream;

                SignatureStream << ReturnType << ' '
                            << Method->getNameAsString() << "(";
                if(!Method->param_empty())
                    std::for_each(Method->param_begin(),
                                  Method->param_end(),
                                  [&Method,&SignatureStream](const auto& Param)
                    {
                        SignatureStream << Param->getType().getAsString(printingPolicy()) << ' ' << Param->getNameAsString();
                        if(&(*(Method->param_end()-1)) != &Param)
                            SignatureStream << ", ";
                    });
                SignatureStream << ")" << (Method->isConst() ? " const" : "");
                const auto Signature = SignatureStream.str();

                auto ForwardingWrite =
                        [&](const auto& StorageObject, const auto& Accessor, auto& Stream, std::string Override="")
                {
                    Stream << Signature << " " << Override << " "
                           << "{\n"
                           << (ReturnType == "void" ? "" : "return ")
                           << StorageObject << Accessor << Method->getNameAsString()
                           << '('
                           << utils::useFunctionArgumentsInInterface(*Method, ClassName, Configuration)
                           << ");\n"
                           << "}\n\n";

                };

                ClassStream << "virtual " << Signature << " = 0;";
                ForwardingWrite("impl", ".", BaseImplStream, "override");
                ForwardingWrite(Configuration.StorageObject, "->", ForwardingStream);
            });

            ClassStream << "};\n\n";
            BaseImplStream << "Impl impl;};\n\n"
                           << "template <class Impl> struct " << WRAPPER << "<std::reference_wrapper<Impl>>"
                           << " : " << WRAPPER << "<Impl&>{"
                           << "template <class T> " << WRAPPER <<"(T&& t) : " << WRAPPER << "<Impl&>(std::forward<T>(t)){}\n\n"
                           << "};\n\n";
            ClassStream << BaseImplStream.str() << "\n"
                        << "public:\n"
                        << getAliasesAndStaticMemberPlaceholder(CurrentClass) << "\n\n";

            writeConstructors(ClassStream, ClassName, Configuration);
            ClassStream << ForwardingStream.str();
            writeOperators(ClassStream, ClassName, Configuration);

            writeCasts(ClassStream, Configuration);
            writePrivateSection(ClassStream, Configuration);
            ClassStream << "};\n";

            InterfaceFileStream << getClassPlaceholder(Interfaces.size());
            Interfaces.emplace_back(CurrentClass, ClassStream.str());
            return true;
        }

        bool SimpleInterfaceGenerator::VisitVarDecl(VarDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()) ||
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
                                                     " " + Declaration->getNameAsString() + Initializer + ";");
            }
            else
                copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        bool SimpleInterfaceGenerator::VisitTypedefDecl(TypedefDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()))
                return true;

            if(isMember(CurrentClass, *Declaration))
            {
                if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                {
                    std::stringstream CommentStream;
                    copyComment(CommentStream, *Comment, Context.getSourceManager());
                    AliasesAndStaticMembers.emplace_back(CurrentClass, CommentStream.str());
                }
                AliasesAndStaticMembers.emplace_back(CurrentClass,
                                                     "typedef " + Declaration->getUnderlyingType().getAsString() +
                                                     " " + Declaration->getNameAsString() + ";");
            }
            else
            {
                if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                    copyComment(InterfaceFileStream, *Comment, Context.getSourceManager());
                copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            }
            return true;
        }

        bool SimpleInterfaceGenerator::VisitTypeAliasDecl(TypeAliasDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()))
                return true;

            if(isMember(CurrentClass, *Declaration))
            {
                if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                {
                    std::stringstream CommentStream;
                    copyComment(CommentStream, *Comment, Context.getSourceManager());
                    AliasesAndStaticMembers.emplace_back(CurrentClass, CommentStream.str());
                }
                AliasesAndStaticMembers.emplace_back(CurrentClass,
                                                     "using " + Declaration->getNameAsString() +
                                                     " = " + Declaration->getUnderlyingType().getAsString() + ";");
            }
            else
            {
                if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                    copyComment(InterfaceFileStream, *Comment, Context.getSourceManager());
                copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            }
            return true;
        }

        bool SimpleInterfaceGenerator::VisitFunctionDecl(FunctionDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()) ||
                    dynamic_cast<CXXMethodDecl*>(Declaration) != nullptr ||
                    Declaration->getTemplatedKind() != FunctionDecl::TK_NonTemplate)
                return true;

            if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                copyComment(InterfaceFileStream, *Comment, Context.getSourceManager());
            copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        bool SimpleInterfaceGenerator::VisitFunctionTemplateDecl(FunctionTemplateDecl* Declaration)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(Declaration->getBeginLoc()) ||
                    dynamic_cast<CXXMethodDecl*>(Declaration) != nullptr)
                return true;

            if(auto Comment = Context.getCommentForDecl(Declaration, &PP))
                copyComment(InterfaceFileStream, *Comment, Context.getSourceManager());
            copy(InterfaceFileStream, *Declaration, Context.getSourceManager());
            return true;
        }

        std::ofstream& SimpleInterfaceGenerator::getFileStream()
        {
            return InterfaceFile;
        }
    }
}
