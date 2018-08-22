FROM ubuntu:16.04
MAINTAINER Lars Lubkoll "lars.lubkoll@posteo.de"

RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update && apt-get install -y cmake doxygen graphviz git rbenv gcc-6 g++-6 ninja-build python2.7 libboost-filesystem-dev clang-format-3.8 && \
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 100 --slave /usr/bin/g++ g++ /usr/bin/g++-6 && \
update-alternatives --set gcc /usr/bin/gcc-6
RUN mkdir /home/deps && mkdir /home/deps/googletest && git clone https://github.com/google/googletest.git /home/deps/googletest && cd /home/deps/googletest && mkdir build && cd build && cmake -GNinja .. && cmake --build . && cp -r /home/deps/googletest/googletest/include/gtest /usr/local/include/ && cp -r /home/deps/googletest/googlemock/include/gmock /usr/local/include/ && cp /home/deps/googletest/build/googlemock/lib*.a /usr/local/lib && cp /home/deps/googletest/build/googlemock/gtest/lib*.a /usr/local/lib
RUN mkdir /home/deps/clang-llvm && cd /home/deps/clang-llvm && git clone http://llvm.org/git/llvm.git && cd llvm/tools && git clone http://llvm.org/git/clang.git && cd clang/tools && git clone http://llvm.org/git/clang-tools-extra.git extra
WORKDIR /home
