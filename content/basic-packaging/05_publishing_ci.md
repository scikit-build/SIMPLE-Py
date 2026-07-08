# Publishing and CI

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/1_05_publishing_ci>`

You have a package and a set of healthy-development tools around it. This
chapter automates both: run your tests and linters on every pull request with
[GitHub Actions][], then publish to PyPI with one click using [Trusted
Publishing][]. This page covers pure Python packages; compiled packages add one
more tool ([cibuildwheel][]) that we'll meet in the
[compiled section](../compiled/03_publishing_ci.md).

## GitHub Actions in a nutshell

GitHub Actions (GHA) is the standard CI for scientific Python projects: it's
free for open source, tightly integrated with GitHub, and endlessly extensible.
The model is simple:

- A **workflow** is a YAML file in `.github/workflows/` that runs on _events_
  (a PR, a push, a release, a button press).
- A workflow contains **jobs**, which run in parallel on fresh virtual
  machines unless you say otherwise.
- A job is a list of **steps**: either shell commands (`run:`) or reusable
  **actions** (`uses:`).

Every CI workflow starts about the same way:

```{code} yaml
:filename: .github/workflows/ci.yml
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

This runs on every PR and on pushes to `main`. The `concurrency` block is a
freebie worth copying everywhere: pushing a fix to a PR cancels the
already-obsolete run instead of wasting runner time.

### A lint job

You already run your static checks through prek/pre-commit, so CI just needs to
run the same thing. One config, identical results locally and in CI:

::::{tab-set}

:::{tab-item} prek

```yaml
lint:
  name: Lint
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
    - uses: j178/prek-action@v2
```

:::

:::{tab-item} pre-commit

```yaml
lint:
  name: Lint
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
    - uses: actions/setup-python@v6
      with:
        python-version: "3.x"
    - uses: pre-commit/action@v3.0.1
```

