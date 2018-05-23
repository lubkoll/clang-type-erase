cmake_minimum_required(VERSION 3.1)
project(type_erasure_test)

set(CMAKE_CXX_STANDARD 14)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(.)

aux_source_directory(. SRC_LIST)
# inheritance-based type erasure
aux_source_directory(gen/basic SRC_LIST)
aux_source_directory(gen/cow SRC_LIST)
aux_source_directory(gen/sbo SRC_LIST)
aux_source_directory(gen/sbo_cow SRC_LIST)
# vtable-based type erasure
aux_source_directory(gen/vtable_basic SRC_LIST)
aux_source_directory(gen/vtable_cow SRC_LIST)
aux_source_directory(gen/vtable_sbo SRC_LIST)
aux_source_directory(gen/vtable_sbo_cow SRC_LIST)

aux_source_directory(gen/test SRC_LIST)

add_executable(unit_tests ${SRC_LIST})
target_link_libraries(unit_tests ${GTEST_LIBRARIES} pthread)

include(CTest)
enable_testing()
add_test(test ${PROJECT_BINARY_DIR}/Test/unit_tests)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
DEPENDS ${PROJECT_BINARY_DIR}/Test/unit_tests)
