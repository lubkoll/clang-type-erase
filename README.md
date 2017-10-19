# clang-type-erase

clang-type-erase is based on Clang's [LibTooling](https://clang.llvm.org/docs/LibTooling.html). To compile it do:
* [obtain Clang](https://clang.llvm.org/docs/LibASTMatchersTutorial.html) (Step 0). 
* download/clone clang-type-erase and place the folder 'clang-type-erase' into \<path-to-llvm\>/tools/clang/tools/extra
* add 'add_subdirectory(clang-type-erase)' to \<path-to-llvm\>/tools/clang/tools/extra/CMakeLists.txt
* (re-)compile (see [here](https://clang.llvm.org/docs/LibASTMatchersTutorial.html))
