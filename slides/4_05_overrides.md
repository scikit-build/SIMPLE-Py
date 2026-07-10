---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Scikit-build-core: The override system

---

# An example says it all

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true
```
- If `WITH_MPI` is not defined, it provides a `-DBUILD_MPI=OFF`
- If `WITH_MPI` is `ON`, it provides a `-DBUILD_MPI=ON`

---

# Example scenarios

- Specifying different CMake defines in the CI
- Providing platform specific default settings
- Change the build flags based on Python and/or CMake version
- Fail early on unsupported environments
- Download CMake dependencies when building from sdist, but not when building locally
- Adjust SPDX license if using bundled dependencies

---

# `if` conditional

- Left side: what to test against e.g. `env`, `python-version`
- Right side: regex pattern, version specifier (`>=4.0`), bool
- Full list in <https://scikit-build-core.readthedocs.io/en/latest/configuration/overrides.html>

---

# Things to consider
## Order of tables is important

<div class="columns">
<div>

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true

[[tool.scikit-build.overrides]]
if.platform-system = "win32"
cmake.define.BUILD_MPI = false
```
Always `-DBUILD_MPI=OFF` on windows

</div>
<div>

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false

[[tool.scikit-build.overrides]]
if.platform-system = "win32"
cmake.define.BUILD_MPI = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true
```
Windows override is a no-op

</div>
</div>

---

# Things to consider
## `inherit` and tables

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false
cmake.define.BUILD_TESTS = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```
- `-DBUILD_TESTS` is not defined
- `-DBUILD_MPI=ON`
- `-DMPI_PROC=2`

---

# Things to consider
## `inherit` and tables

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false
cmake.define.BUILD_TESTS = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
inherit.cmake.define = "append"
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```
- `-DBUILD_TESTS=OFF`
- `-DBUILD_MPI=ON`
- `-DMPI_PROC=2`

---

# Things to consider
## `inherit` and tables

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false
cmake.define.BUILD_TESTS = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
inherit.cmake.define = "prepend"
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```
- `-DBUILD_TESTS=OFF`
- `-DBUILD_MPI=OFF`
- `-DMPI_PROC=2`
