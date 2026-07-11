---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## A minimal compiled package

---

## A compiled package is three files

You already built a pure-Python package. A compiled one adds a compiler and a
build backend, but the shape is the same:

<div class="columns">
<div>

- **source** — `collatz.cpp` (the code to compile)
- **`pyproject.toml`** — declares the build backend
- **`CMakeLists.txt`** — tells CMake how to build

</div>
<div>

We use:

- **pybind11** to bind C++ ↔ Python
- **scikit-build-core** as the build backend
- **CMake** as the build system

</div>
</div>

The packaging is the same whichever binding tool you pick.

---

## The source

A trivial pybind11 extension — a tight integer loop, fast in a compiled language:

```c++
#include <pybind11/pybind11.h>

int collatz_steps(long long n) {
  int steps = 0;
  while (n != 1) {
    n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    ++steps;
  }
  return steps;
}

PYBIND11_MODULE(collatz, m) {  // this name IS the importable module
  m.def("collatz_steps", &collatz_steps);
}
```

---

## The build backend

`pyproject.toml` picks scikit-build-core and lists `pybind11` so CMake can find it:

```toml
[build-system]
requires = ["scikit-build-core", "pybind11"]
build-backend = "scikit_build_core.build"

[project]
name = "collatz"
version = "0.1.0"
```

`CMakeLists.txt` does the actual build:

```cmake
cmake_minimum_required(VERSION 3.15...4.3)
project(collatz LANGUAGES CXX)

find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(collatz collatz.cpp)
install(TARGETS collatz DESTINATION .)
```

---

## And it just works

No manual compile step — `uv run` sees the `pyproject.toml`, builds the
extension, and installs it into a temporary environment:

```console
$ uv run python
>>> from collatz import collatz_steps
>>> collatz_steps(27)
111
```

(It climbs to `9232` on the way to `1` — 111 steps.)

---

## Was it worth it?

The same loop in pure Python, then `timeit` both:

```console
$ python -m timeit -s "from collatz import collatz_steps"  "collatz_steps(97)"
1000000 loops, best of 5: 210 nsec per loop

$ python -m timeit -s "from mymodule import collatz_steps" "collatz_steps(97)"
100000 loops, best of 5: 5.8 usec per loop
```

**~20–30× faster.** That gap is the whole reason to compile — the hot loop
lives in C++, the packaging that runs once stays in Python.

The win shrinks for trivial inputs: with no work inside the call, you're just
timing the Python↔C boundary.

---

## Minimum version = good defaults, safely

`cmake_minimum_required` isn't just an error check — it selects CMake
**policies** (versioned defaults), so old projects don't break on new CMake:

```cmake
cmake_minimum_required(VERSION 3.15...4.3)  # floats up to 4.3
```

scikit-build-core has the same idea. Set it once, reuse it:

```toml
[build-system]
requires = ["scikit-build-core>=1.0"]

[tool.scikit-build]
minimum-version = "build-system.requires"  # reads the pin above
```

Raise it (or leave unset) to opt into the latest recommendations all at once.

---

## src layout

Especially important for compiled code — you can't run the *uncompiled* version,
so don't let Python pick up the source dir:

```text
example
├── pyproject.toml
├── CMakeLists.txt
└── src
    └── collatz
        ├── __init__.py
        └── _core.cpp   # builds as collatz._core
```

Auto-discovered when the package name matches the project name (like hatchling).
`__init__.py` re-exports from `._core` so `from collatz import collatz_steps`
still works.

---

## Three names that must line up

The #1 beginner error — a mismatch gives a runtime `ImportError`, not a build
failure:

1. `PYBIND11_MODULE(_core, m)` — what the compiled `.so`/`.pyd` is called
2. `install(TARGETS _core DESTINATION collatz)` — where it lands in the wheel
3. `from collatz._core import ...` — what you import

Rename the module but forget the `__init__.py` re-export → the build succeeds
and the import breaks. When something imports oddly, check these three first.

---

## Two distributions

<div class="columns">
<div>

### SDist

- What you build **from**
- Source, `CMakeLists.txt`, tests
- Baseline = everything not `.gitignore`d

```bash
uv build --sdist
tar -tf dist/*.tar.gz
```

</div>
<div>

### wheel

- What you **install**
- Package + compiled extension only
- Platform- and Python-tagged filename

```bash
uv build --wheel
unzip -l dist/*.whl
```

</div>
</div>

`uv build` makes each **independently** from source; pass the archive
(`uv build dist/collatz-0.1.0.tar.gz`) to build a wheel *from* the SDist.

---

## Iterating

For real development, install editable — scikit-build-core can even rebuild the
extension on import:

```bash
uv pip install --no-build-isolation -e .
```

When a build goes wrong, turn on the details:

```toml
[tool.scikit-build]
build.verbose = true
cmake.build-type = "Debug"
```

```bash
uv build --wheel -C cmake.define.CMAKE_CXX_STANDARD=20
```

---

## Summary

- A compiled package = **source + `pyproject.toml` + `CMakeLists.txt`**;
  `uv run` builds it for you
- Compiled pays off when there's **real work inside the call** (~20–30× here)
- `minimum-version` gives new users good defaults without breaking old builds
- Use **src layout**, and keep the **three names** aligned
- **SDist** is what you build from; the **wheel** is what you install
- Develop with an **editable** install; debug with `build.verbose`
