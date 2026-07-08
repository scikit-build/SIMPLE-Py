---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Publishing and CI

---

## GitHub Actions in a nutshell

The standard CI for scientific Python: free for open source, tightly
integrated with GitHub, endlessly extensible.

- A **workflow** is a YAML file in `.github/workflows/` that runs on _events_
  (a PR, a push, a release, a button press)
- A workflow contains **jobs**, which run in parallel on fresh virtual
  machines
- A job is a list of **steps** — shell commands (`run:`) or reusable
  **actions** (`uses:`)

---

## Every CI workflow starts the same way

```yaml
# .github/workflows/ci.yml
name: CI

on:
  pull_request:
  push:
    branches:
      - main

concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
```

The `concurrency` block is a freebie — pushing a fix cancels the
already-obsolete run.

---

## A lint job

You already run static checks through prek/pre-commit — CI just runs the same
thing. One config, identical results locally and in CI:

```yaml
lint:
  name: Lint
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
    - uses: j178/prek-action@v2
```

> Or let [pre-commit.ci](https://pre-commit.ci) run it for you — it can even
> auto-fix PRs.

---

## A test matrix

```yaml
tests:
  runs-on: ${{ matrix.runs-on }}
  strategy:
    fail-fast: false
    matrix:
      python-version: ["3.10", "3.12", "3.14"]
      runs-on: [ubuntu-latest, macos-latest, windows-latest]
  name: Test ${{ matrix.python-version }} on ${{ matrix.runs-on }}
  steps:
    - uses: actions/checkout@v7

    - uses: astral-sh/setup-uv@v8

    - name: Test package
      run: uv run --python ${{ matrix.python-version }} pytest
```

---

## Matrix notes

- A `matrix` expands into one job per combination (here 3 × 3 = 9)
- `fail-fast: false` keeps one red job from cancelling the others
- `uv run` does everything: gets the Python, makes the environment, installs
  your package, runs pytest
- More runners: `ubuntu-24.04-arm`, `windows-11-arm`, `macos-15-intel`
  (last Intel macOS), `ubuntu-slim`

> Using `setuptools-scm` or git-based versioning? Add
> `with: fetch-depth: 0` to checkout — no history, no tags, no version.

---

## Keeping actions updated

Anything pinned with `@v7` goes stale. One file gets you a monthly PR:

```yaml
# .github/dependabot.yml
version: 2
updates:
  - package-ecosystem: "github-actions"
    directory: "/"
    schedule:
      interval: "monthly"
    groups:
      actions:
        patterns:
          - "*"
```

`groups` combines everything into a single PR — required for paired actions
like `upload-artifact`/`download-artifact`.

---

## Publishing to PyPI

Two things to upload:

<div class="columns">
<div>

### SDist

The source, always. `.tar.gz` built from your repo.

</div>
<div>

### Wheel

Pre-built, ready to install — make one even for pure Python:

- Installs without running any code
- Faster, safer, no setup tools needed
- Unzip it and see exactly what lands where

</div>
</div>

---

## The trigger

Publishing gets its own workflow — you don't want every PR uploading to PyPI:

```yaml
# .github/workflows/cd.yml
name: CD

on:
  workflow_dispatch:
  release:
    types:
      - published

jobs:
```

- `workflow_dispatch` — a manual "Run workflow" button, perfect for testing
- `release` — fires when you publish a GitHub Release

Building runs on both; the _upload_ step is guarded.

---

## Building the distributions

```yaml
dist:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
      with:
        fetch-depth: 0

    - uses: hynek/build-and-inspect-python-package@v2
```

One action: builds the SDist and wheel, runs a battery of metadata checks,
prints a nice summary, uploads an artifact named `Packages`.

> DIY version: `pipx run build`, then `actions/upload-artifact`, plus
> `pipx run twine check dist/*`.

---

## Trusted Publishing

Tell PyPI "trust artifacts from this repo's `cd.yml` workflow" — PyPI verifies
a short-lived OpenID Connect token that GitHub attaches to the run.

**No API tokens to generate, store, rotate, or leak.**

First-time setup:

1. PyPI → **Publishing** → add a "pending publisher": user/org, repo,
   workflow file (`cd.yml`), environment (`pypi`)
2. GitHub → **Settings → Environments** → create `pypi`
3. Publish a GitHub Release — the workflow builds, then uploads. Done.

---

## The publish job

```yaml
publish:
  needs: [dist]
  environment: pypi
  permissions:
    id-token: write
    attestations: write
  runs-on: ubuntu-latest
  if: github.event_name == 'release' && github.event.action == 'published'
  steps:
    - uses: actions/download-artifact@v8
      with:
        name: Packages
        path: dist
    - name: Generate artifact attestations
      uses: actions/attest-build-provenance@v4
      with:
        subject-path: "dist/*"
    - uses: pypa/gh-action-pypi-publish@release/v1
```

---

## Three guard rails

- `needs: [dist]` — only runs if the build succeeded
- `if: github.event_name == 'release' && ...` — only on a published release,
  so `workflow_dispatch` runs stop after building
- `environment: pypi` — add required reviewers or other protection rules in
  the repo settings

`id-token: write` enables the OIDC handshake; the attestation step is a
verifiable public record that these exact files came from this workflow.

> Try it locally first: `uv build`, then `uvx twine check dist/*`.

---

## What about compiled packages?

Everything here still applies — except one wheel is no longer enough:

<div class="columns">
<div>

### Pure Python

One wheel works everywhere:

`mypkg-1.0-py3-none-any.whl`

</div>
<div>

### Compiled

A wheel _per platform, per architecture_ (and without the stable ABI, per
Python version).

</div>
</div>

That's a build matrix problem — **cibuildwheel** solves it in a few lines of
CI. See the compiled section!

---

## Summary

- **GitHub Actions**: workflows → jobs → steps; lint with prek, test with a
  `uv run pytest` matrix
- **dependabot** keeps your pinned actions fresh
- Always ship an **SDist and a wheel** — even pure Python
- Build with **build-and-inspect-python-package**; publish on GitHub Releases
- **Trusted Publishing** over tokens — no secrets to leak, plus attestations
- Compiled packages: same story + **cibuildwheel** (up next!)
