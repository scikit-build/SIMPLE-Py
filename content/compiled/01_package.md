# A minimal compiled package with scikit-build

You've seen how to build a package already. Let's try a compiled package now!

## First simple package

First, you'll need a file to compile. C++, C, Fortran, Rust, etc. You also need
to pick a binding tool; if you are in C, you can use the C-API, but don't --
it's verbose, and you'll have to manage reference counting; binding tools
handle this for you.

Let's use a trivial pybind11 extension. Name the source file `collatz.cpp`:

```c++
#include <pybind11/pybind11.h>

// Even -> n/2, odd -> 3n+1. Conjectured to always terminate.
int collatz_steps(long long n) {
  int steps = 0;
  while (n != 1) {
    n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    ++steps;
  }
  return steps;
}

PYBIND11_MODULE(collatz, m) {
  m.def("collatz_steps", &collatz_steps, "Steps to reach 1 in the Collatz sequence");
}
```

It's a tight integer loop, something that's fast in a compiled language.
Hopefully you noticed that we set a module name, `collatz`, in
`PYBIND11_MODULE`. Every CPython module needs to know the name it will be
compiled to, since that's also how you look up the main entry point into the
code.

Now, you need a `pyproject.toml`. We use `scikit-build-core` as the build
backend, and list `pybind11` so CMake can find it while building:

```toml
[build-system]
requires = ["scikit-build-core", "pybind11"]
build-backend = "scikit_build_core.build"

[project]
name = "collatz"
version = "0.1.0"
```

And, a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.15...4.3)
project(collatz LANGUAGES CXX)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(collatz collatz.cpp)
install(TARGETS collatz DESTINATION .)
```

And that should be it! Try it:


```console
$ uv run python
>>> from collatz import collatz_steps
>>> collatz_steps(27)
111
```

## Make the package better

You don't have to do add anything more, but there are config settings that
really help, let's look at a few.

### Minimium-version


