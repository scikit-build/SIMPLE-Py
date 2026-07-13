# Shipping to conda-forge

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/5_03_conda_forge>`

## What is a conda package (short version)?

A conda package is fundamentally a zip compressed archive (`.conda`) that contains two `.tar.zst` archives with one containing the package files and the other containing all the metadata.
The JSON metadata describes the package structure, provenance, requirements, and dependencies.
The package files consist of relocatable, **platform-specific binaries** and, optionally, symlinks (on supported platforms).
The package format is simple yet general, powerful, and **language agnostic** &mdash; effectively any software can be packaged as a conda package.

<!-- https://mystmd.org/guide/admonitions -->
::: {tip} What are "platforms"?
:class: dropdown

Conda "platforms" are combinations of operating systems (e.g. Linux, macOS) and computing architectures (e.g. x86_64, AArch64) for which a particular conda package is built.

Example of conda platform name mapping:

* `linux-64`: Linux x86_64
* `linux-aarch64`: Linux AArch64
* `osx-64`: macOS x86_64
* `osx-arm64`: macOS Apple silicon (ARM64-based)
* `win-64`: Windows x86_64
* `win-arm64`: Windows ARM64

:::

## Building a conda package

Conda packages are defined through a package recipe YAML file (`recipe.yaml`) with Jinja2 templating support and a robust schema. Recipes are built with a recipe build tool ([`rattler-build`](https://rattler-build.prefix.dev/)) into packages (the archives that are distributed).

::: {note} Tooling choices

There are multiple conda package recipe formats and build tools, but for the purposes of this workshop we are only going to focus on the new `rattler-build` ecosystem.

:::

The recipe file is separated into [multiple sections](https://rattler-build.prefix.dev/latest/highlevel/#overview-of-a-recipeyaml) that can conditionally be excluded or included

<!-- https://github.com/prefix-dev/rattler-build/blob/82c50220449b38c2a3e6602da14c9a0b7bef8fb8/docs/highlevel.md?plain=1#L64-L84 -->

| Sections       | Description                                                                                                                              |
|----------------|------------------------------------------------------------------------------------------------------------------------------------------|
| `context`      | Defines variables that can be used in the Jinja context later in the recipe (e.g. version is commonly interpolated in strings) |
| `package`      | Defines the name and version of the package you are currently building and will be the name of the final output             |
| `source`       | Defines where the source code is going to be downloaded from and checksums                                                               |
| `build`        | Settings for the build and the build script                                                                                              |
| `requirements` | Allows the definition of build, host, run and run-constrained dependencies |

`rattler-build` can then be used to build the recipe at the command line with `rattler-build`'s CLI API:

```text
# pixi global install rattler-build
rattler-build build --recipe <path to recipe directory>
```

## Example conda recipes

::: {important} Conda package dependencies

Conda packages require that **all** of their dependencies exist as conda packages already.
Conda-forge packages require that **all** of their dependencies already exist as conda packages **on conda-forge**.

:::

### Pure-Python (`noarch: python`)

(From the [conda-forge maintainer documentation](https://conda-forge.org/docs/maintainer/example_recipes/pure-python/))

```yaml
schema_version: 1

context:
  version: 1.2.3
  python_min: "3.10"

package:
  name: example-package
  version: ${{ version }}

source:
  url: https://pypi.org/packages/source/e/example-package/example_package-${{ version }}.tar.gz
  sha256: 12ff4785d337a1bb490bb7e9c2b1ee5da3112e94a8622f26a6c77f5d2fc6842a

build:
  noarch: python
  number: 0
  script: ${{ PYTHON }} -m pip install . -vv --no-deps --no-build-isolation

requirements:
  host:
    - python ${{ python_min }}.*
    - hatchling  # your build-system build tool goes here
    - pip
  run:
    - python >=${{ python_min }}

tests:
  - python:
      imports:
        - example_package
      python_version:
        - ${{ python_min }}.*
        - "*"
      pip_check: true

about:
  homepage: https://example.com/example-package
  license: BSD-3-Clause
  license_file: LICENSE
  summary: Single-line summary of the package.
  description: |
    One or two paragraphs with more information about the package.
  repository: https://github.com/example/example-package/
  documentation: https://example.com/example-package-docs/

extra:
  recipe-maintainers:
    # GitHub username
    - henryiii
    - matthewfeickert
```

### Compiled extensions

If the package recipe has compiled extensions the package will be built against every version of Python that is currently supported by conda-forge.

::: {hint} Cross-compilation on conda-forge

Conda packages can be built for platforms other than the platform of the machine running the build through cross-compilation (or in the worst case scenario, emulation).
The conda-forge documentation has [resources on cross-compilation](https://conda-forge.org/docs/how-to/advanced/cross-compilation/) and how to make conda package recipes cross-compilation ready for build anywhere (as seen in the following example).

However, as of 2026, in addition to native operating system x86_64 build runners, conda-forge has support for native builds on Linux AArch64 (`linux-aarch64`), macOS Apple silicon (`osx-arm64`), and Windows ARM64 (though not many packages have been built against this).
So unless you need to target a more niche platform, like `linux-ppc64le`, you no longer need to design your conda-forge recipes for cross-compilation!

:::

```yaml
schema_version: 1

