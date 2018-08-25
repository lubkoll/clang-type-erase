set(LLVM_LINK_COMPONENTS support)
set(CMAKE_CXX_STANDARD 14)
add_definitions(-DCLANG_TYPE_ERASE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

add_subdirectory(tool)
add_subdirectory(utils)

install(FILES files/Storage.h files/SmartPointerStorage.h files/type_erasure_util.h DESTINATION etc)
