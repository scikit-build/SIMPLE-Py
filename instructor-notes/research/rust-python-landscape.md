---
type: research
title: The Rust-for-Pythonistas Landscape (July 2026)
created: 2026-07-10
tags:
  - rust
  - python
  - ecosystem
  - pyo3
  - motivation
related:
  - '[[pyo3-current-state]]'
  - '[[maturin-workflows]]'
  - '[[pixi-rust-toolchain]]'
  - '[[rust-pyo3-maturin-deep-dive]]'
---

Research note for the Rust/PyO3/maturin section (`examples/2_04_rust_pyo3/`). This is the
*motivational* note: proof that attendees already run Rust every day, where the
Rust-extension approach genuinely pays off, where it does not, and what other
tutorials/workshops exist so this session can differentiate. Claims were checked on
2026-07-10 against each project's repository (`pyproject.toml`/`Cargo.toml` on the default
branch), PyPI, and crates.io; anything not re-verifiable today is marked "(unverified)".
Download figures are pepy.tech month badges read on 2026-07-10 - treat as order of
magnitude, not precision.

## Rust tools Pythonistas already use daily

The single best motivational slide: a typical 2026 Python stack already executes Rust on
every run - usually without the user ever installing a Rust compiler. One line each:

| Package | Downloads/mo | What it is | How it ships Rust |
| ------- | ------------ | ---------- | ----------------- |
| `cryptography` | ~1 billion | TLS/crypto primitives | PyO3 extension, built with maturin |
| `pydantic-core` | ~863 M | validation core of pydantic v2 (and thus FastAPI) | PyO3 extension (`bindings = 'pyo3'`), maturin |
| `ruff` | ~234 M | linter + formatter | pure Rust binary in a wheel (maturin `bindings = "bin"`, no PyO3) |
| `tokenizers` | ~187 M | Hugging Face tokenizers | PyO3 extension, maturin, abi3 |
| `orjson` | ~176 M | fast JSON | Rust via low-level `pyo3-ffi`, maturin |
| `uv` | ~139 M | package/project manager | pure Rust binary in a wheel (maturin `bindings = "bin"`) |
| `polars` | ~52 M | DataFrame library | pure-Python meta package over PyO3-built `polars-runtime-*` abi3 wheels |

