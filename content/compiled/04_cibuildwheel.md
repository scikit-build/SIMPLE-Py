# Publishing with cibuildwheel

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/2_04_cibuildwheel>`

In the [basic publishing chapter](../basic-packaging/05_publishing_ci.md), one
job built an SDist and a wheel, and one job uploaded them. If you have
compiled components, you need slightly more than one wheel::

$$ 3 \text{ OSs} \times 2 \text{ architectures} \times 5 \text{ CPython versions} = 30 \text{ wheels} $$

...and that's before musl-based Linux, PyPy, or free-threaded builds. There
are quite a few quirks to doing this properly on each platform. [cibuildwheel][] is the standard tool (used by
scikit-learn, matplotlib, mypy, and thousands more) that turns this into a
six-job matrix.

## What cibuildwheel does

On each CI runner, one cibuildwheel invocation:

1. Installs every **Python** version it needs (you don't set up Python yourself).
2. **Builds** a wheel for each one with your normal build backend.
3. **Repairs** each wheel: bundles any shared libraries it links against and
   applies the right platform tag ([auditwheel][] on Linux, [delocate][] on
   macOS, optionally [delvewheel][] on Windows).
4. **Tests** each wheel in a fresh environment, so you know the wheel works,
   not just your source tree.

On Linux, the builds happen inside [manylinux][] containers: old, standardized
distro images that guarantee the resulting wheel runs on essentially any Linux.
This is why a wheel built on `ubuntu-latest` doesn't require Ubuntu.

cibuildwheel runs on GitHub Actions, GitLab CI, Azure Pipelines, and
more, and also runs locally, which is a great way to debug.

## Configuration

cibuildwheel reads configuration from `[tool.cibuildwheel]` in
`pyproject.toml` (every option also has a `CIBW_*` environment variable, handy
for one-off overrides in CI). A good starting set:

```{code} toml
:filename: pyproject.toml
[tool.cibuildwheel]
test-groups = ["test"]
test-command = "pytest {project}/tests"
build-frontend = "build[uv]"
```

- `test-groups` installs your `test` dependency-group before testing each
  wheel, and `test-command` runs your tests against the installed wheel;
  `{project}` is a placeholder for your source checkout, since the tests run
  from a different directory (you're testing the wheel, not the source tree!).
- `build-frontend = "build[uv]"` uses uv to make the build environments, which
  is noticeably faster; it just needs `uv` on the runner.

Other options you'll reach for eventually: `skip` (e.g. `"*musllinux*"` to drop
musl wheels), and `environment` (set environment variables like compiler flags
inside the build, linux runs in a container so doesn't see the host's
environment). See the [options docs][cibw options].

## Try it locally

You don't need CI to run cibuildwheel; it's just a Python app. It builds
wheels for the platform you're on (Linux wheels use Docker or
Podman, since they build inside manylinux containers, which also means you can
build Linux wheels _from_ macOS or Windows). On macOS, it needs `python.org` Python,
so I'd recommend targeting linux there.

Building every Python version locally is slow, so there's a `--only` flag that
takes a single build identifier:

```bash
uvx cibuildwheel --only cp314-manylinux_x86_64
```

Identifiers follow the pattern `<python>-<platform>`, like
`cp314-manylinux_x86_64`, `cp313-macosx_arm64`, or `cp312-win_amd64`.

:::{exercise} A wheel on your machine
:label: cibw-local

Using the compiled package you built earlier in this section:

1. Run `uvx cibuildwheel --print-build-identifiers` to see what a full run
   on your machine would build. How many wheels is it?
2. Build just one of them with `--only`, and look in `wheelhouse/`.

:::

:::{solution} cibw-local
:class: dropdown

```bash
uvx cibuildwheel --print-build-identifiers
uvx cibuildwheel --only cp314-macosx_arm64   # pick one from the list
ls wheelhouse
```

The identifier list depends on your OS; on Linux you'll see both `manylinux`
and `musllinux` entries, which is why the count is larger there. The wheel in
`wheelhouse/` has a platform-specific tag like `cp314-cp314-macosx_11_0_arm64`
instead of pure Python's `py3-none-any`.

:::

## The workflow

Everything from the [pure Python workflow](../basic-packaging/05_publishing_ci.md)
carries over: separate CD workflow, build jobs feeding artifacts to a guarded
publish job, Trusted Publishing. Only the build jobs change.

### The trigger

Building 30+ wheels takes time, so don't do it on every PR. Releases and
the manual button are the usual triggers, plus one nice trick: rebuild when
the workflow file itself changes in a PR, so you can't break the release
pipeline without noticing.

```{code} yaml
:filename: .github/workflows/cd.yml
name: CD

