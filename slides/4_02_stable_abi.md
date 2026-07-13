---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Scikit-build-core: The Stable ABI

---

## Anatomy of a wheel name

```text
scikit_build_core-1.0.3-py3-none-any.whl
|_______________| |___| |_| |__| |_|
         |          |    |    |    \
       name      version |    |  platform
                      python  |
                             abi
```

- Normalized name (`.`/`-` converted to `_`), then version
- **python** — the language version it supports
- **abi** — the CPython ABI it links against
- **platform** — the OS/architecture it targets
- The python/abi pair decides **how many wheels you ship**

---

## Pure wheels: `py3-none-any`

One wheel for every Python, every platform — CPython, PyPy, Pyodide on the
web...

The catch: pure Python only, no compiled bits.

```toml
[tool.scikit-build]
wheel.py-api = "py3"
wheel.cmake = false
```

---

## Python-less wheels: `py3-none-<platform>`

```text
clang_format-22.1.8-py2.py3-none-macosx_11_0_arm64.whl
```

Compiled bits that don't touch the Python ABI — `cmake`, `ruff`, or a `.so`
you load yourself. One wheel per platform, any Python:

```toml
[tool.scikit-build]
wheel.py-api = "py3"
```

---

## Standard wheels: `cp314-cp314`

```text
pyzmq-27.1.0-cp314-cp314t-macosx_10_15_universal2.whl
```

The full CPython API means one wheel per Python **per platform**.

- No special settings — this is the default
- 5+ Pythons × many platforms = a big build matrix

---

## Limited API → Stable ABI: `cp312-abi3`

Restrict yourself to the limited subset of the API, and the ABI stops
changing between Python versions:

```text
onnx-1.22.0-cp312-abi3-macosx_12_0_universal2.whl
```

One wheel per platform, runs on 3.12 and up:

```toml
[tool.scikit-build]
wheel.py-api = "cp312"
```

The number is the **minimum** Stable ABI version; below it (and on PyPy)
scikit-build-core builds traditional wheels instead.

---

## The CMake side

Find the Stable ABI library component:

```cmake
find_package(Python REQUIRED COMPONENTS
             Interpreter Development.Module ${SKBUILD_SABI_COMPONENT})
```

`SKBUILD_SABI_COMPONENT` expands to `Development.SABIModule` when targeting
the Stable ABI, and to nothing when not (PyPy, 3.14t, ...).

```cmake
if(NOT "${SKBUILD_SABI_VERSION}" STREQUAL "")
  set(USE_SABI "USE_SABI ${SKBUILD_SABI_VERSION}")
endif()

python_add_library(some_ext MODULE WITH_SOABI ${USE_SABI} ...)
```

With `USE_SABI 3.12`, CMake defines `Py_LIMITED_API` to match.

---

## Free-threaded Stable ABI (3.15)

Free-threading needs a **new** stable ABI — classic `abi3` won't work. Not
out yet (no 3.15 RC at time of writing), but the tags will be:

- `cp315-abi3t` — free-threaded only
- `cp315-abi3.abi3t` — both

```toml
[tool.scikit-build]
wheel.py-api = "cp315.cp315t"
```

Build with the free-threaded interpreter — the result generally runs on
both.

---

## ... on older CMake

```cmake
find_package(Python REQUIRED COMPONENTS Interpreter Development.SABIModule)

# abi3t or abi3, resolved by scikit-build-core
if(SKBUILD_SOABI)
  set(Python_SOABI ${SKBUILD_SOABI})
endif()

python_add_library(_core MODULE src/mypackage/_core.c WITH_SOABI
                   USE_SABI ${SKBUILD_SABI_VERSION})

# CMake < 4.4 has no abi3t awareness; define the target ABI ourselves
if(Py_TARGET_ABI3T)
  target_compile_definitions(_core PRIVATE Py_TARGET_ABI3T=0x030f0000)
endif()
```

Want the free-threaded-only form? Use overrides.

---

## Summary

- The **python/abi tags** in the wheel name set your build matrix
- `py3-none-any` — pure Python; `py3-none-<plat>` — compiled, no Python ABI
- Default: one wheel per Python per platform
- `wheel.py-api = "cp312"` — Stable ABI, one wheel per platform from 3.12 up
- CMake: `${SKBUILD_SABI_COMPONENT}` + `USE_SABI ${SKBUILD_SABI_VERSION}`
- 3.15 adds a free-threaded stable ABI: `cp315.cp315t` → `abi3.abi3t`