context:
  version: 1.2.3

package:
  name: example-package
  version: ${{ version }}

source:
  url: https://pypi.org/packages/source/e/example-package/example_package-${{ version }}.tar.gz
  sha256: 12ff4785d337a1bb490bb7e9c2b1ee5da3112e94a8622f26a6c77f5d2fc6842a

build:
  number: 0
  script:
    env:
      CMAKE_GENERATOR: Ninja
    content:
      - ${{ PYTHON }} -m pip install . -vv --no-deps --no-build-isolation

requirements:
  build:
    - ${{ stdlib("c") }}
    - ${{ compiler("cxx") }}
    - cmake
    - ninja
    - if: build_platform != host_platform
      then:
        - cross-python_${{ host_platform }}
        - python
        - numpy
  host:
    - python
    - pip
    - scikit-build-core
    - numpy
  run:
    - python
    - numpy

tests:
  - python:
      imports:
        - example_package
      pip_check: true
  - script:
      - example-package-cli --help

about:
  homepage: https://example.com/example-package
  license: BSD-3-Clause
  license_file: LICENSE
  summary: Single-line summary of the package.
  description: |
    One or two paragraphs with more information about the package.
  repository: https://github.com/example/example-package/
  documentation: https://example.com/example-package-docs/

extra:
  recipe-maintainers:
    # GitHub username
    - henryiii
    - matthewfeickert
```

## Submitting a package to conda-forge

The building of packages for the conda-forge conda channel is centralized through a community maintained build farm which builds all of the conda-forge feedstocks &mdash; GitHub repository with the conda package recipe, supporting scripts, and CI configuration &mdash; on <https://github.com/conda-forge/>.
To get a conda recipe on conda-forge you need to submit it for build checks and review to <https://github.com/conda-forge/staged-recipes>.

### Submitting recipes to `conda-forge/staged-recipes` workflow

1. Fork <https://github.com/conda-forge/staged-recipes> to your personal GitHub.

   ```text
   # pixi global install gh git
   gh repo fork conda-forge/staged-recipes --clone=false
   git clone --depth 10 git@github.com:<your GitHub account username>/staged-recipes.git
   cd staged-recipes
   git remote add upstream git@github.com:conda-forge/staged-recipes.git
   git fetch --all --depth 10 --prune
   # You can optionally unshallow your clone if you want
   ```

2. Make a **new** branch from `main` for your package's recipe.

   ```text
   git checkout upstream/main -b feat/add-my-package
   ```

3. Create the package recipe

    * If the package exists on PyPI already use Pixi and `grayskull` to [generate the recipe](https://github.com/conda-forge/staged-recipes#generating-recipes-with-grayskull)

    ```text
    pixi run pypi <PyPI package name>
    ```

   Example:

    ```text
    pixi run pypi boost-histogram
    ```

   This will produce the recipe at `./recipes/<PyPI package name>/`.

    * If the package is not already on PyPI, copy the example `rattler-build` ("v1") recipe into a directory named for your package (replacing `example_package` with your unique package name)

    ```text
    cp -R ./recipes/example-v1 ./recipes/example_package
    ```

   and then edit the package's `recipe.yaml` to meet your package needs following the hints in the comments, in the `README.md`, and the [`rattler-build` docs/tutorials](https://rattler-build.prefix.dev/latest/tutorials/python/).

4. Once you have your recipe ready, stage and commit it to Git, and then lint the recipe

   ```text
   pixi run lint
   ```

   If the linter fails, address the issues before continuing.

5. Run a test build of the package locally.

   ```text
   pixi run <build-linux|build-osx|build-win>
   ```

6. If the test build passes, make sure all your changes are committed, and push your branch to your fork

   ```text
   git push -u origin HEAD
   ```

   and then open a **draft** pull request against the `upstream`'s `main` branch.

   Ensure that you read, follow, and complete the checklist.
   Once your builds are passing, mark your PR as ready for review and tag the relevant [conda-forge/staged-recipes review team](https://github.com/conda-forge/staged-recipes#review-teams).
   For pure-Python packages this would be

   ```text
   @conda-forge/help-python, ready for review!
   ```

   and for packages with compiled extensions this would be

   ```text
   @conda-forge/help-python-c, ready for review!
   ```

   Once you do so, the conda-forge-webservices will apply the relevant tags for the review team to find the recipe.

7. Once your recipe is reviewed and approved, it will be merged, which will then kick off automation to generate a conda-forge feedstock at `https://github.com/conda-forge/<package name>-feedstock` and to generate a maintainer team on GitHub populated by the `extra.recipe-maintainers` list in the package recipe.
   Your package feedstock will be automatically regenerated by the `conda-forge-admin` bot account and then the built packages will be automatically uploaded to <https://anaconda.org/channels/conda-forge>.

## Getting help

When in doubt, just ask!
The [conda-forge Zulip server](https://conda-forge.zulipchat.com/) is the best way to get responses from the community and the conda-forge/core team, but also make sure to [check out the maintainer knowledge base in the documentation](https://conda-forge.org/docs/maintainer/knowledge_base/) first.