on:
  workflow_dispatch:
  release:
    types:
      - published
  pull_request:
    paths:
      - .github/workflows/cd.yml

jobs:
```

### The SDist job

Don't forget the SDist; it's what everyone without a matching wheel (and every
distro) builds from. This is the same single job as before, just split out
from wheel building:

```yaml
make_sdist:
  name: Make SDist
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
      with:
        fetch-depth: 0 # Optional, use if you use setuptools_scm
        submodules: true # Optional, use if you have submodules

    - uses: astral-sh/setup-uv@v8

    - name: Build SDist
      run: uv build --sdist

    - uses: actions/upload-artifact@v7
      with:
        name: cibw-sdist
        path: dist/*.tar.gz
```

### The wheels job

The matrix is over _runners_ only; cibuildwheel handles the loop over Python
versions inside each job:

```yaml
build_wheels:
  name: Wheel on ${{ matrix.os }}
  runs-on: ${{ matrix.os }}
  strategy:
    fail-fast: false
    matrix:
      os:
        - ubuntu-latest
        - ubuntu-24.04-arm
        - windows-latest
        - windows-11-arm
        - macos-15-intel
        - macos-latest

  steps:
    - uses: actions/checkout@v7
      with:
        fetch-depth: 0
        submodules: true

    - uses: astral-sh/setup-uv@v8

    - uses: pypa/cibuildwheel@v4.1

    - name: Upload wheels
      uses: actions/upload-artifact@v7
      with:
        name: cibw-wheels-${{ matrix.os }}
        path: wheelhouse/*.whl
```

Six jobs, one line of real work each; all the configuration came from
`pyproject.toml`. Every architecture here is built natively (GitHub provides
ARM Linux/Windows runners and both macOS architectures); rarer platforms can
be emulated with QEMU, one extra setup step.

### The publish job

Nearly identical to the pure Python version; the only difference is
collecting artifacts from _seven_ jobs instead of one, which is what the
`pattern:` + `merge-multiple:` options do (and why we prefixed every artifact
name with `cibw-`):

```yaml
upload_all:
  needs: [build_wheels, make_sdist]
  environment: pypi
  permissions:
    id-token: write
    attestations: write
    contents: read
  runs-on: ubuntu-latest
  if: github.event_name == 'release' && github.event.action == 'published'
  steps:
    - uses: actions/download-artifact@v8
      with:
        pattern: cibw-*
        path: dist
        merge-multiple: true

    - name: Generate artifact attestations
      uses: actions/attest-build-provenance@v4
      with:
        subject-path: "dist/*"

    - uses: pypa/gh-action-pypi-publish@release/v1
```

The `needs:` list now has both build jobs, so nothing uploads unless every
wheel built and passed its tests. Setting up Trusted Publishing (and the token
fallback) is exactly as described in
[the previous publishing chapter](../basic-packaging/05_publishing_ci.md).

:::{exercise} Wheels for your package
:label: cibw-workflow

Give your compiled package a `.github/workflows/cd.yml` using the pieces
above, and add the `[tool.cibuildwheel]` configuration to its
`pyproject.toml`. If you have it on GitHub, push it to a branch and trigger
the workflow with the "Run workflow" button (`workflow_dispatch` runs build
everything but skip the upload); then download the wheels from the run's
artifacts.

:::

:::{solution} cibw-workflow
:class: dropdown

The full file is the four snippets above concatenated: the `on:` block, then
`make_sdist`, `build_wheels`, and `upload_all` under `jobs:`. Compare with
[cookie][]'s generated `cd.yml` if you get stuck, or trim the matrix to a
single OS while experimenting to keep runs fast.

:::

[auditwheel]: https://github.com/pypa/auditwheel
[cibuildwheel]: https://cibuildwheel.pypa.io
[cibw options]: https://cibuildwheel.pypa.io/en/stable/options/
[cookie]: https://github.com/scientific-python/cookie
[delocate]: https://github.com/matthew-brett/delocate
[delvewheel]: https://github.com/adang1345/delvewheel
[manylinux]: https://github.com/pypa/manylinux
