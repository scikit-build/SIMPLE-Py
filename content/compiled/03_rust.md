---
authors: [Matt McCormick]
---

# Rust, PyO3, and Maturin

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/2_03_rust>`

In this chapter we build a real, importable Python extension module in **Rust**, with **PyO3** providing the bindings and **maturin** as the build backend, and the entire toolchain, Rust compiler included, installed by pixi.
No prior Rust is assumed: every Rust construct we meet is translated into a Python concept you already know.
The complete project lives in [`examples/2_04_rust_pyo3/`](https://github.com/scikit-build/SIMPLE-Py/tree/main/examples/2_04_rust_pyo3) in the workshop repository.

## Why Rust?

You have almost certainly run Rust today, even if you have never written a line of it.
[ruff](https://astral.sh/ruff) lints and formats your Python, [uv](https://docs.astral.sh/uv/) resolves and installs your packages, [Polars](https://pola.rs/) crunches your dataframes, [pydantic-core](https://github.com/pydantic/pydantic-core) validates your models, and [cryptography](https://cryptography.io/) guards your TLS connections.
All of them are Python-facing tools with a Rust core, shipped to PyPI as ordinary wheels.

Why do so many extension authors reach for Rust instead of C or C++?
Two reasons stand out.
First, **memory safety without a garbage collector**: the Rust compiler proves at compile time that memory is used correctly, so the classic native-extension crashes (use-after-free, double-free, a forgotten reference count) become compile errors instead of segfault whack-a-mole, while performance stays in C territory because there is no garbage collector pausing your hot loop.
Second, **a real package manager**: cargo resolves dependencies, builds, and tests with one tool, so adding a library is one line in a manifest rather than a CMake scavenger hunt.

Rust has a reputation for a steep learning curve, and the borrow checker earns some of it.
But for the kind of code that belongs in an extension module (small numeric kernels, tight loops, parsing), the subset of Rust you need is small, and PyO3 hides most of the sharp edges.

## The stack, translated

The Rust ecosystem maps almost one-to-one onto tools this book has already covered:

| Rust           | Python analog       | Role                                                          |
| -------------- | ------------------- | ------------------------------------------------------------- |
| cargo          | pip                 | resolves and installs dependencies, drives builds             |
| crates.io      | PyPI                | the public package index                                      |
| `Cargo.toml`   | `pyproject.toml`    | project metadata and dependencies                             |
| PyO3           | pybind11            | the binding layer (PyO3 is to Rust what pybind11 is to C++)   |
| maturin        | scikit-build-core   | the PEP 517 build backend that turns source into a wheel      |

The last row is the important one: **maturin is just another build backend**.
The same `pyproject.toml` plumbing you have used throughout this book applies unchanged, and any frontend (pip, uv, pixi) can build and install the project like any other Python package.

## Setup with pixi

Nearly every published PyO3 tutorial begins with "install Rust with rustup."
We won't.
conda-forge packages the Rust toolchain, so pixi installs it into the project environment exactly the way it installs Python, maturin, and pytest.
Here is the example's complete `pixi.toml`:

```{code} toml
:filename: pixi.toml

[workspace]
channels = ["conda-forge"]
name = "pyo3-example"
platforms = ["linux-64", "osx-arm64"]
version = "0.1.0"

[tasks.develop]
description = "Build the Rust extension and install it into the pixi environment"
cmd = "maturin develop"

[tasks.develop-release]
description = "Build the optimized Rust extension and install it"
cmd = "maturin develop --release"

[tasks.test]
description = "Run the pytest suite"
cmd = "pytest -v"
depends-on = ["develop"]

[tasks.bench]
description = "Benchmark the Rust extension against pure Python"
cmd = "python bench.py"
depends-on = ["develop-release"]

