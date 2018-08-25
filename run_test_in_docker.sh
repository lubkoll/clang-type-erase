#!/bin/bash

CLANG_TOOLS_EXTRA=/home/deps/clang-llvm/llvm/tools/clang/tools/extra
cd $CLANG_TOOLS_EXTRA
echo "add_subdirectory(clang-type-erase)" >> CMakeLists.txt
# uncomment when running locally and you don't want to share your repository
# git clone https://github.com/lubkoll/clang-type-erase

BUILD_DIR=/home/deps/build-llvm
mkdir -p $BUILD_DIR && cd $BUILD_DIR
ls -al /home/deps/clang-llvm/llvm
cmake -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_EH=ON ../clang-llvm/llvm
make -j4 clang-type-erase
cd tools/clang/tools/extra/clang-type-erase
make install

cd $CLANG_TOOLS_EXTRA/clang-type-erase/tests
./run $BUILD_DIR/bin/clang-type-erase
