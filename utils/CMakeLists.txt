set(LLVM_LINK_COMPONENTS support)

add_definitions("-fexceptions")
add_clang_library(clangTypeEraseUtils
  Config.cpp
  )
