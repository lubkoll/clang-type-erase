set(LLVM_LINK_COMPONENTS support)

add_subdirectory(tool)

install(FILES files/Storage.h files/SmartPointerStorage.h files/TypeErasureUtil.h DESTINATION etc)
