# uv vs. Pixi: A showdown

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/1_02_tooling>`

As mentioned in [Setting up for development](https://scikit-build.org/SIMPLE-Py/setup/), two tools were mentioned as being important to have installed: `uv` and Pixi.
What tool should you use and when?
The answers mainly come down to what are your goals and what are your constraints.
To start with though, let's compare the tools feature sets for their high-level interfaces and note their similarity in design.
A more complete list is [given on the Pixi documentation website](https://pixi.prefix.dev/latest/switching_from/uv/#quick-look-at-the-differences).

| Feature                                           | uv                                                         | Pixi                                                     |
| ------------------------------------------------- | ---------------------------------------------------------- | -------------------------------------------------------- |
| Installs Python packages                          | ✅                                                         | ✅ (via uv's Rust libraries)                             |
| Installs conda packages                           | ❌                                                         | ✅                                                       |
| Installs Python                                   | ✅                                                         | ✅                                                       |
| Environment format                                | Python virtual environments (`.venv`)                      | conda environments (`.pixi/envs`)                        |
| Activate environments in the shell                | ✅ (`. .venv/bin/activate`)                                | ✅ (`pixi shell`)                                        |
| Supports lockfiles                                | ✅ (`uv.lock`, TOML)                                       | ✅ (`pixi.lock`, YAML)                                   |
| Automatically produce lockfiles                   | ✅ (`uv run`, `uv sync`)                                   | ✅ (all operations that touch the workspace or lockfile) |
| Resolve environment to a lockfile without install during development | ❌                                            |  ❌[^1]                                     |
| Low-level commands                                | ✅ (`uv pip install`)                                      | ❌                                                       |
| Fully disable lockfile operations                 | ❌ (partial: `--frozen`, `--no-sync`, or drop to `uv pip`) | ❌ (partial: `--frozen`, `--locked`)                     |
| Declarative dependency specification              | ✅ (`uv add`)                                              | ✅ (`pixi add`)                                          |
| Globally install tools in isolated envs           | ✅ (`uv tool install`)                                     | ✅ (`pixi global`)                                       |
| Run tools in temporary envs                       | ✅ (`uvx`)                                                 | ✅ (`pixi exec`)                                         |
| Task runner system                                | ❌                                                         | ✅                                                       |
| Native multi-environment support                  | ❌ (delegated to `nox` or `tox`)                           | ✅ (arbitrary environments)                              |
| Build packages                                    | ✅ (sdists and wheels via `uv build`)                      | ✅ (conda packages via Pixi Build)                       |
| Publish packages                                  | ✅ (to PyPI via `uv publish`)                              | ✅ (to conda channels via `pixi upload`)                 |
| Fast?                                             | Fastest!                                                   | Fast!                                                    |

While both tools support lockfiles, they mean different things by it.
uv's `uv.lock` lockfiles are a single **universal** resolution recorded with environment markers, that is valid on all platforms.
Pixi's `pixi.lock` lockfiles record a **separate solve for each platform** that the workspace declares.
Neither format is the standardized lockfile format ([PEP 751](https://peps.python.org/pep-0751/)'s `pylock.toml`), though `uv` can export one with `uv export --format pylock.toml`.

## When to use what tool

Importantly `uv` and Pixi are tools for software environment resolution, installation, and management.
They do not preclude use of any of the tools that we use for **building** Python packages (e.g. `hatchling`, `scikit-build-core`, `setuptools`).[^2]
So the choice comes down much more to what sort of project you're working on and what kind of dependencies it requires for building, running, and testing.

### uv

If **everything your project needs, including at build time, is a Python package** then `uv` is a reasonable default.
This covers more than the pure-Python case: build tools like CMake and Ninja are distributed as wheels, so a compiled-extension project using `scikit-build-core` can still be developed with `uv` as long as a suitable compiler exists on the system.
It uses "traditional" Python virtual environments, with development dependencies organized into [PEP 735](https://peps.python.org/pep-0735/) dependency groups, so the standard mechanics of virtual environment workflows are retained.
As `uv` also only operates on and with Python packages it also can focus on the Python ecosystem and optimizations that can be made for Python packages and Python package index design that are not generalizable.
It is also blazingly fast and as of 2026 still is the fastest Python dependency manager that exists.

### Pixi

If you are working with Python projects that **depend on things that do not exist as Python packages**, like a compiler stack or large non-Python dependencies (e.g. your project is Python bindings to a large C++ framework like Geant4), then Pixi's strengths of being a multi-platform software environment manager with access to conda packages start to stand out more.
With Pixi and Pixi Build you have the ability to ensure consistency of compiler stacks used across time.
Pixi also supports **multiple named environments in one workspace**, composed from "features", so a test matrix across Python versions, or an optional CUDA environment, can coexist side by side without an external tool like `nox` or `tox`.
One cost is that because `pixi.lock` records a separate solve for every platform the workspace declares, lock files are large and resolution takes longer with each platform added.
Another cost appears during development: resolving a PyPI **source** dependency with dynamic metadata — which usually includes the package under development itself, present as an editable install — requires running its build backend with the environment's Python interpreter, so the conda environment must be installed just to produce the lock file.[^1]

### Developing a package for distribution

The tools also differ in how the development environment relates to the package you eventually ship.

- **Dependency metadata:** `uv add` writes to `[project.dependencies]` in `pyproject.toml` (the same metadata that ends up in your published sdist and wheel) so the development environment and the package's declared dependencies can not drift apart.
  Pixi's conda dependencies live in `[tool.pixi.dependencies]`, which does not propagate into package metadata, so a package distributed on PyPI needs its runtime dependencies specified twice: once as conda packages for development and once under `[project]` for distribution.
- **Editable installs:** `uv sync` installs the project itself in editable mode by default.
  Pixi requires declaring this explicitly:

  ```toml
  [tool.pixi.pypi-dependencies]
  your-package = { path = ".", editable = true }
  ```

- **Build artifacts:** `uv build` produces sdists and wheels and `uv publish` uploads them to PyPI, while Pixi Build produces **conda packages** for conda channels.

### Using them together

The choice is not exclusive.
Pixi already uses uv's resolver internally for its PyPI dependencies, and some projects ship both manifests so that contributors can use whichever tool fits their setup.
In continuous integration both tools have first-party GitHub Actions with built-in caching keyed on the lockfile.

- [`astral-sh/setup-uv`](https://github.com/astral-sh/setup-uv)
- [`prefix-dev/setup-pixi`](https://github.com/prefix-dev/setup-pixi)

## Pixi Exercises

::::{exercise} Basic Pixi environment
:label: basic-pixi-env

Use Pixi to create a Pixi workspace.
Install the `cowsay` package from PyPI and then run `cowsay`.
Remove the workspace when done.

:::{solution} basic-pixi-env
:class: dropdown

```bash
pixi init cowsay-example
cd cowsay-example
pixi add python
pixi add --pypi cowsay
pixi run cowsay -t SIMPLE-Py
cd ..
rm -rf ./cowsay-example
```

:::

::::

::::{exercise} Tool install
:label: pixi-tool-install

Use `pixi global` to install `pixi-browse`.
Run it.
Remove the tool when done.

:::{solution} pixi-tool-install
:class: dropdown

```bash
pixi global install pixi-browse
pixi browse -m boost-histogram
pixi global uninstall pixi-browse
```

:::

::::

::::{exercise} Development environment
:label: pixi-dev-env

The most downloaded Python package in the world is probably `packaging` (not counting AWS).
Download the `https://github.com/pypa/packaging` repository from GitHub, and run the test suite with `pytest` using Pixi.

:::{solution} pixi-dev-env
:class: dropdown

Assuming that you have nothing installed on a new machine beyond Pixi:

```bash
pixi global install git
git clone https://github.com/pypa/packaging
cd packaging
pixi init --format pyproject
pixi run --environment test pytest
```

:::

::::

[^1]: PyPI dependencies resolve without installation (`pixi lock --no-install`) only when their metadata can be read statically: wheels, or source dependencies with fully static `pyproject.toml` metadata. With dynamic metadata (e.g. `dynamic = ["version"]`, common for packages built with `setuptools_scm`, `hatch-vcs`, or `scikit-build-core`) `pixi lock` must install the conda environment to run the build backend, and `pixi lock --no-install` fails with `installation of conda environment is required to solve PyPI source dependencies`.
[^2]: Though `uv` does have its own [PEP 517](https://peps.python.org/pep-0517/) compliant build backend, `uv_build`, for pure-Python projects.
