#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/CompilationDatabase.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "TypeErasureWriter.h"

#include <boost/filesystem.hpp>

#include <chrono>
#include <cstdlib>
#include <memory>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory ClangTypeEraseCategory("clang-type-erase options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

static cl::list<std::string> SourcePaths(cl::Positional,
                                         cl::desc("<source0> [... <sourceN>]"),
                                         cl::OneOrMore);

cl::opt<bool> CopyOnWrite("copy-on-write",
                          cl::desc(R"(enable copy-on-write)"),
                          cl::cat(ClangTypeEraseCategory));
cl::alias CopyOnWriteAlias("cow", cl::desc("Alias for -copy-on-write"),
                           cl::aliasopt(CopyOnWrite));

cl::opt<bool> SmallBufferOptimization("small-buffer-optimization",
                                      cl::desc(R"(enable small buffer optimization)"),
                                      cl::cat(ClangTypeEraseCategory));
cl::alias SmallBufferOptimizationAlias("sbo", cl::desc("Alias for -small-buffer-optimization"),
                                       cl::aliasopt(SmallBufferOptimization));

cl::opt<bool> NonCopyable("non-copyable",
                          cl::desc(R"(non-copyable interfaces)"),
                          cl::cat(ClangTypeEraseCategory));
cl::alias NonCopyableAlias("nc", cl::desc("Alias for -non-copyable"),
                           cl::aliasopt(NonCopyable));

cl::opt<bool> HeaderOnly("header-only",
                         cl::desc(R"(header-only interfaces)"),
                         cl::cat(ClangTypeEraseCategory));
cl::alias HeaderOnlyAlias("ho", cl::desc("Alias for -header-only"),
                          cl::aliasopt(HeaderOnly));

cl::opt<unsigned> BufferSize("buffer-size",
                             cl::desc(R"(buffer size for small buffer optimization)"),
                             cl::init(128),
                             cl::cat(ClangTypeEraseCategory));
cl::alias BufferSizeAlias("bs", cl::desc("Alias for -buffer-size"),
                          cl::aliasopt(BufferSize));

cl::opt<unsigned> CppStandard("cpp-standard",
                              cl::desc(R"(use cpp-standard (11 or 14))"),
                              cl::init(11),
                              cl::cat(ClangTypeEraseCategory));
cl::alias CppStandardAlias("cpp", cl::desc("Alias for -cpp-standard"),
                           cl::aliasopt(CppStandard));

cl::opt<std::string> IncludeDir("include-dir",
                                cl::desc(R"(include directory (used for the determination of inclusion directives)))"),
                                cl::init(""),
                                cl::cat(ClangTypeEraseCategory));

cl::opt<std::string> UtilDir("util-dir",
                             cl::desc(R"(directory to place table_util.h)"),
                             cl::init("util"),
                             cl::cat(ClangTypeEraseCategory));

cl::opt<std::string> UtilInclude("util-include-dir",
                                 cl::desc(R"(string for including table_util.h, incl. angle brackets or parenthesis)"),
                                 cl::init("<util/table_util.h>"),
                                 cl::cat(ClangTypeEraseCategory));

cl::opt<std::string> StorageInclude("storage-include-dir",
                                    cl::desc(R"(string for including storage.h, incl. angle brackets or parenthesis)"),
                                    cl::init("<util/storage.h>"),
                                    cl::cat(ClangTypeEraseCategory));

cl::opt<std::string> CastName("cast-name",
                              cl::desc(R"(name for casting member functions, (defaults to 'target', following std::function))"),
                              cl::init("target"),
                              cl::cat(ClangTypeEraseCategory));

cl::opt<std::string> DetailDir("detail-dir",
                               cl::desc(R"(directory for implementation details)"),
                               cl::init("detail"),
                               cl::cat(ClangTypeEraseCategory));

cl::opt<std::string> TargetDir("target-dir",
                               cl::desc(R"(directory in which the interface will be generated)"),
                               cl::init("."),
                               cl::cat(ClangTypeEraseCategory));

////cl::opt<bool> NoRTTI("no-rtti",
////                          cl::desc(R"(header-only interfaces)"),
////                          cl::cat(ClangTypeEraseCategory));
////cl::alias NoRTTI("nr", cl::desc("Alias for -no-rtti"),
////                           cl::aliasopt(NoRTTI));


// Collect all other arguments, which will be passed to the front end.
static cl::list<std::string>
CC1Arguments(cl::ConsumeAfter,
             cl::desc("<arguments to be passed to front end>..."));


std::string makeAbsolute(const std::string& PathStr)
{
    boost::filesystem::path Path(PathStr);
    if(!Path.is_absolute())
        return (boost::filesystem::current_path() /= Path).c_str();

    return PathStr;
}

