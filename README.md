[![Build Status](https://travis-ci.org/lubkoll/clang-type-erase.svg?branch=master)](https://travis-ci.org/lubkoll/clang-type-erase)

# clang-type-erase

clang-type-erase generates type-erased interfaces for you. It parses a header file with one or multiple interface definitions and creates the corresponding type-erased interface. 

You can choose between two different implementations:
* the straight-forward implementation of type-erased interfaces based on built-in dynamical polymorphism and
* an implementation that is based on custom function tables.

Each approach provides
* a standard implementation
* an implementation providing copy-on-write
* an implementation providing small buffer optimization
* an implementation providing copy-on-write and small buffer optimization

Each of these implementation can additionally be customized to
* work with non-copyable types (which makes the interfaces itself non-copyable)
* work with RTTI disabled

clang-type-erase is based on Clang's [LibTooling](https://clang.llvm.org/docs/LibTooling.html). To compile it do:
* [obtain Clang](https://clang.llvm.org/docs/LibASTMatchersTutorial.html)
* download/clone clang-type-erase and place the folder 'clang-type-erase' into \<path-to-llvm\>/tools/clang/tools/extra
* add 'add_subdirectory(clang-type-erase)' to \<path-to-llvm\>/tools/clang/tools/extra/CMakeLists.txt
* (re-)compile (see [here](https://clang.llvm.org/docs/LibASTMatchersTutorial.html))