Or skip the job entirely and let [pre-commit.ci](https://pre-commit.ci) run it
for you (it can also auto-fix PRs).

:::

::::

### A test matrix

Because your package builds with a standard PEP 517 backend, uses dependency-groups, and tests with
pytest, the test job is nearly copy-paste for _any_ package; only the matrix
is yours:

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

A `matrix` expands into one job per combination (here 3 × 3 = 9), and
`fail-fast: false` keeps one red job from cancelling the others; you want the
full picture. `uv run` does everything: gets the Python, makes the
environment, installs your package, runs pytest. If you have a lockfile committed, it uses that too.

Besides the three `-latest` images, there are ARM runners
(`ubuntu-24.04-arm`, `windows-11-arm`), a final Intel macOS image
(`macos-15-intel`), and a fast single-core `ubuntu-slim`; see the [runner
images][] list.

> [!NOTE]
> If you use `setuptools-scm` or any git-based versioning, add
> `with: fetch-depth: 0` to the checkout step; by default you only get the
> final commit, with no history and no tags to compute a version from.

### Keeping actions updated

Anything pinned with `@v7` goes stale. One file,
`.github/dependabot.yml`, gets you a monthly PR (with changelogs) whenever an
action updates:

```{code} yaml
:filename: .github/dependabot.yml
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

The `groups` key combines everything into a single PR; cleaner, and required
for paired actions like `upload-artifact`/`download-artifact` that must move
together.

:::{exercise} Build a CI workflow
:label: ci-workflow

Give the package you generated from cookie in the last chapter a
`.github/workflows/ci.yml` (pretend it doesn't have one!) with a prek lint job
and a test matrix covering Python 3.10 and 3.14 on Linux, plus 3.14 on
Windows. Bonus: matrices take an `include:` list for adding one-off
combinations.

:::

:::{solution} ci-workflow
:class: dropdown

```{code} yaml
:filename: .github/workflows/ci.yml
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
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v7
      - uses: j178/prek-action@v2

  tests:
    runs-on: ${{ matrix.runs-on }}
    strategy:
      fail-fast: false
      matrix:
        python-version: ["3.10", "3.14"]
        runs-on: [ubuntu-latest]
        include:
          - python-version: "3.14"
            runs-on: windows-latest
    steps:
      - uses: actions/checkout@v7
      - uses: astral-sh/setup-uv@v8
      - run: uv run --python ${{ matrix.python-version }} pytest
```

Now compare with the `ci.yml` cookie actually generated; it should look
familiar.

:::

## Publishing to PyPI

Publishing means uploading two things: an **SDist** (the source, always) and
**wheels** (pre-built, ready to install). For a pure Python package the wheel
is trivial to make, and you should still always make one: wheels install
without running any code (faster, safer, no setup tools needed on the user's
machine), and you can unzip one and see exactly what will land where.

### The trigger

Publishing gets its own workflow with different triggers; you don't want every
PR uploading to PyPI:

```{code} yaml
:filename: .github/workflows/cd.yml
name: CD

on:
  workflow_dispatch:
  release:
    types:
      - published

jobs:
```

`workflow_dispatch` gives you a manual "Run workflow" button, perfect for
testing the build. The `release` trigger fires when you publish a GitHub
Release. Building runs on both; the actual _upload_ step will be guarded so it
only happens on a real release.

### Building the distributions

```yaml
dist:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v7
      with:
        fetch-depth: 0

    - uses: hynek/build-and-inspect-python-package@v2
```

That one action builds the SDist and wheel, runs a battery of checks on the
metadata, prints a nice summary, and uploads the result as an artifact named
`Packages`. (The DIY version is `run: pipx run build` followed by
`actions/upload-artifact`, plus `pipx run twine check dist/*` for the
checks.)

### The publish job

The modern way to authenticate is [Trusted Publishing][]: you tell PyPI "trust
artifacts from this repo's `cd.yml` workflow", and PyPI verifies a short-lived
OpenID Connect token that GitHub attaches to the run. No API tokens to
generate, store, rotate, or leak.

::::{tab-set}

:::{tab-item} Trusted Publishing (recommended)

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

The `id-token: write` permission is what enables the OIDC handshake, and the
attestation step generates a verifiable public record that these exact files
came from this workflow.

:::

:::{tab-item} Token

```yaml
publish:
  needs: [dist]
  runs-on: ubuntu-latest
  if: github.event_name == 'release' && github.event.action == 'published'
  steps:
    - uses: actions/download-artifact@v8
      with:
        name: Packages
        path: dist

    - uses: pypa/gh-action-pypi-publish@release/v1
      with:
        password: ${{ secrets.pypi_password }}
```

If you can't use Trusted Publishing: generate a token on PyPI and store it as
a repository secret. Start with a user-scoped token; once the project exists,
replace it with a project-scoped one.

:::

::::

Note the three guard rails on this job: `needs: [dist]` (only runs if the
build succeeded), the `if:` (only on a published release, so
`workflow_dispatch` runs stop after building), and `environment: pypi` (you
can add required reviewers or other protection rules to that environment in
the repo settings).

To wire up Trusted Publishing the first time:

1. On PyPI, go to your account's **Publishing** page and add a "pending
   publisher": your GitHub user/org, repo name, workflow file name
   (`cd.yml`), and environment (`pypi`).
2. On GitHub, create the `pypi` environment under **Settings → Environments**.
3. Publish a GitHub Release; the workflow builds, then uploads. Done.

> [!TIP]
> While you're in `.github/`, a `release.yml` with a `changelog:` `exclude:`
> block keeps dependabot and pre-commit-ci noise out of GitHub's
> auto-generated release notes. See the [full CI guide][gha_basic] for this
> and more (deploying GitHub Pages, composite actions, a `pass` job for
> auto-merge, ...).

:::{exercise} Dry-run a release
:label: ci-dryrun

You don't need CI (or a PyPI account) to check that your package _would_
publish cleanly. Build the SDist and wheel locally, then run the same metadata
check the upload would perform.

:::

:::{solution} ci-dryrun
:class: dropdown

```bash
uv build
uvx twine check dist/*
```

`uv build` produces `dist/*.tar.gz` (SDist) and `dist/*.whl` (wheel);
`twine check` validates the metadata will render on PyPI. Bonus: `unzip -l
dist/*.whl` shows exactly what a user would get.

:::

:::{exercise} Read a real CD workflow
:label: ci-cookie

Open `.github/workflows/cd.yml` in your cookie-generated package. Find: (a)
what events trigger it, (b) which job builds the distributions, and (c) what
stops a manual run from uploading to PyPI.

:::

:::{solution} ci-cookie
:class: dropdown

\(a\) `workflow_dispatch`, `release: types: [published]`, and (in cookie's
version) PRs, which build but never upload. (b) The `dist` job, using
`hynek/build-and-inspect-python-package`. (c) The `if: github.event_name ==
'release' && github.event.action == 'published'` guard on the publish job;
everything else only builds.

:::

## What about compiled packages?

Everything above still applies, except one wheel is no longer enough: a
compiled package needs a wheel _per platform, per architecture_ (and without
the stable ABI, per Python version). That's a build matrix problem, and
[cibuildwheel][] solves it in a few lines of CI. That's the
[compiled section's publishing chapter](../compiled/03_publishing_ci.md).

[github actions]: https://docs.github.com/en/actions
[trusted publishing]: https://docs.pypi.org/trusted-publishers/
[cibuildwheel]: https://cibuildwheel.pypa.io
[runner images]: https://docs.github.com/en/actions/reference/runners/github-hosted-runners#single-cpu-runners
[gha_basic]: https://learn.scientific-python.org/development/guides/gha-basic/