[dependencies]
rust = ">=1.85"
maturin = ">=1.9"
python = ">=3.12"
pytest = ">=8"
```

The `[dependencies]` table is the entire toolchain: the Rust compiler is the one-line `rust = ">=1.85"`.
The `[tasks]` table wraps the commands we will use for the rest of the chapter, and `depends-on` chains a build in front of the tests and the benchmark so they can never run against a stale extension.

::: {tip} No rustup, no system Rust
:class: dropdown

A single `pixi install` provisions `rustc`, `cargo`, and even Rust's linter and formatter (`clippy` and `rustfmt`, the `ruff check` and `ruff format` of Rust) from conda-forge, entirely inside the project environment.
Nothing touches your system: no rustup, no shell-profile edits, no admin rights.

:::

Reproducibility follows the same pattern as everywhere else in the book, just doubled: `pixi.lock` pins the toolchain (rustc, maturin, Python, pytest), while `Cargo.lock` pins the Rust libraries the extension depends on.
Commit both.

## A first extension

maturin can scaffold a fresh project for you (`maturin new -b pyo3` generates a PyO3 project skeleton), and the result is where our example started.
The layout is compact:

```text
2_04_rust_pyo3/
├── pixi.toml         # toolchain and tasks (shown above)
├── pyproject.toml    # Python package metadata
├── Cargo.toml        # Rust package metadata
├── src/
│   └── lib.rs        # the extension module itself
├── tests/
│   └── test_pyo3_example.py
└── bench.py          # the benchmark, later in this chapter
```

The Rust side is just two manifests and one source file.
Start with the file you know best:

```{code} toml
:filename: pyproject.toml

[build-system]
requires = ["maturin>=1.14,<2.0"]
build-backend = "maturin"

[project]
name = "pyo3_example"
version = "0.1.0"
requires-python = ">=3.9"

[tool.maturin]
features = ["pyo3/extension-module"]
```

This should look completely familiar; it has the same shape as every `pyproject.toml` in this book, with only the backend name changed.
The one new line is `[tool.maturin] features`, which compiles PyO3 in *extension module* mode: build a module to be loaded by an existing interpreter, rather than embedding an interpreter inside a Rust program.

```{code} toml
:filename: Cargo.toml

[package]
name = "pyo3_example"
version = "0.1.0"
edition = "2024"

[lib]
name = "pyo3_example"
crate-type = ["cdylib"]

[dependencies]
pyo3 = "0.29.0"
```

`Cargo.toml` is `pyproject.toml`'s Rust twin.
A **crate** is Rust's unit of packaging: this project is one crate with one dependency, PyO3, which cargo fetches from crates.io on the first build.
`edition = "2024"` opts into a snapshot of language rules: think `from __future__ import ...` applied project-wide, not a compiler version.
The line doing the real work is `crate-type = ["cdylib"]`: build a *C dynamic library*, a shared library exposing C symbols, which is precisely what a CPython extension module is.

Finally, the module itself.
Here is the top of `src/lib.rs`:

```{code} rust
:filename: src/lib.rs

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

    // ...
}
```

Reading it as a Pythonista:

* `use pyo3::prelude::*;` is an import: `from pyo3.prelude import *`. Glob imports are frowned upon in Python; Rust *preludes* are curated for exactly this use.
* `///` comments are docstrings: PyO3 turns them into the `__doc__` of the module and function.
* `#[pymodule] mod pyo3_example { ... }`: `mod` declares a namespace, like a Python module but spelled out explicitly. The `#[pymodule]` **attribute** looks like a decorator and reads like one, but runs at *compile time*, generating the entry point CPython looks for when it executes `import pyo3_example`. This block is the Rust analog of "the code that runs at import."
* `#[pyfunction]` is the decorator-alike that exposes a Rust `fn` (a `def`) to Python.
* `fn sum_as_string(a: usize, b: usize) -> PyResult<String>`: type annotations that are actually *enforced*. `usize` is a machine-sized unsigned integer; Python's `int` is arbitrary-precision, so PyO3 converts at the boundary and raises `OverflowError` if a value is negative or too large. The `String` comes back out as an ordinary `str`.
* `Ok((a + b).to_string())`: Rust returns errors as values rather than raising. `PyResult<String>` means "a `str`, or a Python exception"; `Ok` wraps the success case, and returning an `Err` raises a genuine exception on the Python side; we will use that later in this chapter.
* There is no `return`: the last expression in a block is its value.

