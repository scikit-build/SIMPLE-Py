---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Shipping to conda-forge

---

# What is a conda package?

- A zip compressed archive (`.conda`) with two `.tar.zst` archives inside
  - One with the package files
  - One with the metadata (JSON: structure, provenance, dependencies)
- Relocatable, **platform-specific binaries**
- Simple yet general and **language agnostic**
  - Effectively any software can be a conda package

---

# What are "platforms"?

Operating system + architecture combinations:

| Platform | Meaning |
|---|---|
| `linux-64` | Linux x86_64 |
| `linux-aarch64` | Linux AArch64 |
| `osx-64` | macOS x86_64 |
| `osx-arm64` | macOS Apple silicon |
| `win-64` | Windows x86_64 |
| `win-arm64` | Windows ARM64 |

---

# Building a conda package

- Packages are defined by a recipe: `recipe.yaml`
  - Jinja2 templating, robust schema
- Recipes are built into packages with [`rattler-build`](https://rattler-build.prefix.dev/)

```bash
# pixi global install rattler-build
rattler-build build --recipe <path to recipe directory>
```

Multiple recipe formats and build tools exist; we focus on the new
`rattler-build` ecosystem.

---

# Recipe sections

| Section | Description |
|---|---|
| `context` | Variables for the Jinja context (e.g. version) |
| `package` | Name and version of the output package |
| `source` | Where source is downloaded from + checksums |
| `build` | Settings for the build and the build script |
| `requirements` | build, host, run & run-constrained dependencies |

---

# Conda package dependencies

## The golden rule

- Conda packages require **all** dependencies to exist as conda packages
- Conda-forge packages require **all** dependencies to exist **on conda-forge**

---

# Pure-Python recipe (`noarch: python`)

<div class="columns">
<div>

```yaml
schema_version: 1

context:
  version: 1.2.3
  python_min: "3.10"

package:
  name: example-package
  version: ${{ version }}

source:
  url: https://pypi.org/packages/...
  sha256: 12ff4785d337a1bb...
```

</div>
<div>

```yaml
build:
  noarch: python
  number: 0
  script: >-
    ${{ PYTHON }} -m pip install .
    -vv --no-deps --no-build-isolation

requirements:
  host:
    - python ${{ python_min }}.*
    - hatchling  # your build backend
    - pip
  run:
    - python >=${{ python_min }}
```

</div>
</div>

---

# Pure-Python recipe (continued)

<div class="columns">
<div>

```yaml
tests:
  - python:
      imports:
        - example_package
      python_version:
        - ${{ python_min }}.*
        - "*"
      pip_check: true
```

</div>
<div>

```yaml
about:
  homepage: https://example.com/...
  license: BSD-3-Clause
  license_file: LICENSE
  summary: Single-line summary.

extra:
  recipe-maintainers:
    - henryiii
    - matthewfeickert
```

</div>
</div>

---

# Compiled extensions

- Built against **every Python version** conda-forge supports
- Compilers come from conda packages, not your system
- Cross-compilation supported, but native runners now exist for
  `linux-aarch64`, `osx-arm64`, and `win-arm64`
  - Unless you target something niche (`linux-ppc64le`), you may not
    need cross-compilation at all

---

# Compiled extension recipe

<div class="columns">
<div>

```yaml
build:
  number: 0
  script:
    env:
      CMAKE_GENERATOR: Ninja
    content:
      - >-
        ${{ PYTHON }} -m pip install .
        -vv --no-deps --no-build-isolation
```

</div>
<div>

```yaml
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
```

</div>
</div>

---

# Submitting to conda-forge

- conda-forge builds everything on a community build farm
- Each package gets a **feedstock**: a repo with the recipe,
  scripts, and CI at `github.com/conda-forge/<name>-feedstock`
- New recipes go through
  [conda-forge/staged-recipes](https://github.com/conda-forge/staged-recipes)

---

# The staged-recipes workflow (1/2)

1. Fork and clone `conda-forge/staged-recipes`
2. Branch from `upstream/main`
3. Create the recipe at `./recipes/<package name>/`
   - On PyPI already? Generate it with `grayskull`:

     ```bash
     pixi run pypi boost-histogram
     ```

   - Otherwise, copy `./recipes/example-v1` and edit `recipe.yaml`

---

# The staged-recipes workflow (2/2)

4. Lint the recipe: `pixi run lint`
5. Test build locally: `pixi run <build-linux|build-osx|build-win>`
6. Push and open a **draft** PR; when green, ping a review team:
   - `@conda-forge/help-python, ready for review!` (pure Python)
   - `@conda-forge/help-python-c, ready for review!` (compiled)
7. On merge, automation creates your feedstock and maintainer team,
   and packages upload to `anaconda.org/channels/conda-forge`

---

# Getting help

## When in doubt, just ask!

- The [conda-forge Zulip](https://conda-forge.zulipchat.com/) is the best
  way to reach the community and conda-forge/core
- Check the
  [maintainer knowledge base](https://conda-forge.org/docs/maintainer/knowledge_base/)
  first

---

# A full `rattler-build` recipe: `collatz` (1/2)

<div class="columns">
<div>

```yaml
package:
  name: rattler-collatz
  version: ${{ version }}

source:
  url: https://github.com/scikit-build/...
  sha256: fbd494e7216db678...
```

</div>
<div>

```yaml
build:
  number: 0
  script:
    env:
      CMAKE_GENERATOR: Ninja
    content:
      - cd examples/5_02_pixi_build/compiled
      - >-
        ${{ PYTHON }} -m pip install .
        -vv --no-deps --no-build-isolation
```

</div>
</div>

---

# A full `rattler-build` recipe: `collatz` (2/2)

<div class="columns">
<div>

```yaml
requirements:
  build:
    - ${{ compiler("cxx") }}
    - cmake
    - ninja
    - if: build_platform != host_platform
      then:
        - cross-python_${{ host_platform }}
        - python
  host:
    - python
    - pip
    - scikit-build-core
    - pybind11
  run:
    - python
```

</div>
<div>

```yaml
tests:
  - python:
      imports:
        - collatz
      pip_check: true
```

Plus the usual `about` and `extra`
sections

</div>
</div>

---

# Exercise: build the `rattler-build` recipe

Using
[the full recipe for `collatz`](https://github.com/scikit-build/SIMPLE-Py/blob/main/examples/5_02_pixi_build/compiled/recipe.yaml),
build the recipe file into a conda package with `rattler-build` and
then inspect the package.
