# Pixi-build

In this section we'll see how we can use [Pixi Build](https://pixi.prefix.dev/latest/build/getting_started/) to extend all of the packaging concepts that we've learned about so far into conda packages with as little additional effort as possible.
Pixi Build allows for defining metadata in the Pixi manifest that define [package](https://pixi.prefix.dev/latest/reference/pixi_manifest/#the-package-section), [build](https://pixi.prefix.dev/latest/reference/pixi_manifest/#build-table), [build-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#build-dependencies), [host-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#host-dependencies), and [run-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#host-dependencies) TOML tables.
These tables can be used together with the information in the rest of the Pixi manifest to build and install conda packages without having to have the full formal specification of a `rattler-build` recipe, like those shown in the [Shipping to conda-forge section](https://scikit-build.org/SIMPLE-Py/conda-forge/).
To learn the sections and functionalities by example rather than by inspection of the spec, we'll take the examples from previous packaging sections and adapt them to Pixi Build.

## Basic Packaging

### Building (and installing) the package

Run any Pixi command that requires an environment to be installed (such as `pixi install`, `pixi run`, `pixi shell`).

```text
pixi run tests
```

You can see the build logs from Pixi Build and see that `rescale` is installed as a conda package

```text
$ pixi list -x
Installed for: linux-64
Name     Version  Build                    Size  Kind   Source
numpy    2.5.1    py314h2b28147_0      8.67 MiB  conda  https://conda.anaconda.org/conda-forge
python   3.14.6   habeac84_100_cp314  35.02 MiB  conda  https://conda.anaconda.org/conda-forge
rescale                                          conda  .
```

### Building a conda package

We can use [`pixi publish`](https://pixi.prefix.dev/latest/reference/cli/pixi/publish/) to build the package into a conda package and either publish it to a (`--target-channel`) conda channel or copies the artifact into a (`--target-dir`) local directory &mdash; which by default is the current working directory (`.`).

```text
pixi publish --clean
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
