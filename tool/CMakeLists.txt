set(CMAKE_CXX_STANDARD 14)

add_definitions(-DCLANG_TYPE_ERASE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

add_clang_executable(clang-type-erase
  ClangTypeEraseMain.cpp
  Config.h
  Config.cpp
  TableWriter.h
  TableWriter.cpp
  InterfaceWriter.h
  InterfaceWriter.cpp
  TypeErasureWriter.h
  TypeErasureWriter.cpp
  PreprocessorCallback.h
  PreprocessorCallback.cpp
  Utils.h
  Utils.cpp
)
add_dependencies(clang-type-erase
  clang-headers
)

target_link_libraries(clang-type-erase PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangTooling
  boost_filesystem
  boost_system
)

install(TARGETS clang-type-erase RUNTIME DESTINATION bin)
