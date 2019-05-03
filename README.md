[![Build Status](https://travis-ci.org/lubkoll/clang-type-erase.svg?branch=master)](https://travis-ci.org/lubkoll/clang-type-erase)

# clang-type-erase

Parses a header file with one or multiple interface definitions and creates the corresponding type-erased interfaces. 



Options:
* Choose between the straight-forward implementation of type-erased interfaces based on built-in dynamical polymorphism or an optimized implementation that is based on custom function tables.
* copy-on-write
* small buffer optimization
* non-copyable interfaces
* no RTTI



**clang-type-erase** is based on Clang's [LibTooling](https://clang.llvm.org/docs/LibTooling.html). To compile it do:
* [obtain Clang](https://clang.llvm.org/docs/LibASTMatchersTutorial.html)
* download/clone clang-type-erase and place the folder 'clang-type-erase' into \<path-to-llvm\>/tools/clang/tools/extra
* add 'add_subdirectory(clang-type-erase)' to \<path-to-llvm\>/tools/clang/tools/extra/CMakeLists.txt
* (re-)compile (see [here](https://clang.llvm.org/docs/LibASTMatchersTutorial.html))

