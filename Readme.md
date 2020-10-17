# kdab-supercede-stdpow-checker

This is a clang/libTooling based refactoring tool meant to serve as an example of
project specific, custom taylored tooling.

It finds usages of `std::pow` with static integer exponents and replaces them by
calls to a templated `utils::pow<>` that is expected to exist inside the user code.

Note: `std::pow` usually involves a function call, which can be a bottle-neck in
very performance critical code paths. With an inline, templated power function
made for integer potentation, this overhead can be reduced to `O(log(exponent))`
multiplications. On most common platforms, though, compilers support the flag
`-ffast-math`, which among other optimizations applies this same optimization to
`std::pow` directly.

### Build and installation

In order to find all clang system headers properly, this tool needs to be installed
to the same location, as clang is. The install location is specified as a command
line argument to cmake with `-DCMAKE_INSTALL_PREFIX=/path/`.

```
cd path/to/kdab-supercede-stdpow-checker
mkdir build && cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=/usr ..
ninja
sudo ninja install
```

or, if you prefer to use `make`

```
cd path/to/kdab-supercede-stdpow-checker
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j
sudo make install
```


### Usage

The easiest way to use this tool is with a compilation database. If you use
`cmake` for configuring your project, you can create one by passing
`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to cmake as command line argument. Then,
you can analyze a source file using

```
kdab-supercede-stdpow-checker -p /path/to/build/dir /path/to/sourcefile.cpp
```

Alternatively or additionally, you may pass compiler arguments and flags to
`kdab-supercede-stdpow-checker` after a double dash.

```
kdab-supercede-stdpow-checker /path/to/sourcefile.cpp -- -std=c++17 -I/usr/include/qt/QtCore
```

For additional information on how to create libtooling based custom tooling, see
https://clang.llvm.org/docs/LibTooling.html
https://clang.llvm.org/docs/LibASTMatchersReference.html
https://clang.llvm.org/docs/InternalsManual.html
