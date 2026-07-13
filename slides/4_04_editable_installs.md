---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Scikit-build-core: editable installs

---

# If you don't know?

- `pip install -e .` or `uv sync`
- Edit python files
- Changes show up immediately (± `importlib.reload`)

---

# scikit-build-core caveat

- Edit C/C++/CMake source files
- Changes show up 🙂
  - Since 1.0: just call `<pkg>.__loader__.rebuild()` 😉

---

# Example: `pyproject.toml`
```toml
[build-system]
requires = ["scikit-build-core"]
build-backend = "scikit_build_core.build"

[project]
name = "example"
version = "0.1.0"

[tool.scikit-build]
build-dir = "build"
```

---

# Example: `python/example/__init__.py`
```python
from importlib.resources import files


def show_file():
    cmake_file = files("example") / "file.txt"
    print(cmake_file.read_text())
```

---

# Example: `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.15...4.3)
project(${SKBUILD_PROJECT_NAME} LANGUAGES NONE)

add_custom_target(copy_file ALL
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/source.txt
    ${CMAKE_CURRENT_BINARY_DIR}/file.txt
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/file.txt
  DESTINATION ${SKBUILD_PROJECT_NAME}
 )
```

---

# Example: It works!
```pycon
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

---

# Two `editable.mode`: `redirect`

- CMake build in predictable `build-dir` (if you include it 🙂)
- `_editable_skbc_{project}.(py|pth)` point `<pkg>.<module>` to `build-dir` or `source-dir`

---

# Actually forget about: `inplace`

- If you think you need it
  - Do you really need it?
- If you sure you need it
  - Have you exhausted all options?
- `inplace` is really what you want
  - Why?

---

# Why avoid `inplace`

- `build-dir` == `source-dir`
  - `CMakeCache.txt` etc.
  - We cannot help you delete build files
- You have to put build artifacts in correct place
- Since 1.0: try `edtiable.rebuild-dir` instead

---

# Tips and pitfalls
## Dependencies in `build-system.requires`

- CMake dependencies (`pybind11`) *DO NOT* persist
- Make sure you use `--no-build-isolation`
  ```console
  $ pip install --no-build-isolation -e .
  $ uv sync --no-build-isolation
  ```

---

# Tips and pitfalls
## Use `importlib.resources`

```pycon
>>> from pathlib import Path
>>> lib = Path(__file__) / "../my_lib.so"
>>> lib.open()
NotADirectoryError: [Errno 20] Not a directory: 'test.py/../my_lib.so'
```

`my_lib.so` is in `build-dir`, `test.py` is in `source-dir`

---

# Tips and pitfalls
## Use `importlib.resources`

```pycon
>>> from importlib.resources import files
>>> lib = files("my_pkg") / "my_lib.so"
>>> lib.open()
```

---

# Tips and pitfalls
## 1.0 changes

- `<pkg>.__loader__.rebuild()`: rebuild on demand
- `edtiable.rebuild-dir`: specify where to install
- `importlib.resources` should work properly (custom implementation)

---

# What to expect in 1.1?

- `editable.mode` without install
  - better support for debuggers and code coverage
