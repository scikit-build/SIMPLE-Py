---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Publishing with cibuildwheel

---

## One wheel is no longer enough

Pure Python: one `py3-none-any` wheel works everywhere. Compiled code is
native, so you need a wheel per OS, per architecture, per Python version:

$$
3 \text{ OSs} \times 2 \text{ architectures} \times 5 \text{ CPythons} = 30 \text{ wheels}
$$

...and that's before musl-based Linux, PyPy, or free-threaded builds — plus
per-platform quirks to get each one right.

**cibuildwheel** (scikit-learn, matplotlib, mypy, thousands more) turns this
into a six-job matrix.

---

## What cibuildwheel does

On each CI runner, one invocation:

1. Installs every **Python** version it needs (you don't set up Python
   yourself)
2. **Builds** a wheel for each one with your normal build backend
3. **Repairs** each wheel — bundles linked shared libraries, applies the right
   platform tag (auditwheel / delocate / delvewheel)
4. **Tests** each wheel in a fresh environment — the wheel works, not just
   your source tree

---

## manylinux

On Linux, builds happen inside **manylinux** containers: old, standardized
distro images that guarantee the wheel runs on essentially any Linux.

A wheel built on `ubuntu-latest` doesn't require Ubuntu.

<br>

Runs on GitHub Actions, GitLab CI, Azure Pipelines, and more —
**and locally**, which is a great way to debug.

---

## Configuration

Lives in `pyproject.toml` (every option also has a `CIBW_*` environment
variable for one-off overrides):

```toml
[tool.cibuildwheel]
test-groups = ["test"]
test-command = "pytest {project}/tests"
build-frontend = "build[uv]"
```

- `test-groups` + `test-command` — install your `test` group, run pytest
  against the installed wheel (`{project}` = your checkout; tests run
  elsewhere on purpose)
- `build-frontend = "build[uv]"` — faster environments, just needs `uv`
- Later: `skip` (e.g. `"*musllinux*"`), `environment` (compiler flags — Linux
  can't see the host's environment)

---

## Try it locally

Just a Python app — builds wheels for the platform you're on. Linux wheels
use Docker/Podman, so you can build them _from_ macOS or Windows. (On macOS,
native builds need python.org Python — target Linux there.)

Building every version is slow; `--only` takes one build identifier:

```bash
uvx cibuildwheel --print-build-identifiers
uvx cibuildwheel --only cp314-manylinux_x86_64
ls wheelhouse
```

Identifiers are `<python>-<platform>`: `cp314-manylinux_x86_64`,
`cp313-macosx_arm64`, `cp312-win_amd64`, ...

---

## The trigger

30+ wheels takes time — don't build on every PR:

```yaml
# .github/workflows/cd.yml
name: CD

on:
  workflow_dispatch:
  release:
    types:
      - published
  pull_request:
    paths:
      - .github/workflows/cd.yml
```

The `paths:` trick rebuilds wheels when the workflow file itself changes in a
PR — you can't break the release pipeline without noticing.

---

## The SDist job

Don't forget the SDist — it's what everyone without a matching wheel (and
every distro) builds from:

```yaml
make_sdist:
  name: Make SDist
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
      with:
        fetch-depth: 0 # if you use setuptools_scm
        submodules: true # if you have submodules
    - uses: astral-sh/setup-uv@v8
    - name: Build SDist
      run: uv build --sdist
    - uses: actions/upload-artifact@v7
      with:
        name: cibw-sdist
        path: dist/*.tar.gz
```

---

## The wheels job

```yaml
build_wheels:
  name: Wheel on ${{ matrix.os }}
  runs-on: ${{ matrix.os }}
  strategy:
    fail-fast: false
    matrix:
      os: [ubuntu-latest, ubuntu-24.04-arm,
           windows-latest, windows-11-arm,
           macos-15-intel, macos-latest]
  steps:
    - uses: actions/checkout@v7
      with:
        fetch-depth: 0
        submodules: true
    - uses: astral-sh/setup-uv@v8
    - uses: pypa/cibuildwheel@v4.1
    - uses: actions/upload-artifact@v7
      with:
        name: cibw-wheels-${{ matrix.os }}
        path: wheelhouse/*.whl
```

---

## Wheels job notes

- The matrix is over **runners only** — cibuildwheel loops over Python
  versions inside each job
- Six jobs, one line of real work each; all configuration came from
  `pyproject.toml`
- Every architecture here builds **natively** (GitHub has ARM Linux/Windows
  runners and both macOS architectures)
- Rarer platforms can be emulated with QEMU — one extra setup step

---

## The publish job

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
    - uses: actions/attest-build-provenance@v4
      with:
        subject-path: "dist/*"
    - uses: pypa/gh-action-pypi-publish@release/v1
```

---

## What changed vs. pure Python?

- `needs: [build_wheels, make_sdist]` — nothing uploads unless every wheel
  built **and passed its tests**
- Artifacts come from _seven_ jobs — collected with `pattern: cibw-*` +
  `merge-multiple: true` (that's why every artifact name starts with `cibw-`)
- Everything else is identical: guarded `if:`, `environment: pypi`,
  attestations, Trusted Publishing setup

---

## Summary

- Compiled packages need **~30 wheels** per release — cibuildwheel makes that
  a six-job matrix
- Each wheel is **built, repaired, and tested** in isolation; manylinux makes
  Linux wheels portable
- Configure in `pyproject.toml`; debug locally with
  `uvx cibuildwheel --only ...`
- Build on releases + the manual button (+ workflow-file PRs), never every PR
- Ship the **SDist** too; publish with the same Trusted Publishing job as
  pure Python
