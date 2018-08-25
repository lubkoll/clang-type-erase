set(LLVM_LINK_COMPONENTS support)
set(CMAKE_CXX_STANDARD 14)
add_compile_definitions(CLANG_TYPE_ERASE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}

#add_definitions(-std=c++1z)
add_subdirectory(tool)
add_subdirectory(utils)


install(FILES files/Storage.h files/SmartPointerStorage.h files/type_erasure_util DESTINATION etc)