The `// ...` marks the rest of the file: a class and two more functions we will meet in the coming sections.

## Build and iterate

One command builds the extension and installs it into the environment:

```console
$ pixi run develop
✨ Pixi task (develop): maturin develop: (Build the Rust extension and install it into the pixi environment)
🐍 Found CPython 3.14 at /home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3/.pixi/envs/default/bin/python
🔗 Found pyo3 bindings
📡 Using build options features from pyproject.toml
   Compiling pyo3_example v0.1.0 (/home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3)
    Finished `dev` profile [unoptimized + debuginfo] target(s) in 0.09s
📦 Built wheel for CPython 3.14 to /tmp/.tmpxvrtja/pyo3_example-0.1.0-cp314-cp314-linux_x86_64.whl
✏️ Setting installed package as editable
🛠 Installed pyo3_example-0.1.0
```

Note what you did *not* have to configure: maturin found the environment's interpreter on its own.
A pixi environment is a conda environment, so `maturin develop` detects it exactly as it would a virtualenv: compile, wrap in a wheel, install, done.
The very first build takes a minute or two while cargo compiles PyO3 itself; after that, rebuilds take seconds.

Start a REPL inside the environment with `pixi run python`:

```{code} python
>>> import pyo3_example
>>> pyo3_example.sum_as_string(2, 3)
'5'
```

That is the whole loop: Rust code, called from Python, with type conversion handled at the boundary.

::: {note} What did we just install?
:class: dropdown

Look inside the environment's `site-packages` and you will find `pyo3_example/pyo3_example.cpython-314-x86_64-linux-gnu.so`, plus a small generated `__init__.py` that re-exports it.
An extension module is nothing more exotic than that shared library.
Its long filename suffix encodes the interpreter version, ABI, and platform it was built for (this one only loads on CPython 3.14 on x86-64 Linux), a contract that resurfaces when we ship wheels at the end of the chapter.

:::

This is the same editable workflow the book covers in [Editable installs](../scikit-build/04_editable_installs.md): maturin is a full PEP 660 backend, so `pip install -e .` works here too, and `maturin develop` is the convenient shorthand for it.
The one Rust-specific rule of the edit-rebuild loop: Python cannot re-import its way to your new Rust code.
After every change to `src/lib.rs`, rerun `pixi run develop`; if an edit ever seems to do nothing, the installed extension is stale; the fix is the same command.

## How fast is it?

Time to cash the performance check.
The example module also exports `count_primes(limit)`, which counts the primes below `limit` by trial division: a deliberately branchy, CPU-bound loop, exactly the kind of code CPython executes slowly and a compiler loves.
We will read its Rust source a little later in the chapter; first, let's measure it.
`bench.py` implements the identical algorithm in pure Python and times both:

```{code} python
:filename: bench.py

"""Benchmark pyo3_example.count_primes against the same algorithm in pure Python."""

import timeit

import pyo3_example

LIMIT = 1_000_000


def count_primes(limit):
    """Count primes below `limit` by trial division (same algorithm as src/lib.rs)."""
    count = 0
    for n in range(2, limit):
        is_prime = True
        d = 2
        while d * d <= n:
            if n % d == 0:
                is_prime = False
                break
            d += 1
        if is_prime:
            count += 1
    return count


if __name__ == "__main__":
    assert count_primes(10_000) == pyo3_example.count_primes(10_000)

    print(f"count_primes({LIMIT:_}), best of 3 runs:")
    python_seconds = min(timeit.repeat(lambda: count_primes(LIMIT), number=1, repeat=3))
    rust_seconds = min(
        timeit.repeat(lambda: pyo3_example.count_primes(LIMIT), number=1, repeat=3)
    )
    print(f"  pure Python: {python_seconds:.3f} s")
    print(f"  Rust (PyO3): {rust_seconds:.3f} s")
    print(f"  speedup:     {python_seconds / rust_seconds:.0f}x")
```

