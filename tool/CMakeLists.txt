set(LLVM_LINK_COMPONENTS
  support
  )

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "$CMAKE_CXX_FLAGS -fexceptions")
add_clang_executable(clang-type-erase
  ClangTypeEraseMain.cpp
  TableWriter.cpp
  InterfaceWriter.cpp
  SimpleInterfaceWriter.cpp
  TypeErasureWriter.cpp
  PreprocessorCallback.cpp
  Utils.cpp
  )
add_dependencies(clang-type-erase
  clang-headers
  )

include_directories(/home/lars/Libraries/build-boost-clang/include)
link_directories(/home/lars/Libraries/build-boost-clang/lib)

target_link_libraries(clang-type-erase PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangTypeEraseUtils
  clangTooling
  boost_filesystem
  boost_system
  )

install(TARGETS clang-type-erase
  RUNTIME DESTINATION bin)
