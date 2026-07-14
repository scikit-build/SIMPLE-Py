---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Pixi Build

---

# What is Pixi Build?

- Extends everything we've learned so far to **conda packages** with as
  little additional effort as possible
- Package metadata lives directly in the Pixi manifest as TOML tables:
  `package`, `build`, `build-dependencies`, `host-dependencies`, and
  `run-dependencies`
- Combined with the rest of the manifest, that's enough to build and
  install conda packages — no full `rattler-build` recipe required
- We'll adapt the examples from the previous packaging sections

---

# Basic packaging: starting point

Take the `rescale` package from _Making a basic package_ and create a
Pixi manifest as a `pyproject.toml` `[tool.pixi]` table:

```bash
pixi init --format pyproject
```

Environments are created automatically from the optional dependencies
and dependency groups:

```text
✔ Added package 'rescale' as an editable dependency.
✔ Added environment 'dev' from optional dependencies or dependency groups.
```

---

# Enable Pixi Build

Pixi Build is a **preview** feature — opt in:

```toml
preview = ["pixi-build"]
```

Add the platforms you might want to support (like normal):

```bash
pixi workspace platform add linux-64 osx-64 win-64 osx-arm64 linux-aarch64
```

Change `rescale` from a Python package dependency to a **conda package**
dependency:

```diff
-[tool.pixi.pypi-dependencies]
-rescale = { path = ".", editable = true }
+[tool.pixi.dependencies]
+rescale = { path = "." }
```

---

# Define the conda package

<div class="columns">
<div>

The `[package]` table and the Python build backend:

```toml
[tool.pixi.package]
name = "rescale"
version = "0.1.0"

[tool.pixi.package.build.backend]
name = "pixi-build-python"
version = "0.*"
```

</div>
<div>

The `host` and `run` dependencies:

```toml
[tool.pixi.package.host-dependencies]
hatchling = "*"

[tool.pixi.package.run-dependencies]
numpy = ">=1.24"
```

And a `tests` task for the `dev` feature:

```bash
pixi task add --feature dev tests "pytest tests"
```

</div>
</div>

---

# The final `[tool.pixi]` manifest

One `pyproject.toml` describes both the Python **and** conda package:

<div class="columns">
<div>

```toml
[tool.pixi.workspace]
channels = ["conda-forge"]
platforms = [
  "linux-64", "osx-64", "win-64",
  "osx-arm64", "linux-aarch64",
]
preview = ["pixi-build"]

[tool.pixi.dependencies]
rescale = { path = "." }

[tool.pixi.package]
name = "rescale"
version = "0.1.0"
```

</div>
<div>

```toml
[tool.pixi.package.build.backend]
name = "pixi-build-python"
version = "0.*"

[tool.pixi.package.host-dependencies]
hatchling = "*"

[tool.pixi.package.run-dependencies]
numpy = ">=1.24"

[tool.pixi.feature.dev.tasks.tests]
cmd = "pytest tests"
```

</div>
</div>

---

# Building (and installing) the package

Any Pixi command that needs an installed environment builds and
installs the conda package (`pixi install`, `pixi run`, `pixi shell`):

```bash
pixi run tests
```

```console
$ pixi list -x
Installed for: linux-64
Name     Version  Build                    Size  Kind   Source
numpy    2.5.1    py314h2b28147_0      8.67 MiB  conda  https://conda.anaconda.org/conda-forge
python   3.14.6   habeac84_100_cp314  35.02 MiB  conda  https://conda.anaconda.org/conda-forge
rescale                                          conda  .
```

---

# Building a conda package artifact

[`pixi publish`](https://pixi.prefix.dev/latest/reference/cli/pixi/publish/)
builds the conda package and publishes it to a conda channel
(`--target-channel`) or a local directory (`--target-dir`):

```text
pixi publish --clean --target-dir .
```

```text
📦 Publishing 1 package(s) to directory </path/to/cwd>
✔ Successfully published 1 package(s) to directory </path/to/cwd>
  - rescale-0.1.0-pyh4616a5c_0.conda
```

Inspect the package with `rattler-build`:

```text
pixi global install rattler-build
rattler-build package inspect ./*.conda
```

---

# Exercises

## Installing from local conda channels

Publish the package to a local conda channel on your filesystem and
then install the `rescale` Pixi package in a new Pixi workspace.

## Publish to a personal conda channel

Publish the conda package to a personal
[prefix.dev conda channel](https://prefix.dev/channels).

---

# Compiled packaging: `collatz`

Take the `collatz` package from _A minimal compiled package with
scikit-build_ and repeat the same steps:

1. `pixi init --format pyproject`
2. Enable the preview feature: `preview = ["pixi-build"]`
3. Add platforms with `pixi workspace platform add`
4. Swap `[tool.pixi.pypi-dependencies]` → `[tool.pixi.dependencies]`
5. Add `[tool.pixi.package]` and the `pixi-build-python` backend

Compiled extensions need a little more configuration...

---

# Compilers and dependency mapping

```toml
[tool.pixi.package.build.config]
compilers = ["cxx"]
ignore-pypi-mapping = false  # Enable automatic PyPI-to-conda mapping
```

- Declare the **compiler types** you require — Pixi Build figures out
  the rest
- `ignore-pypi-mapping = false` (somewhat confusingly) **enables**
  automatic PyPI-to-conda name mapping, so the `host` dependencies
  (`scikit-build-core`, `pybind11`) don't need to be redefined

---

# Build dependencies and the Ninja gotcha

The build tools come from the `build-dependencies` table:

```toml
[tool.pixi.package.build-dependencies]
cmake = "*"
ninja = "*"
```

`rattler-build` (through `pixi-build-python`) exports
`CMAKE_GENERATOR='Unix Makefiles'`, overriding scikit-build-core's
default of Ninja — restore it with explicit arguments:

```toml
[tool.scikit-build]
# c.f. https://github.com/prefix-dev/rattler-build/issues/2487
cmake.args = ["-GNinja"]
```

---

# Just over 20 lines of TOML

The complete set of additions that turn `collatz` into a conda package:

<div class="columns">
<div>

```toml
[tool.scikit-build]
cmake.args = ["-GNinja"]

[tool.pixi.dependencies]
collatz = { path = "." }

[tool.pixi.package]
name = "collatz"
version = "0.1.0"
```

</div>
<div>

```toml
[tool.pixi.package.build.backend]
name = "pixi-build-python"
version = "0.*"

[tool.pixi.package.build.config]
compilers = ["cxx"]
ignore-pypi-mapping = false

[tool.pixi.package.build-dependencies]
cmake = "*"
ninja = "*"
```

</div>
</div>

---

# Exercises (1/2)

## Build and install the conda package

Build `collatz` as a conda package and install it into your Pixi
workspace environment.

## Build a conda package archive

Build `collatz` as a local conda package archive, publish it to a
local channel, and inspect it.

---

# Exercises (2/2)

## Install collatz as a conda package from source

Add `collatz` as a source dependency to a Pixi workspace and install it
as a conda package
([full example](https://github.com/scikit-build/SIMPLE-Py/tree/main/examples/5_02_pixi_build/compiled)).

## Install a conda package from a new channel

`collatz` was published on the conda channel
<https://prefix.dev/matthewfeickert> — create a new workspace and
install from that channel.