The `assert` keeps the comparison honest: both implementations must agree on an answer before either is timed.
One task runs the benchmark, and its `depends-on` chain (look back at `pixi.toml`) rebuilds the extension in *release* mode first:

```console
$ pixi run bench
✨ Pixi task (develop-release): maturin develop --release: (Build the optimized Rust extension and install it)
🐍 Found CPython 3.14 at /home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3/.pixi/envs/default/bin/python
🔗 Found pyo3 bindings
📡 Using build options features from pyproject.toml
   Compiling pyo3_example v0.1.0 (/home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3)
    Finished `release` profile [optimized] target(s) in 0.19s
📦 Built wheel for CPython 3.14 to /tmp/.tmpfn9edB/pyo3_example-0.1.0-cp314-cp314-linux_x86_64.whl
✏️ Setting installed package as editable
🛠 Installed pyo3_example-0.1.0

✨ Pixi task (bench): python bench.py: (Benchmark the Rust extension against pure Python)
count_primes(1_000_000), best of 3 runs:
  pure Python: 1.601 s
  Rust (PyO3): 0.084 s
  speedup:     19x
```

Nineteen times faster, from a line-for-line transcription of the Python loop: no algorithm change, no tuning.
The exact digit varies by machine and by run; call it an order of 20×.

::: {warning} Benchmark release builds only

`maturin develop` compiles an *unoptimized* debug build by default: the fast-compile, slow-run profile meant for the edit-rebuild loop, the same trade-off as `cmake -DCMAKE_BUILD_TYPE=Debug`.
Depending on the workload, debug Rust can run 10-50× slower than release and can even lose to pure Python, which is where most "I rewrote it in Rust and it got slower!" stories come from.
Benchmark only `--release` builds.
The example's `pixi.toml` encodes the rule so nobody has to remember it: `bench` depends on `develop-release`, so the benchmark can never see a debug build.

:::

An honest coda: a 19× speedup is representative for moving a branchy scalar loop wholesale into a compiled language, not a promise.
Code that is I/O-bound, or that already spends its time inside NumPy's vectorized C loops, gains little from Rust; and a Python loop that calls into Rust once per element instead of once per loop can end up *slower* than pure Python, because every crossing pays the conversion toll at the boundary.
The design rule that covers all of these: **move the loop into Rust, not the loop body**.

## Classes

Speed is one half of the story; ergonomics is the other.
`#[pyclass]` exposes a Rust `struct` to Python as a class.
Here is the next piece of `src/lib.rs` (still inside the `mod pyo3_example` block):

```{code} rust
:filename: src/lib.rs

    /// A 2D point, exposed to Python as a class.
    #[pyclass]
    struct Point {
        #[pyo3(get)]
        x: f64,
        #[pyo3(get)]
        y: f64,
    }

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

        fn __repr__(&self) -> String {
            // {:?} keeps the decimal point on whole floats: 1.0, not 1.
            format!("Point(x={:?}, y={:?})", self.x, self.y)
        }
    }
```

Again as a Pythonista:

* A `struct` is pure data with named, typed fields; Rust keeps behavior in a separate `impl` block, where the methods live. `#[pyclass]` and `#[pymethods]` publish the pair to Python as one ordinary class.
* `#[new]` marks the constructor: `__init__`. Its body `Point { x, y }` builds the struct, using shorthand for `Point { x: x, y: y }`.
* `#[pyo3(get)]` generates a read-only attribute for a field: a `@property` without the boilerplate. There is no matching `set`, so assigning to `p.x` raises `AttributeError`.
* `&self` is `self`; the `&` means the method *borrows* the instance to read it rather than taking ownership.
* `__repr__` is exactly what you think: give a method a dunder name and PyO3 wires it into the matching Python protocol.

```{code} python
>>> p = pyo3_example.Point(3.0, 4.0)
>>> p.magnitude()
5.0
>>> p
Point(x=3.0, y=4.0)
>>> p.x
3.0
```

