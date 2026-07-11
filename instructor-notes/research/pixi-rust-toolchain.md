---
type: research
title: The pixi/conda-forge Rust Toolchain (July 2026)
created: 2026-07-10
tags:
  - rust
  - pixi
  - conda-forge
  - toolchain
related:
  - '[[pyo3-current-state]]'
  - '[[maturin-workflows]]'
  - '[[rust-python-landscape]]'
  - '[[rust-pyo3-maturin-deep-dive]]'
---

Research note for the Rust/PyO3/maturin section (`examples/2_04_rust_pyo3/`).
Checked 2026-07-10 against the conda-forge `rust` feedstock, the anaconda.org package
index, the rustup book, conda-forge/conda-build docs, and the pixi documentation -
plus a lot of local ground truth, because the Phase 01 pixi environment on linux-64 *is*
a conda-forge Rust toolchain we can dissect directly. Claims that could not be verified
today are marked "(unverified)".

## What the conda-forge `rust` package ships

conda-forge repackages the official upstream Rust binary distribution; it does not
build rustc from source patches. The feedstock produces several outputs
(source: <https://github.com/conda-forge/rust-feedstock/blob/main/recipe/meta.yaml>):

- **`rust`** - the toolchain itself. Verified from the Phase 01 env's `bin/`:
  `rustc`, `cargo`, `rustdoc`, **`rustfmt`/`cargo-fmt`**, **`clippy-driver`/`cargo-clippy`**,
  and the `rust-gdb`/`rust-gdbgui`/`rust-lldb` debugger wrappers. So the two components
  Pythonistas would `rustup component add` (formatter, linter) are already there.
- **`rust-std-<target>`** - the standard library for one target, split out per
  architecture and pinned **exactly** by `rust` (`pin_subpackage(..., exact=True)`;
  the lockfile shows `rust-std-x86_64-unknown-linux-gnu 1.97.0 h2c6d0dc_0`). Extra
  `rust-std-<target>` packages exist for cross-compilation targets, the conda analogue
  of `rustup target add`.
- **`rust-src`** and **`rust-docs`** - separate optional packages, not installed by
  default (the feedstock builds them only for the `x86_64-unknown-linux-gnu` and
  `x86_64-pc-windows-msvc` targets).

Details observed in the Phase 01 env worth teaching:

- `rustc --print sysroot` returns the **environment prefix itself**
  (`…/.pixi/envs/default`) - the conda env *is* the Rust sysroot; the upstream installer
  manifests (`manifest-rustc`, `manifest-cargo`, `manifest-clippy-preview`,
  `manifest-rustfmt-preview`, `manifest-llvm-tools-preview`, …) sit right in the prefix.
- There is **no `rust-analyzer` LSP binary** in the env - only the proc-macro server
  (`libexec/rust-analyzer-proc-macro-srv`). Attendees who want IDE support should let
  their editor install rust-analyzer; it happily drives the conda-provided toolchain.
- It is a **big download**: the `rust` package alone is ~174 MB compressed on linux-64
  and ~180 MB on osx-arm64 (`size:` fields in `pixi.lock`) - pre-solve and pre-install
  before a live demo; see the teaching runbook in [[rust-pyo3-maturin-deep-dive]].
- Available for linux-64, linux-aarch64, linux-ppc64le, osx-64, osx-arm64, win-64, and
  win-arm64; latest version on all of them today is **1.97.0**, matching the Phase 01
  env exactly. Source: <https://api.anaconda.org/package/conda-forge/rust>.
- conda-forge tracks stable closely: the env's `rustc 1.97.0 (2d8144b78 2026-07-07)`
  was packaged with a linux-64 timestamp of 2026-07-10 (`timestamp:` in `pixi.lock`) -
  days, not months, behind upstream.

## How it differs from rustup

rustup is a *toolchain multiplexer*: it "installs and manages many Rust toolchains and
presents them all through a single set of tools installed to `~/.cargo/bin`", with
stable/beta/nightly channels, `rustup component add`, `rustup target add`, and per-project
overrides via `rust-toolchain.toml`.
Source: <https://rust-lang.github.io/rustup/concepts/index.html>.

The conda-forge model is deliberately different:

| Concern | rustup | conda-forge `rust` via pixi |
| ------- | ------ | --------------------------- |
| Install | `curl \| sh`, global `~/.cargo`/`~/.rustup` | one dependency in `pixi.toml`, per-project env |
| Versions | any channel incl. beta/nightly, switchable | **stable releases only** (package index has no nightly/beta builds) |
| Version selection | `rustup default`/`rust-toolchain.toml` | conda solver + version spec, frozen by `pixi.lock` |
| Components | `rustup component add clippy rustfmt` | clippy + rustfmt already in the package |
| Cross targets | `rustup target add <triple>` | install `rust-std-<triple>` as another conda dep |
| Binaries | proxy shims that dispatch per toolchain | real binaries on the env `PATH`, no shims |
| Co-installed stack | Rust only | same solve also pins Python, maturin, pytest |

Why the workshop teaches the pixi route: it is the same install-and-lock machinery the
book already uses for every other tool, it needs no admin rights or shell-profile
surgery, and one `pixi install` gives every attendee an identical Rust+Python stack.
The honest trade-offs to state: no nightly (irrelevant here - PyO3 targets stable, MSRV
1.83, see [[pyo3-current-state]]), and you get conda-forge's packaging cadence instead
of `rustup update` the moment upstream releases.

## Linker requirements per platform

rustc compiles Rust to objects itself, but final linking of an executable or cdylib is
delegated to a platform linker driver (normally `cc`). Who provides that linker is the
per-platform story - and the practical difference between the three OSes.

### Linux: batteries included (verified in Phase 01)

The linux-64 `rust` package **directly depends on a C toolchain**. From `pixi.lock`:

```yaml
- conda: https://conda.anaconda.org/conda-forge/linux-64/rust-1.97.0-h53717f1_0.conda
  depends:
  - __glibc >=2.17,<3.0.a0
  - gcc_impl_linux-64
  - libgcc >=14
  - libzlib >=1.3.2,<2.0a0
  - rust-std-x86_64-unknown-linux-gnu 1.97.0 h2c6d0dc_0
  - sysroot_linux-64 >=2.17
```

`gcc_impl_linux-64` (15.2.0 in the env) transitively brings `binutils_impl_linux-64`/
`ld_impl_linux-64` 2.45.1, and `sysroot_linux-64` resolved to 2.28. The package also
installs a conda activation script - the env's
`etc/conda/activate.d/rust.sh` contains exactly one line:

```bash
export CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=".../.pixi/envs/default/bin/x86_64-conda-linux-gnu-cc"
```

so cargo links with the env's own gcc wrapper, not whatever `/usr/bin/cc` happens to be.
The feedstock's `install-rust.sh` generates this hook **only on Linux**.
Sources: local env inspection;
<https://github.com/conda-forge/rust-feedstock/blob/main/recipe/install-rust.sh>.

**Observed Phase 01 behavior, worth stating as the headline:** no `compilers` or
`c-compiler` package was ever added on linux-64, and no link step failed - `maturin
develop` worked out of the box. A side effect to be aware of: binaries are linked
against the conda sysroot's glibc (2.28 here), which sets the glibc floor of anything
you build (inference from the dependency data - not separately verified; the wheel
portability story lives in [[maturin-workflows]]).

### macOS: bring Xcode Command Line Tools (not locally verified)

The osx-arm64 `rust` package depends on **nothing but its standard library** - from
`pixi.lock`:

```yaml
- conda: https://conda.anaconda.org/conda-forge/osx-arm64/rust-1.97.0-h4ff7c5d_0.conda
  depends:
  - rust-std-aarch64-apple-darwin 1.97.0 hf6ec828_0
```

No compiler, no activation script (the feedstock hook is Linux-only, above). rustc on
macOS invokes the system `cc` as its linker driver, so attendees need **Xcode Command
Line Tools** (`xcode-select --install`) - which nearly every developer Mac already has.
Phase 01 ran only on linux-64, so this is inference from package metadata plus standard
rustc behavior, not an observed run - verify on an osx-arm64 machine before the workshop
(the example's `pixi.toml` already lists `osx-arm64` in `platforms`, so the lockfile side
is ready).

The macOS SDK caveat mostly concerns conda-forge's *C/C++* compilers, not pure Rust:
Apple's SDK license prevents bundling it in conda packages, so conda-forge `clang`
users must obtain an SDK separately (`CONDA_BUILD_SYSROOT`); conda-forge currently
builds its stack against SDK 11.0. With system Command Line Tools linking a PyO3
cdylib, none of that machinery is needed.
Sources: <https://docs.conda.io/projects/conda-build/en/latest/resources/compiler-tools.html>,
<https://conda-forge.org/docs/maintainer/knowledge_base/>.

### Windows: MSVC Build Tools are on you (unverified locally)

conda-forge's win-64 `rust` targets `x86_64-pc-windows-msvc`, and - like macOS - its
run dependency is only `rust-std`. The MSVC linker (`link.exe`) and Windows import
libraries **cannot be shipped by conda-forge** (proprietary); they come from
"Build Tools for Visual Studio 2022" with the "MSVC v143 … build tools" and
"Windows 11 SDK" components - the same prerequisites rustup documents for its own
msvc toolchain. conda-forge itself moved its default Windows toolchain to VS2022 in
June 2025. Practical workshop guidance: Windows attendees should either have VS Build
Tools preinstalled or use WSL2 and follow the Linux path (where everything is in the
lockfile).
Sources: <https://rust-lang.github.io/rustup/installation/windows-msvc.html>,
<https://conda-forge.org/news/2025/06/11/moving-to-vs2022/>,
<https://github.com/conda-forge/rust-feedstock/blob/main/recipe/meta.yaml>.

### Where `compilers`/`c-compiler` *do* come in

The conda-forge metapackages `c-compiler`/`cxx-compiler`/`compilers` install the
ABI-consistent toolchain conda-forge builds its own packages with (gcc on Linux, clang
on macOS, MSVC activation on Windows). For this section's pure-PyO3 extension they are
unnecessary (verified on Linux, above). You *do* want them the moment the crate graph
grows a C/C++ build step: `*-sys` crates using the `cc` crate, `bindgen` (needs
libclang), `cxx`, or vendored C libraries. Rule of thumb for the deep-dive: "pure Rust +
PyO3 → the `rust` package is enough on Linux/macOS; add `c-compiler` when a C-building
`-sys` crate enters `Cargo.lock`, `cxx-compiler` when the build step is C++, and
libclang alongside the compiler when `bindgen` is in the graph."
Sources: <https://conda-forge.org/docs/user/faq/>,
<https://docs.conda.io/projects/conda-build/en/latest/resources/compiler-tools.html>.

## Reproducibility: pixi.lock for a Rust+Python stack

The example's `pixi.lock` (lockfile `version: 7`) records, for **both** declared
platforms (`linux-64`, `osx-arm64`), every package in the solve - full download URL,
sha256 and md5, and dependency list; 57 package records for this small env, rustc and
CPython pinned side by side in one file. The pixi docs' framing: "A lock file lists the
exact dependencies that were resolved during this resolution process", and installers
"can create exactly the same environment without needing to manage the actual package
contents itself". It is designed to be committed to git. Lockfile v7 additionally locks
build dependencies and avoids churn from unrelated manifest edits.
Sources: <https://pixi.prefix.dev/latest/workspace/lock_file/>,
<https://prefix.dev/blog/lock-file-v7>; local `pixi.lock`.

For CI and classroom use, two flags matter (same source):

- `pixi install --frozen` - "install the environment as defined in the lock file,
  doesn't update `pixi.lock`".
- `pixi install --locked` - "only install if the `pixi.lock` is up-to-date with the
  manifest file" (fail instead of silently re-solving).

The full-stack picture has **two lockfiles**, which is itself a teaching moment:

| Lockfile | Pins | Written by |
| -------- | ---- | ---------- |
| `pixi.lock` | rustc/cargo 1.97.0, maturin 1.14.1, Python 3.14.6, pytest 9.1.1 | pixi (conda solve) |
| `Cargo.lock` | the crate graph - pyo3 0.29.0 and its deps | cargo |

Commit both and a fresh `pixi install && pixi run test` rebuilds the identical
extension anywhere (modulo the non-conda seam: the system linker on macOS/Windows is
outside both lockfiles). The pyo3 pin story is in [[pyo3-current-state]]; how maturin
finds the env (`CONDA_PREFIX`) is in [[maturin-workflows]].

## Pinning strategy

What the example ships (Phase 01 ground truth):

```toml
[dependencies]
rust = ">=1.85"
maturin = ">=1.9"
python = ">=3.12"
pytest = ">=8"
```

The pattern: **manifest floors encode real requirements; the lockfile does the exact
pinning.** Each floor is meaningful - `rust >=1.85` is the minimum for the crate's
`edition = "2024"` (and comfortably above PyO3 0.29's MSRV of 1.83, see
[[pyo3-current-state]]); `maturin >=1.9` predates every feature the example uses;
`python >=3.12` matches the book's baseline. Because `pixi.lock` freezes the solve at
exact builds (rust 1.97.0 `h53717f1_0`, …), tighter manifest specs would add no
reproducibility - they would only fight future `pixi update` runs.

Recommendation for the workshop repo: keep permissive floors, commit `pixi.lock`, and
have instructors provision with `pixi install --locked` so the classroom env is
bit-identical to the one this research was validated against - unlike `--frozen`,
`--locked` aborts when the lockfile has drifted from the manifest instead of silently
installing a stale solve (local `pixi install --help`). Reach for an exact
manifest pin (e.g. `rust = "1.97.*"`) only if you must protect against someone running
`pixi update` the night before the session.

## Observed Phase 01 behaviors (the record)

All from the Phase 01 run on linux-64 (see `Working/phase-01-ground-truth.md`):

- Resolved stack: rust 1.97.0 (`h53717f1_0`) + rust-std-x86_64-unknown-linux-gnu
  1.97.0, maturin 1.14.1, Python 3.14.6 (cp314), pytest 9.1.1 - conda-forge channel only.
- **No linker package needed**: `gcc_impl_linux-64` 15.2.0, `binutils_impl_linux-64`/
  `ld_impl_linux-64` 2.45.1, `sysroot_linux-64` 2.28, and `kernel-headers_linux-64`
  4.18.0 all arrived as dependencies of `rust`; `maturin develop` linked on the first try.
- `CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER` was set by the env activation to the
  conda gcc wrapper (contents of `etc/conda/activate.d/rust.sh`, quoted above).
- maturin auto-detected the pixi env via `CONDA_PREFIX` ("🐍 Found CPython 3.14 at
  …/.pixi/envs/default/bin/python") - no `VIRTUAL_ENV`, no flags; details in
  [[maturin-workflows]].
- `pixi run test`: 8 tests green; `pixi run bench`: 19× speedup over pure Python for
  `count_primes(1_000_000)` on a release build.

## Sources

- Local ground truth: `examples/2_04_rust_pyo3/` - `pixi.lock`, `pixi list`, env
  `bin/`/`libexec/`/activation scripts, `rustc --print sysroot`; `Working/phase-01-ground-truth.md`
- rust-feedstock recipe (outputs, per-platform run deps):
  <https://github.com/conda-forge/rust-feedstock/blob/main/recipe/meta.yaml>
- rust-feedstock install script (Linux-only linker activation hook):
  <https://github.com/conda-forge/rust-feedstock/blob/main/recipe/install-rust.sh>
- conda-forge package index for `rust` (versions, platforms):
  <https://api.anaconda.org/package/conda-forge/rust>
- rustup concepts (toolchains, channels, components, targets):
  <https://rust-lang.github.io/rustup/concepts/index.html>
- rustup MSVC prerequisites: <https://rust-lang.github.io/rustup/installation/windows-msvc.html>
- conda-forge knowledge base (SDK, compilers): <https://conda-forge.org/docs/maintainer/knowledge_base/>
- conda-forge FAQ (compiler metapackages): <https://conda-forge.org/docs/user/faq/>
- conda-build "Anaconda compiler tools" (SDK licensing, activation model):
  <https://docs.conda.io/projects/conda-build/en/latest/resources/compiler-tools.html>
- conda-forge news - VS2022 default: <https://conda-forge.org/news/2025/06/11/moving-to-vs2022/>
- pixi lock file docs (contents, `--frozen`/`--locked`):
  <https://pixi.prefix.dev/latest/workspace/lock_file/>
- pixi lockfile v7 announcement: <https://prefix.dev/blog/lock-file-v7>
