---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Rust, PyO3, and Maturin

---

## Rust is already in your stack

You have almost certainly run Rust today, even if you have never written a
line of it:

- **ruff** lints your Python, **uv** resolves and installs your packages
- **Polars** crunches your dataframes, **pydantic-core** validates your models
- **cryptography** guards your TLS connections

All Python-facing tools with a Rust core, shipped to PyPI as ordinary wheels.

<br>

Today: a real extension module in Rust - the whole toolchain, compiler
included, installed by pixi - and a **19× speedup** along the way.

---

## Why Rust (and not C or C++)?

- **Memory safety without a garbage collector** - use-after-free,
  double-free, a forgotten reference count: compile errors instead of
  segfault whack-a-mole, with performance still in C territory
- **A real package manager** - cargo resolves, builds, and tests with one
  tool; adding a library is one line in a manifest, not a CMake scavenger
  hunt

<br>

The borrow checker earns some of its reputation - but extension-module code
(small numeric kernels, tight loops, parsing) needs only a small subset of
Rust, and PyO3 hides most of the sharp edges.

---

## The stack, translated

| Rust         | Python analog     | Role                             |
| ------------ | ----------------- | -------------------------------- |
| cargo        | pip               | installs dependencies and builds |
| crates.io    | PyPI              | the public package index         |
| `Cargo.toml` | `pyproject.toml`  | metadata and dependencies        |
| PyO3         | pybind11          | the binding layer                |
| maturin      | scikit-build-core | the PEP 517 build backend        |

The last row is the important one: **maturin is just another build
backend**. The same `pyproject.toml` plumbing as ever - pip, uv, and pixi
build and install the project like any other Python package.

---

## Setup: pixi installs the compiler

Nearly every PyO3 tutorial begins with "install Rust with rustup." We won't:

```toml
[workspace]
channels = ["conda-forge"]

[tasks.develop]
cmd = "maturin develop"

[tasks.test]
cmd = "pytest -v"
depends-on = ["develop"]

[dependencies]
rust = ">=1.85"
maturin = ">=1.9"
python = ">=3.12"
pytest = ">=8"
```

- One `pixi install` provisions `rustc`, `cargo`, clippy, and `rustfmt` -
  nothing touches your system
- Commit **both** lockfiles: `pixi.lock` (toolchain), `Cargo.lock` (crates)

---

## Project layout

`maturin new -b pyo3` scaffolds it - two manifests and one source file:

```text
2_04_rust_pyo3/
├── pixi.toml         # toolchain and tasks
├── pyproject.toml    # Python package metadata
├── Cargo.toml        # Rust package metadata
├── src/
│   └── lib.rs        # the extension module itself
├── tests/
│   └── test_pyo3_example.py
└── bench.py          # the benchmark
```

- `pyproject.toml`: the same shape as every one in this book - only
  `build-backend = "maturin"` and `features = ["pyo3/extension-module"]`
  are new
- `Cargo.toml`: its Rust twin - one dependency (`pyo3`), and
  `crate-type = ["cdylib"]`: a C shared library, which is exactly what a
  CPython extension module is

---

## The module: `src/lib.rs`

```rust
use pyo3::prelude::*;

/// A Python module implemented in Rust.
#[pymodule]
mod pyo3_example {
    use pyo3::prelude::*;

    /// Formats the sum of two numbers as string.
    #[pyfunction]
    fn sum_as_string(a: usize, b: usize) -> PyResult<String> {
        Ok((a + b).to_string())
    }

    // ... a class and two more functions, later in this deck
}
```

---

## Reading it as a Pythonista

- `use pyo3::prelude::*;` - `from pyo3.prelude import *`; Rust preludes are
  curated for exactly this
- `///` comments become `__doc__`
- `#[pymodule]` and `#[pyfunction]` look like decorators but run at
  *compile time*, generating the entry point `import pyo3_example` needs
- `fn` is `def` with type annotations that are *enforced*: `usize` converts
  from `int` at the boundary - `OverflowError` if negative or too large
- `PyResult<String>` is "a `str`, or a Python exception" - errors are
  values, and returning `Err` raises
- No `return` needed: the last expression in a block is its value

---

## The develop loop

```console
$ pixi run develop
🐍 Found CPython 3.14 at .../.pixi/envs/default/bin/python
🔗 Found pyo3 bindings
   Compiling pyo3_example v0.1.0
📦 Built wheel to .../pyo3_example-0.1.0-cp314-cp314-linux_x86_64.whl
🛠 Installed pyo3_example-0.1.0
```

```pycon
>>> import pyo3_example
>>> pyo3_example.sum_as_string(2, 3)
'5'
```

- maturin found the interpreter on its own - a pixi environment is a conda
  environment, detected just like a virtualenv
- The one Rust-specific rule: after every edit to `src/lib.rs`, rerun
  `pixi run develop` - you cannot re-import your way to new Rust code

---

## How fast?

```console
$ pixi run bench
✨ Pixi task (develop-release): maturin develop --release
✨ Pixi task (bench): python bench.py
count_primes(1_000_000), best of 3 runs:
  pure Python: 1.601 s
  Rust (PyO3): 0.084 s
  speedup:     19x
```