## Errors that feel native

A Rust extension should not make its callers learn new failure modes: when things go wrong, Python code expects a Python exception.
`PyResult` is the bridge.
The first half of the chapter promised we would return an `Err`; here it is:

```{code} rust
:filename: src/lib.rs

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

The `use` inside the function body is a scoped import, like an `import` inside a `def`: it keeps the exception type next to its only use.
`pyo3::exceptions` mirrors the builtins (`PyValueError`, `PyTypeError`, `PyKeyError`, and friends), and `new_err` constructs one with a message.
Returning it as an `Err` (note that `return` exists in Rust; it is simply optional on the last expression) raises it on the Python side:

```{code} python
>>> pyo3_example.checked_div(1.0, 4.0)
0.25
>>> pyo3_example.checked_div(1.0, 0.0)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
    pyo3_example.checked_div(1.0, 0.0)
    ~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^
ZeroDivisionError: division by zero
```

That is a genuine `ZeroDivisionError`; `except ZeroDivisionError:` catches it, and nothing about it betrays that the raiser was Rust.
Even a Rust *panic* (its equivalent of an unrecoverable error) does not take down the interpreter: PyO3 catches it at the boundary and raises a `PanicException` instead.

## The GIL (and life without it)

One promise is still outstanding: the Rust source of `count_primes`, which hides the chapter's most Python-flavored topic: the Global Interpreter Lock.

```{code} rust
:filename: src/lib.rs

    /// Counts primes below `limit` by trial division.
    #[pyfunction]
    fn count_primes(py: Python<'_>, limit: u64) -> u64 {
        // Release the GIL so other Python threads can run during the hot loop.
        py.detach(|| {
            let mut count = 0;
            for n in 2..limit {
                let mut is_prime = true;
                let mut d = 2;
                while d * d <= n {
                    if n % d == 0 {
                        is_prime = false;
                        break;
                    }
                    d += 1;
                }
                if is_prime {
                    count += 1;
                }
            }
            count
        })
    }
