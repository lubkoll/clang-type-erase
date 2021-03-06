#include "PreprocessorCallback.h"
#include "clang/AST/ASTContext.h"

namespace clang
{
    namespace type_erasure
    {
        PreprocessorCallback::PreprocessorCallback(std::ofstream& Stream,
                                                   ASTContext& Context,
                                                   Preprocessor &PP,
                                                   const Config& Configuration)
            : Stream(Stream),
              Context(Context),
              PP(PP),
              Configuration(Configuration)
        {}

        void PreprocessorCallback::InclusionDirective(
                SourceLocation HashLoc, const Token& IncludeTok,
                llvm::StringRef, bool IsAngled,
                CharSourceRange FilenameRange, const FileEntry*,
                llvm::StringRef, llvm::StringRef,
                const Module*, SrcMgr::CharacteristicKind)
        {
            if(!Context.getSourceManager().isWrittenInMainFile(HashLoc))
                return;

            auto IncludePath = getSourceString(FilenameRange).str();

            if(!Configuration.CustomFunctionTable && IncludePath == "<type_traits>")
                return;
            if((Configuration.CopyOnWrite || !Configuration.CustomFunctionTable) && IncludePath == "<memory>")
                return;

            if(IsAngled) {
                IncludePath.front() = '<';
                IncludePath.back() = '>';
            } else
                IncludePath.front() = IncludePath.back() = '"';

            Stream << '#' + PP.getSpelling(IncludeTok) + ' ' + std::move(IncludePath) + "\n";
        }

        llvm::StringRef PreprocessorCallback::getSourceString(CharSourceRange Range)
        {
            auto CharBegin = PP.getSourceManager().getCharacterData(Range.getBegin());
            auto CharEnd = PP.getSourceManager().getCharacterData(Range.getEnd());
            return llvm::StringRef(CharBegin, CharEnd - CharBegin);
        }
    }
}
