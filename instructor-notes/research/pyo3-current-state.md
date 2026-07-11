---
type: research
title: PyO3 - Current State (July 2026)
created: 2026-07-10
tags:
  - rust
  - pyo3
  - python-bindings
related:
  - '[[maturin-workflows]]'
  - '[[pixi-rust-toolchain]]'
  - '[[rust-python-landscape]]'
  - '[[rust-pyo3-maturin-deep-dive]]'
---

Research note for the Rust/PyO3/maturin section (`examples/2_04_rust_pyo3/`).
All claims were checked on 2026-07-10 against pyo3.rs, docs.rs, crates.io, and the
PyO3 GitHub changelog; anything that could not be re-verified today is marked
"(unverified)". Local ground truth: the Phase 01 example builds with **pyo3 0.29.0**
(edition 2024, `cdylib`) on Python 3.14.6 and Rust 1.97.0 from conda-forge.

## Latest release, MSRV, and support matrix

- **Latest release: 0.29.0, published 2026-06-11** - also the version in the example's
  `Cargo.lock`.
  Source: <https://crates.io/api/v1/crates/pyo3> (`max_version: "0.29.0"`,
  `created_at: 2026-06-11`); <https://github.com/PyO3/pyo3/releases>.
- **MSRV: Rust 1.83** ("Requires Rust 1.83 or greater"; bumped in 0.28.0 - "Bump MSRV to
  Rust 1.83"). The example's pixi pin `rust = ">=1.85"` clears this comfortably, and
  edition 2024 needs ≥1.85 anyway.
  Sources: <https://github.com/PyO3/pyo3/blob/main/README.md>,
  <https://pyo3.rs/main/changelog.html>.
- **Python support: CPython ≥3.9, PyPy 7.3 (Python 3.11+), GraalPy ≥25.0 (Python 3.12+).**
  CPython 3.8 support was dropped in 0.28.0.
  Sources: <https://github.com/PyO3/pyo3/blob/main/README.md>,
  <https://pyo3.rs/main/changelog.html>.
- **Release hygiene worth knowing:** 0.28.0 and 0.28.1 were later **yanked** on crates.io
  (0.28.2/0.28.3 superseded them), and 0.29.0 fixed two security issues (a missing `Sync`
  bound on `PyCFunction::new_closure` closures; a possible out-of-bounds read in
  `BoundTupleIterator::nth_back`/`BoundListIterator::nth_back`). Practical advice for the
  workshop: depend on the latest minor and let patch releases float (`pyo3 = "0.29.0"`
  is a caret requirement - it accepts 0.29.x).
  Sources: <https://crates.io/api/v1/crates/pyo3>,
  <https://github.com/PyO3/pyo3/releases>.

### Release cadence (why online snippets rot fast)

PyO3 ships a breaking minor roughly every quarter. Recent history, from the changelog
(<https://pyo3.rs/main/changelog.html>):

| Version | Date | Headline |
| ------- | ---- | -------- |
| 0.21.0 | 2024-03-25 | `Bound<'py, T>` API introduced alongside old "GIL Refs" |
| 0.23.0 | 2024-11-15 | GIL-Refs era APIs removed; `IntoPyObject`; first free-threaded (3.13t) support |
| 0.25.0 | 2025-05-14 | Python 3.14 support; `#[pyclass(generic)]`; `datetime` with abi3 |
| 0.26.0 | 2025-08-29 | `Python::with_gil`/`allow_threads` renamed to `attach`/`detach` |
| 0.27.0 | 2025-10-19 | `FromPyObject` rework (borrowing); `.downcast()` deprecated for `.cast()` |
| 0.28.0 | 2026-02-01 | MSRV 1.83; Python 3.8 dropped; free-threaded support becomes opt-out; `__init__` |
| 0.29.0 | 2026-06-11 | PEP 803 `abi3t` stable ABI for free-threaded builds; Python 3.15.0b1; 3.13t dropped |

Teaching takeaway: any tutorial or Stack Overflow answer predating April 2024 (0.21) is
written against a fundamentally different API, and even 2025 snippets may use pre-rename
GIL vocabulary. Check dates before trusting snippets.

## The `Bound<'py, T>` API - and why older snippets look different

Since 0.21, the core object handle is `Bound<'py, T>`: an owned smart pointer to a Python
object that both holds a reference count and carries the `'py` lifetime proving the thread
is attached to the interpreter. Methods on Python objects come from traits like
`PyAnyMethods`. Source: <https://pyo3.rs/v0.29.0/migration.html>.

History that explains the snippet soup found online:

- **Pre-0.21 ("GIL Refs" era):** APIs handed out `&'py PyAny` references owned by an
  internal pool. Code full of `obj.as_ref(py)`, `&PyAny`, `&PyList` parameters is from
  this era.
- **0.21-0.22 (transition):** `Bound` arrived alongside the old API; new constructors got
  temporary `_bound` suffixes (`PyTuple::new_bound(py, …)`), and `FromPyObject` gained
  `extract_bound`. Snippets with `_bound` names are from this window.
- **0.23 (2024-11-15):** the deprecated GIL-Refs functionality was removed and the plain
  names came to mean the `Bound` variants (`PyTuple::new(py, …)`). The `_bound` suffixes
  are gone in current code.
- **0.27:** `.downcast()` and `DowncastError` were deprecated in favor of `.cast()` and
  `CastError` - another marker for dating snippets.

Sources: <https://pyo3.rs/v0.29.0/migration.html>, <https://pyo3.rs/main/changelog.html>.

## The `Python<'py>` token

`Python<'py>` is a zero-sized token whose possession proves the current thread is attached
to the Python interpreter (on GIL-enabled builds: holds the GIL). It is obtained either by
`Python::attach(|py| { … })` from Rust, or by declaring a `py: Python<'py>` parameter in a
`#[pyfunction]`/`#[pymethods]` method (PyO3 injects it - the example's `count_primes` does
exactly this). The `'py` lifetime is what ties every `Bound<'py, T>` to an attached thread
state, so "you can only touch Python objects while attached" is enforced at compile time -
a nice show-piece for the borrow checker teaching moment.
Sources: <https://pyo3.rs/v0.29.0/types.html>, <https://pyo3.rs/v0.29.0/python-from-rust.html>.

## Conversion traits: `FromPyObject` and `IntoPyObject`

- **`FromPyObject`** drives argument extraction (`obj.extract::<T>()` and automatic
  `#[pyfunction]` argument conversion). Reworked in 0.27.0 with a second lifetime so
  implementations can borrow data directly from Python objects (e.g. `&str` from a
  `str` without copying); `FromPyObjectOwned` was added for the non-borrowing case.
- **`IntoPyObject`** (added in 0.23) is the single, fallible conversion trait for return
  values, replacing the older `IntoPy` and `ToPyObject` (both deprecated in 0.23 - their
  presence marks an outdated snippet). A `#[derive(IntoPyObject)]` macro exists.
- Notable 0.23 behavior change: `Vec<u8>`, `&[u8]`, and `[u8; N]` now convert to `bytes`,
  not `list` - worth a caution slide if byte data comes up.
- 0.28 deprecated the implicit `Clone`-based by-value `FromPyObject` for `#[pyclass]`
  types; it is now explicit via `#[pyclass(from_py_object)]` (or silenced with
  `#[pyclass(skip_from_py_object)]`).

For the workshop, the practical story is simpler: standard types (ints, floats, `String`,
`Vec<T>`, `HashMap<K, V>`, `Option<T>`, tuples) convert automatically in both directions,
and `PyResult<T>` return values map `Err(PyErr)` to a raised Python exception.
Sources: <https://pyo3.rs/main/changelog.html>, <https://pyo3.rs/v0.29.0/migration.html>,
<https://pyo3.rs/v0.29.0/conversions/traits.html>.

## GIL vocabulary: `attach`/`detach` (formerly `with_gil`/`allow_threads`)

All the GIL-centric names were renamed in **0.26.0 (2025-08-29)** to attachment-centric
ones, because on free-threaded builds there is no GIL - what a thread actually does is
attach to/detach from an interpreter thread state:

- `Python::with_gil` → `Python::attach` (PyO3/pyo3#5209)
- `Python::allow_threads` → `Python::detach` (PyO3/pyo3#5221)
- `Python::with_gil_unchecked` → `Python::attach_unchecked` (PyO3/pyo3#5340)
- `Python::assume_gil_acquired` → `Python::assume_attached` (PyO3/pyo3#5354)
- `pyo3::prepare_freethreaded_python` → `Python::initialize` (embedding use case)

Source: <https://pyo3.rs/main/changelog.html> (0.26.0 section);
<https://pyo3.rs/v0.29.0/migration.html>.

The old names are deprecated aliases; whether they still compile (with warnings) on 0.29
was not re-verified (unverified). What is locally verified: `Python::detach` compiles
clean with zero warnings on pyo3 0.29.0 in the Phase 01 example, where `count_primes`
wraps its hot loop in `py.detach(…)` so other Python threads can run - the canonical
"release the GIL around pure-Rust work" pattern. Anything that touches Python objects must
stay outside the `detach` closure (the closure cannot capture `Bound` values - enforced by
the type system via the `Ungil` bound).

## Free-threaded CPython support

Status as of 0.29.0 (source: <https://pyo3.rs/v0.29.0/free-threading.html> and the
changelog):

- PyO3 has supported free-threaded builds **since 0.23** (initially Python 3.13t,
  opt-in). **0.28 flipped the default**: extension modules now advertise free-threaded
  support unless they opt out with `#[pymodule(gil_used = true)]` (or
  `PyModule::gil_used(true)`). **0.29 dropped 3.13t** and targets 3.14t+, and added
  support for the new **PEP 803 `abi3t` stable ABI** (features `abi3t`, `abi3t-py315`)
  so free-threaded wheels can finally be version-independent from CPython 3.15 on.
- Every `#[pyclass]` must be `Sync` (required since 0.23). Mutable access to `#[pyclass]`
  data is protected by PyO3's `RefCell`-style runtime borrow checking, which raises or
  panics on conflicting borrows - under free threading, real concurrent access becomes
  possible, so classes with mutable state may need explicit locking.
- Synchronization helpers: `PyOnceLock` (replaces the deprecated `GILOnceCell`),
  `OnceExt`, and `MutexExt::lock_py_attached` (deadlock-safe locking while attached).
  `GILProtected` is deprecated.
- Before `abi3t`, the `abi3` feature is simply **ignored (with a build warning)** when
  building for a free-threaded interpreter - such builds produce version-specific wheels.
- `cfg(Py_GIL_DISABLED)` is available for conditional compilation.

Teaching angle: this is a strong "why Rust" argument - `Send`/`Sync` means the compiler
audits your extension's thread safety just as PEP 703 makes Python genuinely concurrent.

## abi3 (stable ABI) support and its limits

Sources: <https://pyo3.rs/v0.29.0/building-and-distribution/multiple-python-versions.html>,
<https://docs.rs/crate/pyo3/latest/features>, <https://pyo3.rs/main/changelog.html>.

- Enabled with the `abi3` feature plus a floor feature; docs.rs lists `abi3-py38` through
  `abi3-py315` on 0.29.0, plus the new `abi3t`/`abi3t-py315`. One wheel then runs on every
  CPython ≥ the floor (maturin tags it `cp3X-abi3`; see [[maturin-workflows]]).
- Note the mismatch: the `abi3-py38` feature still exists, but PyO3 0.28 dropped Python
  3.8 support (README floor is 3.9) - so 3.9 is the practical minimum floor.
- Costs and limits of the limited API (`Py_LIMITED_API`): some APIs and fast paths are
  unavailable (code gated `#[cfg(not(Py_LIMITED_API))]`), so abi3 builds can be somewhat
  slower; `datetime` types only became abi3-compatible in 0.25; subclassing native types
  with abi3 requires Python 3.12+ (added in 0.28); and abi3 does not apply to
  free-threaded builds before `abi3t` (see above). PyPy/GraalPy do not use the stable
  ABI - abi3 is a CPython mechanism (unverified for current guide wording).
- The Phase 01 example deliberately builds a **version-specific** extension (no abi3
  feature): simpler story, full API, and it matches the free-threaded reality. abi3 is a
  discussion hook for "how do I ship one wheel per platform instead of one per Python?"

## `#[pyclass]` / `#[pymethods]` capabilities relevant to teaching

Source: <https://pyo3.rs/v0.29.0/class.html>.

- **Derive-like options on `#[pyclass]`:** `eq` (uses `PartialEq` for `__eq__`), `ord`,
  `str` (uses `Display`, or a format string), `hash` (requires `eq` + `frozen`), `frozen`
  (immutable - also removes runtime borrow-check overhead), `get_all`/`set_all` (expose
  all fields), `subclass`/`extends = Base` (inheritance, both directions), `dict`,
  `weakref`, `generic` (0.25+), `mapping`, `sequence`.
- **Constructors:** `#[new]` maps to `__new__`; since 0.28 a real `__init__` can be
  written in `#[pymethods]` too. Constructors may return `Self`, `PyResult<Self>`, or
  `PyClassInitializer<Self>` for inheritance chains.
- **Properties:** `#[pyo3(get, set)]` on fields for the simple case; `#[getter]`,
  `#[setter]`, and (since 0.28) `#[deleter]` methods for computed properties.
- **Method kinds:** instance methods (`&self`/`&mut self`), `#[staticmethod]`,
  `#[classmethod]`, `#[classattr]`.
- **Enums:** simple (unit-variant) enums get `__richcmp__`/`__int__`/`__repr__` (plus
  `eq_int`); complex enums with struct/tuple variants support Python 3.10+ pattern
  matching and `#[pyo3(constructor = (…))]`.
- **Magic methods** go straight into `#[pymethods]` (`__repr__`, `__len__`, arithmetic,
  etc.); `async fn` in `#[pyfunction]`/`#[pymethods]` exists behind `experimental-async`.
- **Modules:** the guide now leads with **declarative modules** - `#[pymodule] mod name`
  auto-registers the `#[pyfunction]`s/`#[pyclass]`es declared inside it, with
  `#[pymodule_export]` for items defined elsewhere and `#[pymodule_init]` as the escape
  hatch. The Phase 01 example already uses this style (no hand-written registration).
  Since 0.28, `#[pymodule]` uses PEP 489 multi-phase initialization.
  Source: <https://pyo3.rs/v0.29.0/module.html>.

## Implications for the Phase 01 example (audit hooks)

- `pyo3 = "0.29.0"` **is the current latest** - nothing to bump.
- `Python::detach`, declarative `#[pymodule] mod`, edition 2024: all current idiom. ✓
- `requires-python = ">=3.8"` in `pyproject.toml` is stale relative to PyO3 0.28+'s
  CPython ≥3.9 floor - the audit task should raise it to `>=3.9` (or align with the pixi
  pin `python >=3.12`).
- Since 0.28 the module **advertises free-threaded support by default**; the audit should
  confirm the `#[pyclass] Point` is safe under that assumption (or opt out with
  `gil_used = true` and say why).
- pixi pin `rust >=1.85` ≥ MSRV 1.83. ✓

## Sources

- crates.io API - latest version and dates: <https://crates.io/api/v1/crates/pyo3>
- GitHub releases: <https://github.com/PyO3/pyo3/releases>
- Changelog (versions, dates, renames, MSRV): <https://pyo3.rs/main/changelog.html>
- Migration guide (Bound, IntoPyObject, attach/detach, cast): <https://pyo3.rs/v0.29.0/migration.html>
- Free-threading guide: <https://pyo3.rs/v0.29.0/free-threading.html>
- abi3 / multiple Python versions: <https://pyo3.rs/v0.29.0/building-and-distribution/multiple-python-versions.html>
- Feature list incl. abi3/abi3t floors: <https://docs.rs/crate/pyo3/latest/features>
- Class guide: <https://pyo3.rs/v0.29.0/class.html>; module guide: <https://pyo3.rs/v0.29.0/module.html>
- README (MSRV, interpreter support): <https://github.com/PyO3/pyo3/blob/main/README.md>
- Local ground truth: `examples/2_04_rust_pyo3/` (`Cargo.lock`, `pixi list`, Phase 01 records)
