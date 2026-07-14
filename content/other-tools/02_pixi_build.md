# Pixi-build

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/5_02_pixi_build>`

In this section we'll see how we can use [Pixi Build](https://pixi.prefix.dev/latest/build/getting_started/) to extend all of the packaging concepts that we've learned about so far into conda packages with as little additional effort as possible.
Pixi Build allows for defining metadata in the Pixi manifest that define [package](https://pixi.prefix.dev/latest/reference/pixi_manifest/#the-package-section), [build](https://pixi.prefix.dev/latest/reference/pixi_manifest/#build-table), [build-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#build-dependencies), [host-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#host-dependencies), and [run-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#host-dependencies) TOML tables.
These tables can be used together with the information in the rest of the Pixi manifest to build and install conda packages without having to have the full formal specification of a `rattler-build` recipe, like those shown in the [Shipping to conda-forge section](https://scikit-build.org/SIMPLE-Py/conda-forge/).
To learn the sections and functionalities by example rather than by inspection of the spec, we'll take the examples from previous packaging sections and adapt them to Pixi Build.

## Basic Packaging

### Configuring the Pixi manifest

Take the example Python package directory tree from [Making a basic package](https://scikit-build.org/SIMPLE-Py/package/) as a starting foundation.

```bash
cp -R ./examples/5_02_pixi_build/basic /tmp/
mv /tmp/basic/original_pyproject.toml /tmp/basic/pyproject.toml
cd /tmp/basic
```

Next, create a Pixi manifest as a `pyproject.toml` `[tool.pixi]` table.

```bash
pixi init --format pyproject
```

which automatically creates environments from the optional dependencies and dependency groups.

```text
✔ Added package 'rescale' as an editable dependency.
✔ Added environment 'dev' from optional dependencies or dependency groups.
```

```{code} toml
:filename: pyproject.toml
:linenos:
:emphasize-lines: 32-43
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "rescale"
version = "0.1.0"
description = "Rescale NumPy arrays to span [0, 1]."
readme = "README.md"
authors = [{ name = "My Name", email = "me@email.com" }]
license = "BSD-3-Clause"
license-files = ["LICENSE"]
keywords = ["arrays", "normalization"]
classifiers = [
  "Development Status :: 3 - Alpha",
  "Intended Audience :: Science/Research",
  "Programming Language :: Python :: 3",
  "Topic :: Scientific/Engineering",
  "Private :: Do Not Upload",
]
requires-python = ">=3.10"
dependencies = ["numpy>=1.24"]

[project.urls]
Homepage = "https://github.com/me/rescale"
"Bug Tracker" = "https://github.com/me/rescale/issues"
Changelog = "https://github.com/me/rescale/releases"

[dependency-groups]
dev = ["pytest"]

[tool.pixi.workspace]
channels = ["conda-forge"]
platforms = ["linux-64"]

[tool.pixi.pypi-dependencies]
rescale = { path = ".", editable = true }

[tool.pixi.environments]
default = { solve-group = "default" }
dev = { features = ["dev"], solve-group = "default" }

[tool.pixi.tasks]
```

Now, enable the Pixi Build preview feature

```toml
preview = ["pixi-build"]
```

and add the platforms you might want to support (like normal)

```bash
pixi workspace platform add linux-64 osx-64 win-64 osx-arm64 linux-aarch64
```

change `rescale` from a Python package dependency to a conda package dependency

```diff
-[tool.pixi.pypi-dependencies]
-rescale = { path = ".", editable = true }
+[tool.pixi.dependencies]
+rescale = { path = "." }
```

add a `[package]` TOML table

```toml
[tool.pixi.package]
name = "rescale"
version = "0.1.0"
```

add the Python build backend to the `[package.build.backend]` table

```toml
[tool.pixi.package.build.backend]
name = "pixi-build-python"
version = "0.*"
```

add the `host` and `run` dependencies package tables

```toml
[tool.pixi.package.host-dependencies]
hatchling = "*"

