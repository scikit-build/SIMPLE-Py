---
authors: [Cristian Le]
---

# The override system

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/4_05_overrides>`

## The basics

This allows you to _override_ the settings in `[tool.scikit-build]` based on the environment that is building the package.

A simple example is

```toml
[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true
```

which checks for an environment variable `WITH_MPI` and then populates the `-DBUILD_MPI` CMake define.

The latest documentation of this feature can be found at the [Configuration/Overrides] page

[Configuration/Overrides]: https://scikit-build-core.readthedocs.io/en/latest/configuration/overrides.html

### Example situations

Here are some scenarios of where and how you could use this to get you inspired:

- Specifying different CMake defines in the CI
- Providing platform specific default settings
- Change the build flags based on Python and/or CMake version
- Fail early on unsupported environments
- Download CMake dependencies when building from sdist, but not when building locally
- Adjust SPDX license if using bundled dependencies

## How it works

All `if.*` conditionals are `and` glued.
If you need `or` glued conditionals instead, use `if.any.*` as needed.
The current `if.*` conditionals available are:

- `scikit-build-version` (version): scikit-build-core version currently used
- `python-version` (version): the builder's python version
- `system-cmake` (version): if any suitable CMake is available on the system
- `implementation-version` (version): `sys.implementation.name`
- `platform-system`, `platform-machine`, `platform-node`, `implementation-name` (regex):
  equivalent `sys.*` variable
- `abi-flags` (regex): abi flags such as `t` for free-threading
- `state` (regex): current build state, one of `sdist`, `wheel`, `editable`, `metadata_wheel`, `metadata_editable`
- `env.*` (regex or bool): environment variables
- `from-sdist` (bool): whether the build comes from an sdist source
- `wheel.cmake` (bool): if there are known CMake wheels available for the system
- `failed` (bool): whether a build has failed, used to try again with other options

where the `regex`, `version`, `bool` conditionals check if the variable match the provided regex pattern,
version specifier, boolean state respectively.

Then you provide any variables you would provide for `tool.scikit-build` to override.
By default, a table (e.g. `cmake.define`) is completely replaced.
`inherit.*` can be used to merge the table with overrides on top of original (`append`) or vice-versa (`prepend`).
For example

::::{tab-set}

:::{tab-item} if does not match

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false
cmake.define.BUILD_TESTS = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```

If environment variable `WITH_MPI` is not defined is equivalent to

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false
cmake.define.BUILD_TESTS = false
```

:::

:::{tab-item} no inheirt

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = false
cmake.define.BUILD_TESTS = false

[[tool.scikit-build.overrides]]
if.env.WITH_MPI = "ON"
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```

If environment variable `WITH_MPI=ON` is equivalent to

```toml
[tool.scikit-build]
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```

:::

:::{tab-item} inherit append

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

If environment variable `WITH_MPI=ON` is equivalent to

```toml
[tool.scikit-build]
cmake.define.BUILD_TESTS = false
cmake.define.BUILD_MPI = true
cmake.define.MPI_PROC = "2"
```

:::

:::{tab-item} inherit prepend

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

If environment variable `WITH_MPI=ON` is equivalent to

```toml
[tool.scikit-build]
cmake.define.BUILD_TESTS = false
cmake.define.BUILD_MPI = false
cmake.define.MPI_PROC = "2"
```

:::

::::