```

The loop itself should read fine by now; `let mut` declares a variable that is allowed to change (Rust variables are immutable unless you say otherwise), and `2..limit` is `range(2, limit)`.
The interesting part is the frame around it:

* `py: Python<'_>` is a *token* proving this thread currently holds the GIL. PyO3 recognizes the parameter type and supplies it automatically; from Python, `count_primes` still takes a single argument.
* `py.detach(|| { ... })` runs the closure (`|| { ... }` is Rust's lambda) with the GIL released. Other Python threads get to run for the entire duration of the hot loop, the same courtesy NumPy extends around its C loops.
* While detached, the code must not touch Python objects. In a C extension that is a rule you memorize; here it is a rule the compiler enforces. A closure that tries to capture anything Python-side does not compile.

::: {tip} Life without the GIL
:class: dropdown

The GIL itself is on the way out: [free-threaded CPython](../interesting/free_threading.md) removes it entirely, and PyO3 is ahead of the curve: recent releases support free-threaded builds, where `py.detach` keeps its meaning (the thread detaches from the interpreter, so it no longer blocks stop-the-world operations like garbage collection).
Free-threading raises the thread-safety bar for every extension author, and this is where Rust earns its keep a second time: the compiler's `Send`/`Sync` checks audit your extension for data races at compile time.
Our `Point`, immutable after construction, needs no changes at all.

:::

## Shipping wheels

`maturin develop` installs into your own environment; sharing with anyone else means building a wheel:

```console
$ pixi run maturin build --release
🐍 Found CPython 3.14 at /home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3/.pixi/envs/default/bin/python3
🔗 Found pyo3 bindings
📡 Using build options features from pyproject.toml
   Compiling pyo3-ffi v0.29.0
   Compiling pyo3 v0.29.0
   Compiling pyo3_example v0.1.0 (/home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3)
    Finished `release` profile [optimized] target(s) in 2.47s
📦 Built wheel for CPython 3.14 to /home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3/target/wheels/pyo3_example-0.1.0-cp314-cp314-manylinux_2_28_x86_64.whl
```

Read the wheel's filename like a shipping label.
`cp314-cp314` says it was built for exactly CPython 3.14, and `manylinux_2_28_x86_64` says it runs on any x86-64 Linux with glibc 2.28 or newer.
That second tag is earned, not asserted: maturin bundles a reimplementation of `auditwheel` and audits every `maturin build` wheel automatically, work that is a separate tool and a separate CI step in the C++ chapters.
(Compare the throwaway wheels `maturin develop` dropped in `/tmp` earlier: those are tagged plain `linux_x86_64`, fine for installing into the local environment, but PyPI would reject them.)

The `cp314-cp314` half is the expensive one: one wheel per Python version per platform, so supporting five CPython versions on three platforms means fifteen builds.
The escape hatch is the **stable ABI**, nicknamed abi3: a restricted subset of the C API that CPython promises to keep compatible across versions.
Add the `abi3-py310` feature (or another version floor) to the `pyo3` dependency in `Cargo.toml`, and maturin emits a single `cp310-abi3` wheel per platform that installs on every CPython from 3.10 onward, at the cost of a slightly restricted API and some conversion fast paths.
[cryptography](https://cryptography.io/) ships exactly such wheels to millions of users a day.

Nobody builds a wheel matrix by hand.
`maturin generate-ci github` prints a complete GitHub Actions release workflow (platform matrix, manylinux-compliant build containers, an sdist, and PyPI upload via trusted publishing), ready to drop into `.github/workflows/`.
Alternatively, [cibuildwheel](./04_cibuildwheel.md) supports maturin projects too: if your CI already revolves around it, it will happily build your Rust wheels alongside everything else.

## Try it yourself

:::{exercise} Extend the extension
:label: rust-pyo3-extend

Clone the workshop repository and open `examples/2_04_rust_pyo3/`; `pixi run test` should pass before you change anything.
Then:

1. Add an `is_prime(n)` `#[pyfunction]` that reports whether a single number is prime; the trial-division test inside `count_primes` is the algorithm.
2. Add a `distance_to(other)` method to `Point` returning the distance between two points, so that `Point(0.0, 0.0).distance_to(Point(3.0, 4.0))` gives `5.0`.
3. Extend `tests/test_pyo3_example.py` to cover both, and make `pixi run test` pass again.

:::

:::{solution} rust-pyo3-extend
:class: dropdown

Some hints in place of full code:

* `is_prime` needs neither `PyResult` nor a `py` token: the signature `fn is_prime(n: u64) -> bool` is enough, and the `bool` comes back to Python as `True`/`False`. Put it inside the `mod pyo3_example` block with `#[pyfunction]` on top.
* `distance_to` goes inside the existing `#[pymethods] impl Point` block with the signature `fn distance_to(&self, other: &Point) -> f64`; PyO3 handles borrowing `other` when Python passes in a second `Point`. The body is two subtractions away from `magnitude`.
* Remember the rule from *Build and iterate*: Rust edits need a rebuild. `pixi run test` runs one for you via its `depends-on: ["develop"]`.

:::

## Where to go next

Four pointers, in the order worth reading them:

* The [PyO3 user guide](https://pyo3.rs/): start with the getting-started walkthrough, then the chapters on classes and modules; everything this chapter glossed over is there.
* The [maturin user guide](https://www.maturin.rs/): project layouts (including mixed Rust/Python packages), configuration, and the full CLI reference.
* [rust-numpy](https://github.com/PyO3/rust-numpy): zero-copy NumPy arrays in Rust. The moment your hot data is an array, reach for this instead of converting element by element.
* [The Rust Programming Language](https://doc.rust-lang.org/book/): "the Book", the canonical way to actually learn Rust; its first ten chapters cover every construct this example uses.

The stack you used today does not change as the project grows: the same `pixi.toml`, the same two manifests, the same `pixi run develop` loop carry a module from these eighty lines to ruff- and Polars-sized codebases.
Only `src/lib.rs` gets bigger.
