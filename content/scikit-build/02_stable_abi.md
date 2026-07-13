# Stable ABI

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/4_02_stable_abi>`

## Pure wheels

If you've built a pure Python project, you've probably seen wheel names that looked like this:

```text
scikit_build_core-1.0.3-py3-none-any.whl
|_______________| |___| |_| |__| |_|
         |          |    |    |    \
       name      version |    |  platform
                      python  |
                             abi
```

That is a `-` separated set of components:

* The normalized name (`.`/`-` converted to `_`, otherwise it wouldn't parse correctly)
* The version
* A tag for the version of Python it's compatible with (the language)
* A tag for the ABI it's compatible with (CPython)
* The platform it targets

There's also an optional build number, not shown here (pretty rare in practice).

:::{note}

The variant project adds a new section, which is a hash that contains other information, like CUDA versions.

:::

Those work on any version of Python; you can load it in Pyodide on the web, in PyPy, etc. However, you can't put built components in those, only Python.

In scikit-build-core, you use:

```toml
[tool.scikit-build]
wheel.py-api = "py3"
wheel.cmake = false
```

to indicate that this is a pure wheel.

## Python-less wheels

The next step up is wheels without Python ABI, _just_ compiled bits and pure Python. This could be something like `cmake` or `ruff`, which have little interaction with Python, or it could be loading a `.so` file from Python. Either way, this is what the tag looks like:

```text
clang_format-22.1.8-py2.py3-none-macosx_11_0_arm64.whl
|__________| |____| |_____| |__| |_______________|
     |          |      |     |          |
    name   version     |     |      platform
                    python   |
                            abi
```

You have one wheel per supported platform.

In scikit-build-core, you use:

```toml
[tool.scikit-build]
wheel.py-api = "py3"
```

to indicate that this has no ABI dependence.

## Standard wheels

If you use the normal ABI, you have to have one wheel per
supported Python, per platform:

```text
pyzmq-27.1.0-cp314-cp314t-macosx_10_15_universal2.whl
|___| |____| |___| |____| |_____________________|
  |      |     |     |              |
 name  version |     |           platform
            python   |
                    abi
```

No special settings are needed for this most common case.

## Limited API / Stable ABI

If you restrict yourself to a limited subset of the API, you can use the
"Stable ABI", which doesn't change between Python versions. This is what
the wheel looks like:

```text
onnx-1.22.0-cp312-abi3-macosx_12_0_universal2.whl
|__| |____| |___| |__| |_____________________|
  |     |     |     |             |
 name version |     |          platform
            python  |
                   abi
```

And in scikit-build-core:

```toml
[tool.scikit-build]
wheel.py-api = "cp312"
```

where the number above is the minimum version of the stable ABI you support. Before that value,
scikit-build-core will build traditional wheels.

There are several things you have to do, though. In CMake, you need to find the Stable ABI library component:

```cmake
find_package(Python REQUIRED COMPONENTS Interpreter Development.Module ${SKBUILD_SABI_COMPONENT})
```

That variable expands to `Development.SABIModule` if you are building for the
Stable ABI, and nothing if you are not (such as on PyPy, 3.14t, etc).

Then you need to add it:

```cmake
if(NOT "${SKBUILD_SABI_VERSION}" STREQUAL "")
  set(USE_SABI "USE_SABI ${SKBUILD_SABI_VERSION}")
endif()

python_add_library(some_ext MODULE WITH_SOABI ${USE_SABI} ...)
```

When `USE_SABI 3.12` is passed, CMake will define `Py_LIMITED_API` to match when compiling.

## Free threaded stable ABI

Free-threaded Python requires a new ABI, so the classic limited ABI will not
work. A new ABI was created. While you can build this from threaded Python,
it's strongly recommended to build it with the free-threading interpreter (it
generally works on both).

Unlike the other examples, this isn't available yet, since 3.15 has not shipped a RC yet at the
time of writing, so instead of an example, here are the two common variations:

* `cp315-abi3t`: Free-threaded only
* `cp315-abi3.abi3t`: Both

In scikit-build-core, it looks like this:

```toml
[tool.scikit-build]
wheel.py-api = "cp315.cp315t"
```

And, something like this for older CMake:

```cmake
find_package(
  Python
  COMPONENTS Interpreter Development.SABIModule
  REQUIRED)

# Use the Stable ABI version and SOABI suffix scikit-build-core resolved (3.15 /
# abi3t for a free-threaded build, or abi3 for a GIL build).
if(SKBUILD_SOABI)
  set(Python_SOABI ${SKBUILD_SOABI})
endif()

python_add_library(_core MODULE src/${SKBUILD_PROJECT_NAME}/_core.c WITH_SOABI
                   USE_SABI ${SKBUILD_SABI_VERSION})

# CMake < 4.4 has no abi3t awareness, so pass the target define ourselves.
# scikit-build-core requests abi3t via this cache variable.
if(Py_TARGET_ABI3T)
  target_compile_definitions(_core PRIVATE Py_TARGET_ABI3T=0x030f0000)
endif()
```

(If you want the 'only free-threaded' form, use overrides.)
