set(LLVM_LINK_COMPONENTS support)

set(CMAKE_CXX_FLAGS "$CMAKE_CXX_FLAGS -fexceptions")
add_clang_library(clangTypeEraseUtils
  Config.cpp
  )
