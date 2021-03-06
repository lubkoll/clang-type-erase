#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/StringRef.h"

#include "Config.h"

#include <fstream>

namespace clang
{
    namespace type_erasure
    {
        class PreprocessorCallback : public PPCallbacks {
        public:
            PreprocessorCallback(std::ofstream& Stream,
                                 ASTContext& Context,
                                 Preprocessor &PP,
                                 const Config& Configuration);

            void InclusionDirective(SourceLocation HashLoc,
                                    const Token &IncludeTok, llvm::StringRef, bool IsAngled,
                                    CharSourceRange FilenameRange, const FileEntry *,
                                    llvm::StringRef, llvm::StringRef, const Module *, SrcMgr::CharacteristicKind) override;

        private:
            /// \brief Get the raw source string of the range.
            llvm::StringRef getSourceString(CharSourceRange Range);

            std::ofstream& Stream;
            ASTContext& Context;
            Preprocessor& PP;
            const Config& Configuration;
        };
    }
}
