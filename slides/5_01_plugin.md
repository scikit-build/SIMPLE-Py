---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Scikit-build-core as a plugin

---

# Why plugins?

- A build backend **owns** the build: it produces the SDist and wheel,
  and decides which hooks get to contribute files along the way
- Scikit-build-core can be one of those hooks: it runs CMake and drops
  the extension modules into another backend's wheel
- Two plugins are built in: **hatchling** and **setuptools**
- Same CMake machinery, same `[tool.scikit-build]` configuration as the
  standalone backend

---

# When do you want this?

- **The project is mostly Python** — keep your existing
  hatchling/setuptools setup and add compiled bits only where needed
- **You need to combine build steps** — e.g. a hatchling hook for mypyc
  alongside the scikit-build hook for C++
- **You're migrating** — setuptools projects (including scikit-build
  classic) can adopt CMake without rewriting their packaging

Starting fresh with a primarily compiled extension? The standalone
`scikit_build_core.build` backend is still the simplest path.

---

# Hatchling

```toml
[build-system]
requires = ["scikit-build-core[hatchling]"]
build-backend = "hatchling.build"

[project]
name = "hatchling_example"
version = "0.1.0"

[tool.hatch.build.targets.wheel.hooks.scikit-build]
```

- The (empty) `hooks.scikit-build` table switches the plugin on
- Always use the `[hatchling]` extra — it pins a compatible version and
  protects you if the plugin ever moves to a separate package

---

# Hatchling: configuration

- Configure in `tool.hatch.build.targets.wheel.hooks.scikit-build`, the
  usual `tool.scikit-build` table, or environment variables
- Hatchling owns its part of the build, so a few features don't apply:
  * `wheel.cmake` must stay on — no pure-Python (`platlib = false`) build
  * Building CMake during the SDist step isn't supported
  * `generate` / `metadata` are handled by hatchling's own plugins
  * No config-settings (`-C...`) — hatchling doesn't pass them to plugins
- Inside `CMakeLists.txt`, `${SKBUILD_HATCHLING}` holds the hatchling
  version

---

# Hatchling: a full example

<div class="columns">
<div>

```text
hatchling
├── pyproject.toml
├── cpp
│   ├── CMakeLists.txt
│   └── example.c
└── src
    └── hatchling_example
        └── __init__.py
```

</div>
<div>

```toml
[tool.hatch.build.targets.wheel.hooks.scikit-build]
cmake.source-dir = "cpp"
wheel.install-dir = "hatchling_example"
```

- `cmake.source-dir` points at the
  `CMakeLists.txt`
- `wheel.install-dir` sends the CMake
  output into the package, so `_core`
  lands next to `__init__.py`

</div>
</div>

---

# The CMake side

```cmake
cmake_minimum_required(VERSION 3.15...4.3)
project(${SKBUILD_PROJECT_NAME} LANGUAGES C)

find_package(
  Python
  COMPONENTS Interpreter Development.Module
  REQUIRED)

python_add_library(_core MODULE example.c WITH_SOABI)
install(TARGETS _core DESTINATION .)
```

Exactly what you'd write for the standalone backend.

---

# Setuptools

```toml
[build-system]
requires = ["scikit-build-core[setuptools]"]
build-backend = "scikit_build_core.setuptools.build_meta"
```

- Keeping `setuptools.build_meta` as the backend also works, but
  `scikit_build_core.setuptools.build_meta` adds auto-inclusion of
  `cmake` and `ninja` plus config-settings — recommended if you can

---

# Setuptools: activation

The plugin only activates with a `cmake.source-dir` setting (or the
classic `cmake_source_dir` keyword in a minimal `setup.py`):

<div class="columns">
<div>

```toml
# pyproject.toml (recommended)
[tool.scikit-build]
cmake.source-dir = "."
```

</div>
<div>

```python
# setup.py
from setuptools import setup

setup(cmake_source_dir=".")
```

</div>
</div>

Everything else configures through `[tool.scikit-build]` as normal.

---

# Setuptools: caveats

- Setuptools owns the SDist and editable machinery:
  * The SDist file list comes from setuptools — only
    `sdist.inclusion-mode = "explicit"` is supported
  * Editable installs (`pip install -e .`) require
    `editable.mode = "inplace"`, which writes build artifacts into
    your source tree
- Config-settings (`-C cmake.build-type=Debug`) work through the
  PEP 517 backend, not `setuptools.build_meta` or a direct `setup.py`
- Classic keywords (`cmake_args`, `cmake_install_dir`,
  `cmake_install_target`, …) are still honored for compatibility

---

# Which one?

| You have | Use |
|---|---|
| A mostly-Python hatchling project | hatchling plugin |
| An existing setuptools / scikit-build classic project | setuptools plugin |
| Primarily a compiled extension, starting fresh | standalone `scikit_build_core.build` |
