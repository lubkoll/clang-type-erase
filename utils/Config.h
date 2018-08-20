#pragma once

#include <ostream>
#include <string>

namespace clang
{
    namespace type_erasure
    {
        struct Config
        {
            Config();

            bool CopyOnWrite = false;
            bool SmallBufferOptimization = false;
            bool NonCopyable = false;
            bool HeaderOnly = true;
            bool NoRTTI = false;
            bool NoOverwriteWarning = false;
            bool UseCppConcepts = false;
            bool CustomFunctionTable = false;
            unsigned BufferSize = 128;
            unsigned CppStandard = 11;
            std::string InterfaceType = "Interface";
            std::string InterfaceObject = "interface";
            std::string StorageObject = "impl_";
            std::string FunctionTableType = "Table";
            std::string FunctionTableObject = "function_";
            std::string CastName = "target";
            std::string DetailDir = "detail";
            std::string UtilInclude = "<util/type_erasure_util.h>";
            std::string StorageInclude = "<util/storage.h>";
            std::string UtilDir = "util";
            std::string SourceFile = "";
            std::string IncludeDir = "";
            std::string TargetDir = "/home/lars/tmp";
            std::string StorageType = CopyOnWrite ?
                                          (SmallBufferOptimization ?
                                               "clang::type_erasure::COWSBOStorage" :
                                               "clang::type_erasure::COWStorage") :
                                          (SmallBufferOptimization ?
                                               "clang::type_erasure::SBOStorage" :
                                               "clang::type_erasure::Storage");
            std::string FormattingCommand = "clang-format-3.8 -i";
        };

        std::ostream& operator<<(std::ostream& OS, const Config& Configuration);
    }
}
