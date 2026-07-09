---
authors: [Cristian Le]
---

# Editable installs

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/4_04_editable_installs>`

## What it means for Python

You edit the project's Python files and the changes are reflected "immediately" in the python interpreter.
All you need is to `pip install -e` or `uv sync` the project/workspace.

## What it means for scikit-build-core

On top of the typical pure-python features, scikit-build-core editable installs also rebuild the CMake project.
Technically `cmake --build` and `cmake --install`.

## Demystifying `editable.mode`

Currently, there are 2 editable modes `redirect` (default), `inplace` (use it at your own risk).

### `redirect` mode

Creates a `_editable_skbc_{}.(py|pth)` file with instructions where to find Python files and CMake build (and installed) artifacts.
The CMake build artifacts _are installed_ into site-packages path after each rebuild.

### `inplace` mode

Effectively builds the CMake project with source-dir (`-S`) the same as build-dir (`-B`), which is a big no-no in CMake design.

:::{warning}
This mode requires _you_ to setup the CMake build to put the build artifacts in the correct place relative to the source.
::::

## Scikit-build-core 1.0 tricks and tips

Use can now rebuild the CMake project on demand with

```{code} python
:linenos:
:emphasize-lines: 2

>>> import foo
>>> foo.__loader__.rebuild()
```

## Scikit-build-core pitfalls

### Dependencies in `build-system.requires`

Build dependencies that provide CMake files (e.g. `pybind11`) need to be persistent in order to be able to rebuild.
With `--no-build-isolation` the build dependencies are taken from local `site-packages`

```console
pip install --no-build-isolation -e .
uv sync --no-build-isolation
```

### Use `importlib.resources`

Inherently `redirect` editable mode behaves like a namespaced package, i.e. sources are in multiple locations.
You cannot rely on patterns like `__file__` to navigate to

```{code} python
>>> from pathlib import Path
>>> lib = Path(__file__) / "../my_lib.so"
>>> lib.open()
NotADirectoryError: [Errno 20] Not a directory: 'test.py/../my_lib.so'
```

Instead use `importlib.resources.files`

```{code} python
:linenos:
:emphasize-lines: 2

>>> from importlib.resources import files
>>> lib = files("my_pkg") / "my_lib.so"
>>> lib.open()
```

### Don't forget the `build-dir`

In order to be able to rebuild the CMake project you need a persistent build directory defined in `build-dir`.
By default `redirect` would not allow you to rebuild the CMake project and only cover the pure python files loading.
Providing a persistent `build-dir` allows to manually `rebuild()` and with the `editable.rebuild` it will do so automatically whenever you `import`

## Example

```{code} toml
:filename: pyproject.toml

[build-system]
requires = ["scikit-build-core"]
build-backend = "scikit_build_core.build"

[project]
name = "example"
version = "0.1.0"

[tool.scikit-build]
build-dir = "build"
```

```{code} toml
:filename: python/example/__init__.py

from importlib.resources import files

def show_file():
    cmake_file = files("example") / "file.txt"
    print(cmake_file.read_text())
```

```{code} toml
:filename: CMakeLists.txt

cmake_minimum_required(VERSION 3.15...4.3)
project(${SKBUILD_PROJECT_NAME} LANGUAGES NONE)

add_custom_target(copy_file ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/source.txt ${CMAKE_CURRENT_BINARY_DIR}/file.txt
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/file.txt DESTINATION ${SKBUILD_PROJECT_NAME})
```

```{code} python
>>> import example
>>> example.show_file()
Hello, world!
>>> example.__loader__.rebuild()
Running cmake --build & --install in /home/cle/tmp/simple-py/build
[1/1] cd build && /usr/bin/cmake -E copy source.txt build/file.txt
-- Install configuration: "Release"
-- Installing: .venv/lib64/python3.14/site-packages/example/file.txt
>>> example.show_file()
Off to the races!
```