- A line-for-line transcription of the same trial-division loop - no
  algorithm change, no tuning
- **Benchmark release builds only**: `maturin develop` compiles a *debug*
  build - 10-50× slower, it can even lose to pure Python. Here `bench`
  depends on `develop-release`, so it cannot get this wrong
- The design rule: move the **loop** into Rust, not the loop body

---

## Classes: a `struct` holds the data

Still inside the `mod pyo3_example` block:

```rust
    /// A 2D point, exposed to Python as a class.
    #[pyclass]
    struct Point {
        #[pyo3(get)]
        x: f64,
        #[pyo3(get)]
        y: f64,
    }
```

- A `struct` is pure data - named, typed fields; behavior lives in a
  separate `impl` block (next slide)
- `#[pyo3(get)]` generates a read-only attribute - a `@property` without
  the boilerplate; there is no `set`, so assigning to `p.x` raises
  `AttributeError`

---

## ...and `#[pymethods]` holds the behavior

```rust
    #[pymethods]
    impl Point {
        #[new]
        fn new(x: f64, y: f64) -> Self {
            Point { x, y }
        }

        /// Distance from the origin.
        fn magnitude(&self) -> f64 {
            (self.x * self.x + self.y * self.y).sqrt()
        }
    }
```

`#[new]` marks `__init__`; `&self` is `self`, *borrowed* rather than owned.
A `__repr__` method (elided here) wires straight into the Python protocol:

```pycon
>>> p = pyo3_example.Point(3.0, 4.0)
>>> p.magnitude()
5.0
>>> p
Point(x=3.0, y=4.0)
```

---

## Errors that feel native

```rust
    /// Divides `a` by `b`, raising ZeroDivisionError like Python's `/`.
    #[pyfunction]
    fn checked_div(a: f64, b: f64) -> PyResult<f64> {
        use pyo3::exceptions::PyZeroDivisionError;

        if b == 0.0 {
            return Err(PyZeroDivisionError::new_err("division by zero"));
        }
        Ok(a / b)
    }
```

```pycon
>>> pyo3_example.checked_div(1.0, 0.0)
Traceback (most recent call last):
  ...
ZeroDivisionError: division by zero
```

- `pyo3::exceptions` mirrors the builtins; returning `Err` raises a genuine
  exception - `except ZeroDivisionError:` catches it
- Even a Rust *panic* cannot take down the interpreter - PyO3 catches it at
  the boundary and raises `PanicException`

---

## The GIL (and life without it)

```rust
    /// Counts primes below `limit` by trial division.
    #[pyfunction]
    fn count_primes(py: Python<'_>, limit: u64) -> u64 {
        // Release the GIL so other Python threads can run during the hot loop.
        py.detach(|| {
            let mut count = 0;
            for n in 2..limit {
                // ... the trial-division loop, straight from bench.py
            }
            count
        })
    }
```

- `py: Python<'_>`: a token proving this thread holds the GIL - PyO3
  supplies it, so Python callers still pass just `limit`
- `py.detach(|| ...)` releases the GIL for the whole hot loop, like NumPy
  does around its C loops - and the *compiler* rejects closures that touch
  Python objects while detached
- Free-threaded CPython removes the GIL; PyO3 is ahead of the curve, and
  Rust's `Send`/`Sync` checks audit your extension for data races

---

## Shipping wheels

```console
$ pixi run maturin build --release
    Finished `release` profile [optimized] target(s) in 2.47s
📦 Built wheel to target/wheels/
   pyo3_example-0.1.0-cp314-cp314-manylinux_2_28_x86_64.whl
```

- Read the filename like a shipping label: `cp314-cp314` - exactly
  CPython 3.14; `manylinux_2_28_x86_64` - any x86-64 Linux with
  glibc ≥ 2.28, earned because maturin audits every wheel itself
- **abi3**: add the `abi3-py310` feature to `pyo3` in `Cargo.toml` and one
  `cp310-abi3` wheel per platform covers every CPython from 3.10 on -
  cryptography ships exactly such wheels
- Nobody builds the matrix by hand: `maturin generate-ci github` prints a
  complete GitHub Actions release workflow - and cibuildwheel supports
  maturin projects too

---

## Try it yourself, then go deeper

`pixi run test` should pass before you change anything. Then:

1. Add an `is_prime(n)` `#[pyfunction]` - the trial-division test inside
   `count_primes` is the algorithm
2. Add a `distance_to(other)` method to `Point`, so that
   `Point(0.0, 0.0).distance_to(Point(3.0, 4.0))` gives `5.0`
3. Extend `tests/test_pyo3_example.py` to cover both; make `pixi run test`
   pass again

<br>

**Further reading:** the [PyO3 user guide](https://pyo3.rs/) · the
[maturin user guide](https://www.maturin.rs/) ·
[rust-numpy](https://github.com/PyO3/rust-numpy) for zero-copy NumPy arrays ·
[The Rust Programming Language](https://doc.rust-lang.org/book/) ("the Book")