[tool.pixi.package.run-dependencies]
numpy = ">=1.24"
```

and then add a `tests` task to the `dev` feature

```bash
pixi task add --feature dev tests "pytest tests"
```

resulting in the final `pyproject.toml` containing all the information for both a Python and conda package of `rescale`.

```{literalinclude} ../../examples/5_02_pixi_build/basic/pyproject.toml
:filename: pyproject.toml
:linenos:
:emphasize-lines: 34-52,58-59
```

### Building (and installing) the package

To build and install the conda package into your environment run any Pixi command that requires an environment to be installed (such as `pixi install`, `pixi run`, `pixi shell`).

```bash
pixi run tests
```

You can see the build logs from Pixi Build and see that `rescale` is installed as a conda package

```console
$ pixi list -x
Installed for: linux-64
Name     Version  Build                    Size  Kind   Source
numpy    2.5.1    py314h2b28147_0      8.67 MiB  conda  https://conda.anaconda.org/conda-forge
python   3.14.6   habeac84_100_cp314  35.02 MiB  conda  https://conda.anaconda.org/conda-forge
rescale                                          conda  .
```

### Building a conda package

We can use [`pixi publish`](https://pixi.prefix.dev/latest/reference/cli/pixi/publish/) to build the package into a conda package and either publish it to a (`--target-channel`) conda channel or copy the artifact into a (`--target-dir`) local directory.

```text
pixi publish --clean --target-dir .
```

```text
📦 Publishing 1 package(s) to directory </path/to/cwd>
✔ Successfully published 1 package(s) to directory </path/to/cwd>
  - rescale-0.1.0-pyh4616a5c_0.conda