std::string concat(const std::string& Path,
                   const std::string& Directory)
{
    return (boost::filesystem::path(Path) /=
            boost::filesystem::path(Directory)).c_str();
}

void readCommandLine(type_erasure::Config& Configuration)
{
    Configuration.CopyOnWrite = CopyOnWrite;
    Configuration.SmallBufferOptimization = SmallBufferOptimization;
    Configuration.NonCopyable = NonCopyable;
    Configuration.HeaderOnly = HeaderOnly;
    //    config.no_rtti = NoRTTI;
    Configuration.BufferSize = BufferSize;
    Configuration.CppStandard = CppStandard;
    Configuration.IncludeDir = makeAbsolute(IncludeDir);
    Configuration.UtilDir = concat(Configuration.IncludeDir,
                                   UtilDir);
    Configuration.UtilInclude = UtilInclude;
    Configuration.StorageInclude = StorageInclude;
    Configuration.CastName = CastName;
    Configuration.TargetDir = concat(Configuration.IncludeDir,
                                     TargetDir);
    Configuration.DetailDir = concat(Configuration.TargetDir,
                                     Configuration.DetailDir);
    Configuration.SourceFile = SourcePaths.front();
    Configuration.StorageType = Configuration.CopyOnWrite ?
                                    (Configuration.SmallBufferOptimization ?
                                         ("clang::type_erasure::SBOCOWStorage<" + std::to_string(Configuration.BufferSize) + ">").c_str() :
                                         "clang::type_erasure::COWStorage") :
                                    (Configuration.SmallBufferOptimization ?
                                         ("clang::type_erasure::SBOStorage<" + std::to_string(Configuration.BufferSize) + ">").c_str() :
                                         "clang::type_erasure::Storage");
}

void copyFile(const std::string& OriginalFile,
              const std::string& TargetDir,
              const std::string& TargetFile)
{
    llvm::outs() << " === Copying '" << TargetFile << "' to '" << TargetDir << "'\n";
    try {
        boost::filesystem::copy(boost::filesystem::path(OriginalFile),
                                boost::filesystem::path(TargetDir) /= boost::filesystem::path(TargetFile));
    } catch (...) {
        llvm::outs() << " === Folder '" << TargetDir << "'' already contains '" << TargetFile << "'. "
                     << "Skipping copy.\n";
    }
}

void formatGeneratedFiles(const type_erasure::Config& Configuration)
{
    llvm::outs() << " === Formatting generated files\n";
    auto Command = Configuration.FormattingCommand + " " + getTableFile(Configuration).c_str();
    std::system(Command.c_str());

    const auto TargetFile = boost::filesystem::path(Configuration.TargetDir) /=
            boost::filesystem::path(Configuration.SourceFile).filename();
    Command = Configuration.FormattingCommand + " " + TargetFile.c_str();
    std::system(Command.c_str());
}

int main(int Argc, const char **Argv)
{
    cl::ParseCommandLineOptions(Argc, Argv, "clang-type-erase.\n");
    type_erasure::Config Configuration;
    readCommandLine(Configuration);

    llvm::outs() << '\n'
                 << " === clang-type-erase ===\n"
                 << " === File: " << Configuration.SourceFile << '\n'
                 << " === Target directory: " << Configuration.TargetDir << '\n'
                 << " ===\n";

    copyFile("/home/lars/Libraries/llvm2/tools/clang/tools/extra/clang-type-erase/files/type_erasure_util.h",
             Configuration.UtilDir,
             "type_erasure_util.h");
    copyFile("/home/lars/Libraries/llvm2/tools/clang/tools/extra/clang-type-erase/files/storage.h",
             Configuration.UtilDir,
             "storage.h");
    copyFile(Configuration.SourceFile,
             Configuration.TargetDir,
             boost::filesystem::path(Configuration.SourceFile).filename().c_str());

    FixedCompilationDatabase Compilations(Twine(boost::filesystem::current_path().c_str()), CC1Arguments);
    ClangTool Tool(Compilations, SourcePaths);

    using namespace std::chrono;
    llvm::outs() << " ===\n === Generating type-erased interface '" << Configuration.SourceFile << "'\n";
    const auto StartTime = steady_clock::now();
    const auto Result = Tool.run(std::make_unique<type_erasure::TypeErasureActionFactory>(Configuration).get());
    llvm::outs() << " === Elapsed time: " << duration_cast<milliseconds>(steady_clock::now() - StartTime).count() << "ms\n"
                 << " ===\n";
    formatGeneratedFiles(Configuration);

    return Result;
}
