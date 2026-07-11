---
type: research
title: maturin - Workflows and Distribution (July 2026)
created: 2026-07-10
tags:
  - rust
  - maturin
  - packaging
  - python-bindings
related:
  - '[[pyo3-current-state]]'
  - '[[pixi-rust-toolchain]]'
  - '[[rust-python-landscape]]'
  - '[[rust-pyo3-maturin-deep-dive]]'
---

Research note for the Rust/PyO3/maturin section (`examples/2_04_rust_pyo3/`).
All claims were checked on 2026-07-10 against maturin.rs, PyPI, and the maturin
GitHub README, or verified locally by running **maturin 1.14.1** inside the Phase 01
pixi environment; anything not re-verifiable today is marked "(unverified)".
maturin's self-description: "Build and publish crates with pyo3, cffi and uniffi
bindings as well as rust binaries as python packages"
(<https://pypi.org/project/maturin/>).

## Version and command surface

- **Latest release: 1.14.1** on PyPI - exactly the version conda-forge resolved into
  the Phase 01 pixi env, so the workshop demos current maturin.
  Sources: <https://pypi.org/pypi/maturin/json>; local `maturin --version`.
- Full command list from local `maturin --help` (1.14.1): `build`, `publish`,
  `list-python`, `develop`, `sdist`, `init`, `new`, `generate-ci`, `upload`,
  `generate-stubs`. `generate-stubs` (autogenerate `.pyi` type stubs) is a recent
  addition worth a mention when attendees ask about typing.

## The four workhorse commands and when each is used

Verified against <https://www.maturin.rs/> and local `--help` output:

- **`maturin new <path>`** - scaffold a fresh project (`maturin init` does the same in
  an existing directory). Key flags: `-b pyo3` selects bindings (`pyo3`, `pyo3-ffi`,
  `cffi`, `uniffi`, `bin`), `--mixed` adds a Python package directory alongside the
  crate, `--src` uses a Python-first `src/` layout for mixed projects. Phase 01 used
  the pure-Rust template. Ground-truth caveat: the maturin 1.14 template **no longer
  emits `features = ["pyo3/extension-module"]`** under `[tool.maturin]` - Phase 01
  restored it by hand (see audit hooks below).
- **`maturin develop`** - compile and install straight into the active environment;
  the inner-loop command. Builds **debug by default**; `--release` opts into optimized
  builds. Useful flags seen in local `--help`: `--uv` ("Use `uv` to install packages
  instead of `pip`"), `--skip-install` ("only build the extension module inplace…
  Only works with mixed Rust/Python project layout"), `-E/--extras` for optional
  dependencies, and `-G/--group` for PEP 735 dependency groups. The README notes the
  trade-off: "while `maturin develop` is faster, it doesn't support all the features
  that running `pip install` after `maturin build` supports."
  Sources: <https://www.maturin.rs/local_development.html>,
  <https://github.com/PyO3/maturin/blob/main/README.md>.
- **`maturin build`** - build wheels into `target/wheels/` without installing or
  uploading. `-i/--interpreter python3.x` selects interpreters ("The python versions
  to build wheels for"), `-F/--features` forwards cargo features, `--sdist` also
  builds a source distribution and "verifies that the source distribution is complete
  and can be used to build the project from source" (local `--help`).
- **`maturin sdist`** - source distribution only, no compilation. An sdist ships the
  crate sources plus `pyproject.toml`; installing from it requires a Rust toolchain on
  the target machine, which is why real projects publish binary wheels and treat the
  sdist as the fallback. Source: <https://www.maturin.rs/distribution.html>.

`maturin publish` (build + upload) and `maturin upload` round out the release story;
both support classic API tokens (`MATURIN_PYPI_TOKEN`) and PyPI **trusted publishing**
via OpenID Connect in CI. Sources: <https://www.maturin.rs/distribution.html>,
<https://www.maturin.rs/environment-variables.html>.

## `--release` semantics

`--release` simply forwards to `cargo build --release` (optimized, no debug
assertions); without it, `maturin develop` produces a debug build and even keeps
debug symbols unless `--strip` is passed. `--profile` selects custom cargo profiles
for finer control. This is the mechanism behind the workshop's two pixi tasks
(`develop` vs `develop-release`) and the "debug builds lose to CPython" teaching
moment - though per the Phase 01 bench, state that warning generally, not for this
u64-division-bound example (debug 0.102 s vs release 0.085 s; see
[[rust-pyo3-maturin-deep-dive]]).
Source: <https://www.maturin.rs/local_development.html>; local ground truth in
`Working/phase-01-ground-truth.md`.

## Environment detection: virtualenvs, conda, and pixi

- maturin discovers the target Python from **`VIRTUAL_ENV`** ("Specifies the path to
  a Python virtual environment") or **`CONDA_PREFIX`** ("Indicates the path to a conda
  environment"). Source: <https://www.maturin.rs/environment-variables.html>.
- **pixi needs no special support:** pixi environments are conda environments, and
  `pixi run` sets `CONDA_PREFIX`. Observed in Phase 01: `maturin develop` printed
  "🐍 Found CPython 3.14 at …/.pixi/envs/default/bin/python" and installed the wheel
  into the pixi env - zero configuration.
- Running `maturin develop` with neither variable set fails with an error asking for
  an activated virtualenv/conda env (unverified exact wording); `--pip-path` exists
  for envs that don't ship their own pip.
- PEP 517 knobs worth knowing for later chapters:
  `MATURIN_PEP517_ARGS` - "Extra arguments passed to `maturin` during PEP 517 builds
  (e.g. `pip install .`)", with pip's `--config-settings` taking priority;
  `MATURIN_PEP517_USE_BASE_PYTHON` to avoid unnecessary rebuilds across venvs; and
  `MATURIN_NO_INSTALL_RUST`, which disables maturin's fallback of **auto-installing a
  Rust toolchain (via puccinialin) when cargo is missing** during a PEP 517 build -
  a surprising default worth mentioning when someone pip-installs an sdist without
  Rust. Source: <https://www.maturin.rs/environment-variables.html>.

## Editable installs (`pip install -e .`) and the book's editable chapter

maturin is a full PEP 517/660 build backend:

```toml
[build-system]
requires = ["maturin>=1.0,<2.0"]
build-backend = "maturin"
```

- With that in place, `pip install -e .` produces an editable install using maturin
  under the hood; `maturin develop` is the equivalent direct command. The guide's
  motivation: "Editable installs can be used with mixed Rust/Python projects so you
  don't have to recompile and reinstall when only Python source code changes."
  Python edits take effect immediately; **Rust edits always require a rebuild**
  (rerun `maturin develop` - or use the import hook below).
  Source: <https://www.maturin.rs/local_development.html>.
- To get an optimized editable install through pip, pass build args via
  `MATURIN_PEP517_ARGS="--release" pip install -e .` (or pip `--config-settings`,
  which takes priority). Source: <https://www.maturin.rs/environment-variables.html>.
- **Relation to the book:** the editable-installs story is already a chapter theme -
  `content/basic-packaging/01_setup.md` introduces editable installs, and
  `content/scikit-build/04_editable_installs.md` covers scikit-build-core's
  `redirect`/`inplace` modes and its `editable.rebuild` auto-rebuild option. The
  Rust section can slot in as "same PEP 660 mechanism, third backend": maturin's
  backend has **no built-in auto-rebuild setting** - the analogue of
  scikit-build-core's `editable.rebuild` is the separate `maturin-import-hook`
  package (next section). That symmetry (backend-specific rebuild ergonomics on top
  of the same standard) is a nice callback for attendees who saw the earlier chapter.

## `maturin-import-hook` - auto-rebuild on import

Source: <https://www.maturin.rs/import_hook.html>;
<https://pypi.org/pypi/maturin-import-hook/json> (latest **0.3.0**, requires
Python ≥3.9).

- Install and activate once per environment:

  ```bash
  pip install maturin_import_hook
  python -m maturin_import_hook site install   # installs into sitecustomize.py
  ```

- Effect: the hook "automatically rebuilds Maturin projects when imported", which
  "reduces friction when developing mixed Python/Rust codebases because edits made to
  Rust components take effect automatically like edits to Python components do", and
  "eliminates the possibility of Python code using outdated rust components, which
  often leads to confusing behaviour".
- Caveats: it only manages packages installed in editable mode ("Only Maturin
  packages installed in editable mode (maturin develop or pip install -e) are
  considered"); it is "intended for use in development environments and not for
  production environments"; disable per-run with `MATURIN_IMPORT_HOOK_ENABLED=0`.
- Workshop stance: demo `maturin develop` explicitly (the rebuild step *is* the
  lesson), then name-drop the hook as the quality-of-life upgrade for daily work.

## Mixed Rust/Python layouts (`python-source`, `module-name`)

Source: <https://www.maturin.rs/project_layout.html>.

- **Pure Rust** (the Phase 01 example): just `Cargo.toml` + `pyproject.toml` +
  `src/lib.rs`; maturin generates the package so `import pyo3_example` works
  directly.
- **Mixed:** add a directory named after the package next to `src/`; the compiled
  module then sits inside the Python package and must be re-exported explicitly
  (`from my_project import my_project`).
- **`python-source`:** relocate Python code, e.g. `python-source = "python"` under
  `[tool.maturin]`, giving the common `python/my_project/…` + `src/…` split.
- **`module-name`:** rename the compiled module into a private submodule -
  `module-name = "my_project._my_project"` - the widely used "private native core,
  public Python API" convention (pydantic-core-style; see
  [[rust-python-landscape]]). The `#[pymodule]` name in `lib.rs` must match the
  last path segment.
- Teaching recommendation: start pure-Rust (least moving parts), show the mixed
  layout on a slide as "what real projects grow into".

## `maturin generate-ci`

- `maturin generate-ci github` emits a complete GitHub Actions workflow (GitHub is
  the only provider listed in local `--help`). Platform selection now lives in
  `pyproject.toml` under `[tool.maturin.generate-ci.github."PLATFORM NAME"]`; the
  deprecated `--platform` flag enumerates the choices: `all`, `manylinux`,
  `musllinux`, `windows`, `macos`, `emscripten` (local `--help`; the guide also
  mentions Android). Options cover per-platform builds, optional pytest runs, zig,
  and artifact attestation. The release job **defaults to API-token publishing**:
  the generated workflow runs `uv publish` with
  `UV_PUBLISH_TOKEN: ${{ secrets.PYPI_API_TOKEN }}` (verified locally against
  maturin 1.14.1 output; the `id-token: write` permission it requests is for
  attestation signing, not OIDC upload). **PyPI trusted publishing is opt-in** via
  `[tool.maturin.generate-ci.github] trusted-publishing = true`, which switches the
  job to `uv publish --trusted-publishing always` and drops the token.
  Sources: local `maturin generate-ci github` output;
  <https://www.maturin.rs/distribution.html>.
- Positioning vs cibuildwheel: `generate-ci` is maturin-native and Rust-aware out of
  the box; cibuildwheel (which the book's C++ chapters reference) also supports Rust
  extensions but needs the toolchain provisioned per build image (unverified detail
  of current cibuildwheel Rust ergonomics). For a maturin-only project,
  `generate-ci` is the lower-friction default.

## manylinux/musllinux wheels and the container images

Source: <https://www.maturin.rs/distribution.html>;
<https://github.com/PyO3/maturin/blob/main/README.md>.

- Linux wheels for PyPI must be **manylinux** or **musllinux** tagged. Because "the
  Rust compiler requires at least glibc 2.17", **manylinux2014 (`manylinux_2_17`) is
  the practical floor** for Rust wheels.
- maturin "contains a reimplementation of auditwheel" that "automatically checks the
  generated library and gives the wheel the proper platform tag" - no separate
  auditwheel pass needed; `--auditwheel repair` can also bundle external shared
  libraries into the wheel (requires patchelf). `--compatibility` pins a specific
  policy (e.g. `manylinux_2_28`) or opts into plain `linux` tags.
- **Official container image:** builds run in a manylinux2014-based image with
  toolchains preinstalled:

  ```bash
  docker run --rm -v $(pwd):/io ghcr.io/pyo3/maturin build --release
  ```

- **Ground-truth teaching hook:** the wheel maturin built inside the Phase 01 pixi
  env was tagged `pyo3_example-0.1.0-cp314-cp314-linux_x86_64.whl` - a plain
  `linux_x86_64` tag, **not** manylinux (a conda env's glibc/linked libs don't
  satisfy the policy, and `maturin develop` doesn't need it to). Perfect for
  contrasting "local dev install" vs "publishable wheel": PyPI would reject this
  tag; CI or the container image produces the manylinux one.

## Cross-compilation (including zig)

Sources: <https://www.maturin.rs/distribution.html>; local `maturin build --help`;
<https://www.maturin.rs/environment-variables.html>.

- Three supported routes: (1) **manylinux-cross Docker images** for Linux targets;
  (2) **zig** - `maturin build --zig --target <triple>` uses zig as the C
  compiler/linker to "ensure compliance for the chosen manylinux version"
  (defaulting to manylinux2014/`manylinux_2_17`), giving glibc-version-pinned
  cross-builds from any host; (3) **cargo-xwin** integration for Windows MSVC
  targets, with automatic CRT/SDK header and library download.
- Cross-interpreter configuration flows through PyO3's env vars
  (`PYO3_CROSS_PYTHON_VERSION`, `PYO3_CROSS_LIB_DIR`, `PYO3_CONFIG_FILE`), which
  maturin sets or honors.
- Workshop scope: out of scope for the 30-minute segment; keep as a Q&A answer
  ("yes, one Linux box can emit wheels for other platforms - maturin + zig or
  Docker").

## abi3 wheels

Sources: <https://www.maturin.rs/bindings.html>; see [[pyo3-current-state]] for the
PyO3 side (feature flags, limited-API costs, `abi3t`).

- maturin auto-detects PyO3's `abi3`/`abi3-py3X` cargo features from `Cargo.toml`
  and tags the wheel accordingly (e.g. `cp310-abi3-manylinux_2_17_x86_64` for an
  `abi3-py310` build): **one wheel per platform covers every CPython ≥ the floor**,
  instead of one wheel per Python version.
- New in the 0.29-era stack: PEP 803 free-threaded stable ABI. "A single maturin
  build selects one stable ABI family. If you want to publish both a GIL-enabled
  `abi3` wheel and an `abi3t` wheel, run separate wheel builds explicitly, with
  compatible interpreters" - e.g. `maturin build -i python3.10` and
  `maturin build -i python3.15t`.
- The Phase 01 example intentionally builds version-specific (no abi3 feature) -
  hence the `cp314-cp314` tag; abi3 stays a distribution-discussion hook, matching
  the decision recorded in [[pyo3-current-state]].

## Comparison: maturin vs setuptools-rust vs when scikit-build-core wins

- **maturin** - purpose-built for Rust: its own PEP 517 backend, near-zero config,
  built-in manylinux auditing, `develop` inner loop, `generate-ci`, publishing.
  The right default for new PyO3 projects, and what this workshop teaches.
- **setuptools-rust** (<https://setuptools-rust.readthedocs.io/en/latest/>) - a
  setuptools plugin: "Compile and distribute Python extensions written in Rust as
  easily as if they were written in C." Choose it when a project is **already on
  setuptools** (existing `setup.py` machinery, other setuptools plugins, custom
  build steps) and Rust is being added incrementally, or when one package must
  bundle **multiple Rust extension modules** - its docs note "If you require
  multiple extension modules you will need to write multiple `Cargo.toml` files"
  (or expose PyO3 submodules from one crate), a shape maturin's one-crate-per-wheel
  model doesn't target. Wheel building is delegated to external tooling
  (typically cibuildwheel) rather than built in.
- **scikit-build-core** (<https://scikit-build-core.readthedocs.io/en/latest/>) -
  "the build backend for making Python modules with CMake", aimed at C, C++, and
  Fortran. It **remains the better fit whenever CMake is the source of truth**:
  existing C/C++ codebases, mixed C++-plus-Rust builds, or teams standardized on
  CMake - exactly the territory of the book's `content/scikit-build/` chapters.
  maturin does not drive CMake; scikit-build-core does not know about cargo. Rule
  of thumb for the slide: *new pure-Rust extension → maturin; legacy setuptools +
  Rust bolt-on → setuptools-rust; CMake/C/C++ world → scikit-build-core.*

## Implications for the Phase 01 example (audit hooks)

- The pixi tasks already model the canonical workflow: `develop` (debug inner
  loop), `develop-release` (bench-worthy builds), `test` depends-on `develop`,
  `bench` depends-on `develop-release`. ✓ Matches current guidance; nothing to
  change.
- `[tool.maturin] features = ["pyo3/extension-module"]` (hand-restored in Phase 01):
  harmless but redundant with this stack - maturin sets
  `PYO3_BUILD_EXTENSION_MODULE=1` when building, and PyO3 0.26+ honors that env var
  with the same don't-link-libpython effect as the feature (PyO3 0.26.0 changelog;
  maturin `src/compile.rs`; maturin's tutorial now scopes the feature to "pyo3 0.26
  or earlier"). The 1.14 template's omission is therefore deliberate, not an
  oversight. Keep the feature only for older PyO3 or for plain `cargo build` outside
  maturin.
- pixi pin `maturin = ">=1.9"` resolves to 1.14.1 today - floor is fine; no action.
- The `linux_x86_64` wheel-tag observation above should feed the deep-dive's
  distribution section rather than change the example.
- No deprecated maturin usage found in the example; commands and flags all match
  1.14.1 help output.

## Sources

- maturin user guide (overview/commands): <https://www.maturin.rs/>
- Local development, develop, editable installs: <https://www.maturin.rs/local_development.html>
- Import hook guide: <https://www.maturin.rs/import_hook.html>
- Project layouts (`python-source`, `module-name`): <https://www.maturin.rs/project_layout.html>
- Distribution (manylinux, Docker, zig, generate-ci, publish): <https://www.maturin.rs/distribution.html>
- Bindings incl. abi3/abi3t wheel selection: <https://www.maturin.rs/bindings.html>
- Environment variables (`VIRTUAL_ENV`, `CONDA_PREFIX`, PEP 517 args): <https://www.maturin.rs/environment-variables.html>
- README (Docker image, auditwheel reimplementation): <https://github.com/PyO3/maturin/blob/main/README.md>
- PyPI metadata: <https://pypi.org/pypi/maturin/json>, <https://pypi.org/pypi/maturin-import-hook/json>
- setuptools-rust docs: <https://setuptools-rust.readthedocs.io/en/latest/>
- scikit-build-core docs: <https://scikit-build-core.readthedocs.io/en/latest/>
- Local ground truth: `maturin 1.14.1 --help` output in `examples/2_04_rust_pyo3/`;
  `Working/phase-01-ground-truth.md`; book chapters
  `content/basic-packaging/01_setup.md`, `content/scikit-build/04_editable_installs.md`