Sources: monthly badges at `https://static.pepy.tech/badge/<package>/month`
(project pages: <https://pepy.tech/project/cryptography> etc.); per-project build facts
below. The `uv` figure understates real usage - most installs come from its standalone
installer, Homebrew, or CI caches, not PyPI (analysis, not a sourced count).

Per-tool detail, verified against each repo's default branch on 2026-07-10:

- **ruff** (Astral): no PyO3 at all - `pyproject.toml` sets `build-backend = "maturin"`
  with `bindings = "bin"`, `manifest-path = "crates/ruff/Cargo.toml"`, and
  `python-source = "python"`. maturin packages a standalone executable into wheels; the
  wheel's job is distribution, not binding. Fun teaching detail: `requires-python =
  ">=3.7"` - a binary that never links libpython doesn't care which Python installs it.
  Source: <https://github.com/astral-sh/ruff/blob/main/pyproject.toml>.
- **uv** (Astral): same pattern - maturin backend, `bindings = "bin"`,
  `manifest-path = "crates/uv/Cargo.toml"`, plus a tiny `python-source = "python"` shim
  package. Source: <https://github.com/astral-sh/uv/blob/main/pyproject.toml>.
- **polars**: since the 1.x runtime split, the PyPI `polars` package (1.42.1 today) is
  **pure Python** (`build-backend = "setuptools.build_meta"`) and depends on
  `polars-runtime-32 == <same version>`, with `polars-runtime-64` and
  `polars-runtime-compat` as opt-in extras; a loader picks the best runtime for the
  host's CPU flags at import time. The compiled runtimes come from the `polars-python`
  crate, which depends on `pyo3` with `features = ["abi3-py310", …]` - the runtime wheels
  on PyPI are tagged `cp310-abi3`. (Build backend of the runtime wheels themselves not
  verified; the bindings crate is unambiguously PyO3.)
  Sources: <https://github.com/pola-rs/polars/blob/main/py-polars/pyproject.toml>,
  <https://github.com/pola-rs/polars/blob/main/crates/polars-python/Cargo.toml>,
  <https://pypi.org/project/polars-runtime-32/>,
  <https://docs.pola.rs/user-guide/installation/>.
- **pydantic-core**: the canonical PyO3 mixed-layout project - maturin backend
  (`maturin>=1.9.4,<2`), `bindings = 'pyo3'`, `python-source = "python"`,
  `module-name = "pydantic_core._pydantic_core"`. Every pydantic v2 model validation
  (so: every FastAPI request) runs this Rust code.
  Source: <https://github.com/pydantic/pydantic-core/blob/main/pyproject.toml>.
- **orjson**: maturin backend, but deliberately **not** the high-level PyO3 API - it
  depends on `pyo3-ffi = { version = "0.28", default-features = false }` (raw C-API
  bindings, edition 2024, `rust-version = "1.95"`) to shave every last conversion cost.
  Good "you can drop a level when you must, but you lose the safety rails" data point.
  Sources: <https://github.com/ijl/orjson/blob/master/Cargo.toml>,
  <https://github.com/ijl/orjson/blob/master/pyproject.toml>.
- **cryptography** (pyca): maturin backend today - and a demanding user of it
  (`maturin>=1.14.1,<2` on new Pythons), mixed layout with
  `module-name = "cryptography.hazmat.bindings._rust"` and `locked = true`. History for
  the "Rust arrives quietly" narrative: Rust entered the build in the 3.4 era (early
  2021) and became mandatory in 35.0.0 (2021-09-29: "Rust is now required for building
  cryptography"); it originally built with setuptools-rust and later switched the
  backend to maturin - community records place the switch in late 2023/early 2024, but
  the exact release is not stated in the changelog (unverified).
  Sources: <https://github.com/pyca/cryptography/blob/main/pyproject.toml>,
  <https://cryptography.io/en/latest/changelog/>.
- **tokenizers** (Hugging Face): maturin backend, `bindings = "pyo3"`,
  `features = ["pyo3/extension-module", "abi3"]`, mixed layout
  (`python-source = "py_src"`, `module-name = "tokenizers.tokenizers"`). A production
  example of the abi3 one-wheel-per-platform strategy discussed in
  [[maturin-workflows]].
  Source: <https://github.com/huggingface/tokenizers/blob/main/bindings/python/pyproject.toml>.

### Two integration patterns - name them explicitly

The table above splits cleanly into two shapes, and naming the split avoids a common
attendee confusion ("is ruff a Python extension?"):

1. **PyO3 extension modules** (pydantic-core, cryptography, tokenizers, polars runtimes,
   orjson via pyo3-ffi): Rust compiled to a `cdylib` that CPython imports; Python calls
   into Rust in-process. This is what the workshop example builds.
2. **Rust binaries delivered as wheels** (ruff, uv): maturin's `bin` bindings put a
   normal executable on `PATH` via pip/uv/pixi. No PyO3, no libpython, works with any
   Python. The wheel is used purely as a package-manager-friendly delivery vehicle.

Either way, **maturin is the common denominator** - every row of the table builds with
it except the pure-Python polars shim. That is the strongest available argument that the
tool this section teaches is the ecosystem default, not a niche choice.

### Honorable mentions (one breath, not a slide)

Also Rust-backed and widely deployed: `jiter` (the JSON parser inside pydantic and the
OpenAI/Anthropic SDKs), `watchfiles` (file watching used by uvicorn's reloader),
`rpds-py` (persistent data structures under `jsonschema`), `granian` (Rust HTTP server
for Python apps), and Astral's `ty` type checker (unverified - listed from ecosystem
familiarity; spot-check any of these before quoting details in the session).

## NumPy interop: rust-numpy and ndarray

For numeric attendees the natural question is "what about arrays?" - the answer is the
`numpy` crate (rust-numpy), which is PyO3-based bindings to NumPy's C-API:

- **Current release 0.29.0 (2026-06-13), tracking pyo3 0.29** - the crate versions its
  minors in lockstep with PyO3 (0.28 ↔ pyo3 0.28, etc.), so the pairing for the example
  would be `pyo3 = { version = "0.29" }` + `numpy = "0.29"`. MSRV 1.83, same as PyO3.
  Sources: <https://crates.io/api/v1/crates/numpy>,
  <https://github.com/PyO3/rust-numpy>.
- It exposes `PyArray<T, D>` (a NumPy array as a Python object) plus safe read/write
  guards (`PyReadonlyArray` etc.), and converts to/from the pure-Rust `ndarray` crate,
  which it re-exports; it tolerates a range of ndarray versions (currently
  `>= 0.15, <= 0.17`). Zero-copy views mean Rust can loop over the same buffer NumPy
  sees - no per-element boundary crossing.
  Source: <https://github.com/PyO3/rust-numpy> (README).
- Teaching placement: the workshop example stays scalar (`count_primes`, `Point`) on
  purpose; rust-numpy is the "where to go next" pointer for the "my hot loop is over a
  NumPy array" question, covered in depth in [[rust-pyo3-maturin-deep-dive]].

## Where Rust does *not* help

The section should spend two honest minutes here - it buys credibility for the 19×
benchmark (local ground truth: pure Python ~1.63 s vs Rust release ~0.085 s for
`count_primes(1_000_000)`; see `Working/phase-01-ground-truth.md`). The following are
engineering analysis rather than sourced claims, backed by local measurements where
noted:

- **I/O-bound code.** If the profile is dominated by network, disk, or database waits,
  compiled code changes nothing - the CPU was already idle. Concurrency tools
  (`asyncio`, threads - increasingly viable on free-threaded CPython, see
  [[pyo3-current-state]]) attack waiting; Rust attacks computing.
- **Already-vectorized NumPy.** `a * b + c` on large arrays already runs C loops (or
  BLAS) over contiguous buffers; a hand-written Rust loop typically matches rather than
  beats it, and a *naive* per-element PyO3 function called from Python is dramatically
  slower than either. Rust wins over NumPy only where vectorization fails: per-element
  branching, stateful scans, custom reductions - and then via rust-numpy whole-array
  calls, never per-element calls.
- **Chatty boundaries.** Every Python→Rust call pays FFI + conversion overhead
  (arguments in, results out - see the conversion-traits section of
  [[pyo3-current-state]]). A tiny function invoked millions of times from a Python loop
  can end up *slower* than pure Python. Design rule for the slides: move the **loop**
  into Rust, not the loop **body**. Passing big `list`/`dict` structures converts
  element-by-element; prefer buffers/arrays at the boundary.
- **Debug builds.** `maturin develop` compiles unoptimized by default; the section's
  workflow encodes the fix (`develop-release` task → `maturin develop --release`).
  Ground-truth caveat: on this u64-division-bound example debug was only ~1.2× slower
  (0.102 s vs 0.085 s), so state the "debug builds can lose to CPython" warning
  generally, not as a claim about this example. See [[maturin-workflows]].
- **The bottleneck is the algorithm.** An O(n²) → O(n log n) fix in Python routinely
  beats a constant-factor 19× from Rust. Profile first (`cProfile`, `py-spy`), rewrite
  second.
- **Team cost is real.** A second toolchain, compile times, a wheel-building CI matrix
  (mitigated by `maturin generate-ci` - [[maturin-workflows]]), and a smaller
  contributor pool. The ecosystem answer is visible in the table above: push Rust into
  *narrow, hot, well-tested cores* (pydantic-core, polars runtimes) and keep the API
  surface in Python.

## Existing PyO3 tutorials and workshops - and how this session differs

What is already out there (all links checked 2026-07-10):

- **The official PyO3 user guide** (<https://pyo3.rs/>) - comprehensive and current;
  the getting-started chapter is effectively the canonical tutorial. Assumes the reader
  installs Rust themselves (rustup) and manages a virtualenv.
- **The maturin tutorial** (<https://www.maturin.rs/tutorial.html>) - builds a
  guessing-game lib crate and installs it with `maturin develop`; the closest published
  analogue to this section's workflow.
- **"PyO3 101 - Writing Python modules in Rust" (Cheuk Ting Ho)** - the most visible
  conference workshop: a ~3-hour hands-on tutorial run at PyCon US 2024, PyCon DE &
  PyData Berlin 2024 (recording on YouTube), and EuroPython 2025. Seven exercises from
  hello-world through iterators and thread-safe decorators; materials MIT-licensed at
  <https://github.com/Cheukting/py03_101>. Setup asks attendees to install Rust via
  rustup, create a venv with uv, `uv pip install maturin`, and even run
  `python -m ensurepip` so `maturin develop` can find pip.
  Sources: <https://us.pycon.org/2024/schedule/presentation/113/>,
  <https://2024.pycon.de/program/8C83EA/>,
  <https://ep2025.europython.eu/session/writing-python-modules-in-rust-pyo3-101/>.
- **PyO3's in-repo examples** (<https://github.com/PyO3/pyo3/tree/main/examples>) -
  small complete projects (word-count etc.) useful for cribbing patterns.
- **Blog posts and Medium tutorials** - abundant but rot quickly: PyO3 ships a breaking
  minor roughly quarterly, and anything predating 0.21 (2024-03) or the 0.26 (2025-08)
  `attach`/`detach` renames shows APIs that no longer exist. The rot mechanics are
  documented in [[pyo3-current-state]]; teach attendees to check dates before trusting
  snippets.

Differentiation of this session (the honest pitch, not marketing):

1. **pixi-managed toolchain, zero rustup.** Every published tutorial above starts with
   "install Rust" as a separate, per-OS step. Here, `pixi install` provisions rust
   1.97.0, maturin 1.14.1, Python 3.14.6, and pytest from conda-forge in one lockfile -
   and on linux-64 the linker comes along transitively, no `compilers` package needed
   ([[pixi-rust-toolchain]], ground truth). maturin then auto-detects the pixi env via
   `CONDA_PREFIX`. Setup friction is the thing that kills 3-hour workshops; this
   session inherits an environment attendees already built in earlier chapters.
2. **Packaging-workshop context.** Attendees arrive knowing PEP 517 backends, editable
   installs, and scikit-build-core from earlier sections - so maturin is taught as
   *another build backend* with a familiar shape (`pyproject.toml`, `pip install -e .`,
   wheels), and the maturin vs setuptools-rust vs scikit-build-core comparison
   ([[maturin-workflows]]) lands with an audience primed to care.
3. **30-minute segment, one artifact.** Not a Rust course: one crate (`count_primes` +
   a `#[pyclass]`), one benchmark payoff, plus pointers out (rust-numpy, abi3,
   generate-ci) for depth - versus the half-day scope of PyO3 101.
