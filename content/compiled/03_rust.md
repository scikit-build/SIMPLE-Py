---
authors: [Matt McCormick]
---

# Rust, PyO3, and Maturin

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/2_03_rust>`

In this chapter we build a real, importable Python extension module in **Rust**, with **PyO3** providing the bindings and **maturin** as the build backend &mdash; and the entire toolchain, Rust compiler included, installed by pixi.
No prior Rust is assumed: every Rust construct we meet is translated into a Python concept you already know.
The complete project lives in [`examples/2_04_rust_pyo3/`](https://github.com/scikit-build/SIMPLE-Py/tree/main/examples/2_04_rust_pyo3) in the workshop repository.

## Why Rust?

You have almost certainly run Rust today, even if you have never written a line of it.
[ruff](https://astral.sh/ruff) lints and formats your Python, [uv](https://docs.astral.sh/uv/) resolves and installs your packages, [Polars](https://pola.rs/) crunches your dataframes, [pydantic-core](https://github.com/pydantic/pydantic-core) validates your models, and [cryptography](https://cryptography.io/) guards your TLS connections.
All of them are Python-facing tools with a Rust core, shipped to PyPI as ordinary wheels.

Why do so many extension authors reach for Rust instead of C or C++?
Two reasons stand out.
First, **memory safety without a garbage collector**: the Rust compiler proves at compile time that memory is used correctly, so the classic native-extension crashes &mdash; use-after-free, double-free, a forgotten reference count &mdash; become compile errors instead of segfault whack-a-mole, while performance stays in C territory because there is no garbage collector pausing your hot loop.
Second, **a real package manager**: cargo resolves dependencies, builds, and tests with one tool, so adding a library is one line in a manifest rather than a CMake scavenger hunt.

Rust has a reputation for a steep learning curve, and the borrow checker earns some of it.
But for the kind of code that belongs in an extension module &mdash; small numeric kernels, tight loops, parsing &mdash; the subset of Rust you need is small, and PyO3 hides most of the sharp edges.

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
The same `pyproject.toml` plumbing you have used throughout this book applies unchanged, and any frontend &mdash; pip, uv, pixi &mdash; can build and install the project like any other Python package.

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

The `[dependencies]` table is the entire toolchain &mdash; the Rust compiler is the one-line `rust = ">=1.85"`.
The `[tasks]` table wraps the commands we will use for the rest of the chapter, and `depends-on` chains a build in front of the tests and the benchmark so they can never run against a stale extension.

::: {tip} No rustup, no system Rust

A single `pixi install` provisions `rustc`, `cargo`, and even Rust's linter and formatter (`clippy` and `rustfmt` &mdash; the `ruff check` and `ruff format` of Rust) from conda-forge, entirely inside the project environment.
Nothing touches your system: no rustup, no shell-profile edits, no admin rights.

:::

Reproducibility follows the same pattern as everywhere else in the book, just doubled: `pixi.lock` pins the toolchain (rustc, maturin, Python, pytest), while `Cargo.lock` pins the Rust libraries the extension depends on.
Commit both.

## A first extension

maturin can scaffold a fresh project for you &mdash; `maturin new -b pyo3` generates a PyO3 project skeleton &mdash; and the result is where our example started.
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

This should look completely familiar &mdash; it has the same shape as every `pyproject.toml` in this book, with only the backend name changed.
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
A **crate** is Rust's unit of packaging &mdash; this project is one crate with one dependency, PyO3, which cargo fetches from crates.io on the first build.
`edition = "2024"` opts into a snapshot of language rules &mdash; think `from __future__ import ...` applied project-wide, not a compiler version.
The line doing the real work is `crate-type = ["cdylib"]`: build a *C dynamic library*, a shared library exposing C symbols &mdash; which is precisely what a CPython extension module is.

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

* `use pyo3::prelude::*;` is an import &mdash; `from pyo3.prelude import *`. Glob imports are frowned upon in Python; Rust *preludes* are curated for exactly this use.
* `///` comments are docstrings: PyO3 turns them into the `__doc__` of the module and function.
* `#[pymodule] mod pyo3_example { ... }` &mdash; `mod` declares a namespace, like a Python module but spelled out explicitly. The `#[pymodule]` **attribute** looks like a decorator and reads like one, but runs at *compile time*, generating the entry point CPython looks for when it executes `import pyo3_example`. This block is the Rust analog of "the code that runs at import."
* `#[pyfunction]` is the decorator-alike that exposes a Rust `fn` (a `def`) to Python.
* `fn sum_as_string(a: usize, b: usize) -> PyResult<String>` &mdash; type annotations that are actually *enforced*. `usize` is a machine-sized unsigned integer; Python's `int` is arbitrary-precision, so PyO3 converts at the boundary and raises `OverflowError` if a value is negative or too large. The `String` comes back out as an ordinary `str`.
* `Ok((a + b).to_string())` &mdash; Rust returns errors as values rather than raising. `PyResult<String>` means "a `str`, or a Python exception"; `Ok` wraps the success case, and returning an `Err` raises a genuine exception on the Python side &mdash; we will use that later in this chapter.
* There is no `return`: the last expression in a block is its value.

The `// ...` marks the rest of the file &mdash; a class and two more functions we will meet in the coming sections.

## Build and iterate

One command builds the extension and installs it into the environment:

```console
$ pixi run develop
✨ Pixi task (develop): maturin develop: (Build the Rust extension and install it into the pixi environment)
🐍 Found CPython 3.14 at /home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3/.pixi/envs/default/bin/python
🔗 Found pyo3 bindings
📡 Using build options features from pyproject.toml
   Compiling pyo3_example v0.1.0 (/home/matt/src/SIMPLE-Py-worktrees/rust/examples/2_04_rust_pyo3)
    Finished `dev` profile [unoptimized + debuginfo] target(s) in 0.14s
📦 Built wheel for CPython 3.14 to /tmp/.tmpyrBTrp/pyo3_example-0.1.0-cp314-cp314-linux_x86_64.whl
✏️ Setting installed package as editable
🛠 Installed pyo3_example-0.1.0
```

Note what you did *not* have to configure: maturin found the environment's interpreter on its own.
A pixi environment is a conda environment, so `maturin develop` detects it exactly as it would a virtualenv &mdash; compile, wrap in a wheel, install, done.
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
Its long filename suffix encodes the interpreter version, ABI, and platform it was built for &mdash; this one only loads on CPython 3.14 on x86-64 Linux &mdash; a contract that resurfaces when we ship wheels at the end of the chapter.

:::

This is the same editable workflow the book covers in [Editable installs](../scikit-build/04_editable_installs.md): maturin is a full PEP 660 backend, so `pip install -e .` works here too, and `maturin develop` is the convenient shorthand for it.
The one Rust-specific rule of the edit&ndash;rebuild loop: Python cannot re-import its way to your new Rust code.
After every change to `src/lib.rs`, rerun `pixi run develop`; if an edit ever seems to do nothing, the installed extension is stale &mdash; the fix is the same command.
