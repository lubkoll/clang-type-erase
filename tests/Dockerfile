FROM ubuntu:16.04
MAINTAINER Lars Lubkoll "lars.lubkoll@posteo.de"

RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update && apt-get install -y cmake doxygen graphviz git rbenv gcc-6 g++-6 ninja-build
RUN mkdir /home/deps && mkdir /home/deps/googletest && git clone https://github.com/google/googletest.git /home/deps/googletest
RUN mkdir /home/deps/clang-llvm && cd /home/deps/clang-llvm && git clone http://llvm.org/git/llvm.git && cd llvm/tools && git clone http://llvm.org/git/clang.git && cd clang/tools && git clone http://llvm.org/git/clang-tools-extra.git extra

WORKDIR /home