```

We can use `rattler-build` to inspect the package and get more information about it

```text
pixi global install rattler-build
rattler-build package inspect ./*.conda
```

::::{exercise} Installing from local conda channels
:label: local-conda-channels

Above we mentioned that `pixi publish` can publish to a conda channel.
Publish the package to a local conda channel on your filesystem and then install the `rescale` Pixi package in a new Pixi workspace.

:::{solution} local-conda-channels
:class: dropdown

First publish the conda package to a local channel on the file system.

```bash
pixi publish --clean --target-channel /tmp/local-channel
```

Then create a new Pixi workspace and add the package

```bash
pixi init /tmp/local-channel-example && cd /tmp/local-channel-example
pixi workspace channel add --prepend "/tmp/local-channel"
pixi add rescale
pixi list rescale
```

:::

::::

::::{exercise} Publish to a personal conda channel
:label: personal-conda-channel

Publish the conda package to a personal prefix.dev conda channel.

:::{solution} personal-conda-channel
:class: dropdown

If you have a [personal conda channel](https://prefix.dev/channels) on prefix.dev you can publish with

```bash
pixi publish --clean --target-channel https://prefix.dev/<channel-name>
```

:::

::::

## Compiled Packaging

Take the example Python package directory tree from the [A minimal compiled package with scikit-build](https://scikit-build.org/SIMPLE-Py/compiled) section as a starting foundation.

```bash
cp -R ./examples/2_01_package/collatz /tmp/compiled
cd /tmp/compiled
```

Next, create a Pixi manifest as a `pyproject.toml` `[tool.pixi]` table.

```bash
pixi init --format pyproject
```

```{code} toml
:filename: pyproject.toml
:linenos:
:emphasize-lines: 13-16
[build-system]
requires = ["scikit-build-core", "pybind11"]
build-backend = "scikit_build_core.build"

[project]
name = "collatz"
version = "0.1.0"

[tool.pixi.workspace]
channels = ["conda-forge"]
platforms = ["linux-64"]

[tool.pixi.pypi-dependencies]
collatz = { path = ".", editable = true }

[tool.pixi.tasks]
```

Now, enable the Pixi Build preview feature

```toml
preview = ["pixi-build"]
```

and add the platforms you might want to support (like normal)

```bash
pixi workspace platform add linux-64 osx-64 win-64 osx-arm64 linux-aarch64
```

change `collatz` from a Python package dependency to a conda package dependency

```diff
-[tool.pixi.pypi-dependencies]
-collatz = { path = ".", editable = true }
+[tool.pixi.dependencies]
+collatz = { path = "." }
```

add a `[package]` TOML table

```toml
[tool.pixi.package]
name = "collatz"
version = "0.1.0"
```

add the Python build backend to the `[package.build.backend]` table.

```toml
[tool.pixi.package.build.backend]
name = "pixi-build-python"
version = "0.*"
```

As we are using compilers we need to tell Pixi Build the compiler types that we require, and it can figure out the rest.
We can also set `ignore-pypi-mapping` to automatically enable PyPI-to-conda name mapping (which somewhat confusingly happens under `false`) to not have to redefine the `host` dependencies.

```toml
[tool.pixi.package.build.config]
# c.f. https://pixi.prefix.dev/latest/build/backends/pixi-build-python/#compilers
# c.f. https://pixi.prefix.dev/latest/build/backends/pixi-build-python/#ignore-pypi-mapping
compilers = ["cxx"]
ignore-pypi-mapping = false  # Enable automatic PyPI-to-conda mapping

...

# Unneeded given package.build.config.ignore-pypi-mapping = false
# [tool.pixi.package.host-dependencies]
# scikit-build-core = "*"
# pybind11 = "*"
```

Then add the the necessary `build` requirements.

```toml
[tool.pixi.package.build-dependencies]
cmake = "*"
ninja = "*"
```

Finally, we want to take advantage of `scikit-build-core`'s defaults of using Ninja.
`rattler-build` (through `pixi-build-python`) exports `CMAKE_GENERATOR='Unix Makefiles'` as a default behavior, which overrides `scikit-build-core`'s default of Ninja.[^1]
To restore the default behavior provide `scikit-build-core` explicit arguments.

```toml
[tool.scikit-build]
# c.f. https://github.com/prefix-dev/rattler-build/issues/2487
cmake.args = ["-GNinja"]
```

```{literalinclude} ../../examples/5_02_pixi_build/compiled/pyproject.toml
:filename: pyproject.toml
:linenos:
:emphasize-lines: 15-17, 19-38
```

So in just over 20 lines of TOML, we can build a conda package for a Python package with compiled extensions!

::::{exercise} Build and install the conda package
:label: build-and-install-collatz

Build `collatz` as a conda package and install it into your Pixi workspace environment.

:::{solution} build-and-install-collatz
:class: dropdown

```bash
pixi install
pixi run python -c 'from collatz import collatz_steps; print(collatz_steps(27))'
```

You can verify with

```bash
pixi list -x
```

:::

::::

::::{exercise} Build a conda package archive
:label: publish-collatz

Build `collatz` as a local conda package archive and publish it to a local channel and inspect it.

:::{solution} publish-collatz
:class: dropdown

```bash
pixi publish --clean --target-channel /tmp/local-channel
rattler-build package inspect /tmp/local-channel/linux-64/collatz*.conda
```

:::

::::

::::{exercise} Install collatz as a conda package from source
:label: conda-source-collatz

A full example is available at <https://github.com/scikit-build/SIMPLE-Py/tree/main/examples/5_02_pixi_build/compiled>.
Add `collatz` as a source dependency to a Pixi workspace and install it as a conda package.

:::{solution} conda-source-collatz
:class: dropdown

```bash
pixi init git-source-pixi-build && cd git-source-pixi-build
```

```toml
[workspace]
channels = ["conda-forge"]
name = "git-source-pixi-build"
platforms = ["linux-64"]
version = "0.1.0"
preview = ["pixi-build"]

[tasks]

[dependencies]

[dependencies.collatz]
git = "https://github.com/scikit-build/SIMPLE-Py"
branch = "main"
subdirectory = "examples/5_02_pixi_build/compiled"
```

:::

::::

::::{exercise} Install a conda package from a new channel
:label: prefix-channel-collatz

`collatz` was published on the conda channel <https://prefix.dev/matthewfeickert>.

```bash
pixi auth login --token <token> prefix.dev
pixi publish --clean --target-channel https://prefix.dev/matthewfeickert
```

Create a new workspace and install from that channel.

:::{solution} prefix-channel-collatz
:class: dropdown

```bash
pixi init prefix-channel-example && cd prefix-channel-example
pixi workspace channel add https://prefix.dev/matthewfeickert
pixi add collatz
pixi list -x
```

:::

::::

[^1]: c.f. <https://github.com/prefix-dev/rattler-build/issues/2487>