4. **Current APIs.** The example is pyo3 0.29 / edition 2024 / declarative modules /
   `Python::detach` - ahead of most published material, per the cadence table in
   [[pyo3-current-state]].

## Teaching hooks recorded for the deep-dive

- Open with the downloads table: "you already ran Rust today - probably before your
  first coffee" (cryptography ~1 B/mo).
- Name the two patterns (extension module vs binary-in-a-wheel) before demoing either.
- Use pydantic-core/tokenizers `pyproject.toml` snippets as "real-world versions of the
  file we just wrote" when showing the example's `[tool.maturin]` table.
- Keep the "where Rust does not help" list next to the benchmark slide - the 19× claim
  is more persuasive with its boundary conditions attached.
- Point NumPy users at rust-numpy 0.29 (versions pair with pyo3), not at hand-rolled
  per-element functions.

## Sources

- Build configs (default branches, read 2026-07-10):
  <https://github.com/astral-sh/ruff/blob/main/pyproject.toml>,
  <https://github.com/astral-sh/uv/blob/main/pyproject.toml>,
  <https://github.com/pola-rs/polars/blob/main/py-polars/pyproject.toml>,
  <https://github.com/pola-rs/polars/blob/main/crates/polars-python/Cargo.toml>,
  <https://github.com/pydantic/pydantic-core/blob/main/pyproject.toml>,
  <https://github.com/ijl/orjson/blob/master/pyproject.toml> (+ `Cargo.toml`),
  <https://github.com/pyca/cryptography/blob/main/pyproject.toml>,
  <https://github.com/huggingface/tokenizers/blob/main/bindings/python/pyproject.toml>
- polars runtime split: <https://pypi.org/project/polars-runtime-32/>,
  <https://docs.pola.rs/user-guide/installation/>
- cryptography Rust history: <https://cryptography.io/en/latest/changelog/> (3.4.x and
  35.0.0 entries)
- rust-numpy: <https://crates.io/api/v1/crates/numpy>, <https://github.com/PyO3/rust-numpy>
- Download badges: <https://pepy.tech/> (`https://static.pepy.tech/badge/<pkg>/month`)
- PyO3 101 workshop: <https://us.pycon.org/2024/schedule/presentation/113/>,
  <https://2024.pycon.de/program/8C83EA/>,
  <https://ep2025.europython.eu/session/writing-python-modules-in-rust-pyo3-101/>,
  <https://github.com/Cheukting/py03_101>
- Official tutorials: <https://pyo3.rs/>, <https://www.maturin.rs/tutorial.html>,
  <https://github.com/PyO3/pyo3/tree/main/examples>
- Local ground truth: `Working/phase-01-ground-truth.md` (versions, benchmark, linker
  and `CONDA_PREFIX` observations)
