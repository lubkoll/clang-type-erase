[![Build Status](https://travis-ci.org/lubkoll/clang-type-erase.svg?branch=master)](https://travis-ci.org/lubkoll/clang-type-erase)

# clang-type-erase

clang-type-erase generates type-erased interfaces for you.

You can choose between two different implementations:
* the straight-forward implementation of type-erased interfaces based on built-in dynamical polymorphism and
* an implementation that is based on custom function tables.

Each approach supports for modes
* a standard implementation
* copy-on-write
* small buffer optimization
* copy-on-write and small buffer optimization

clang-type-erase is based on Clang's [LibTooling](https://clang.llvm.org/docs/LibTooling.html). To compile it do:
* [obtain Clang](https://clang.llvm.org/docs/LibASTMatchersTutorial.html)
* download/clone clang-type-erase and place the folder 'clang-type-erase' into \<path-to-llvm\>/tools/clang/tools/extra
* add 'add_subdirectory(clang-type-erase)' to \<path-to-llvm\>/tools/clang/tools/extra/CMakeLists.txt
* (re-)compile (see [here](https://clang.llvm.org/docs/LibASTMatchersTutorial.html))

