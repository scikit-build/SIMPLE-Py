# Modern Python: Free-threading

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/6_01_free_threading>`

For most of Python's history, the **Global Interpreter Lock** (GIL) has meant
that only one thread runs Python bytecode at a time. Threads are great for
overlapping I/O, but they can't use more than one core for computation --- for
that you reached for multiprocessing, with its pickling and process-startup
costs.

[PEP 703](https://peps.python.org/pep-0703/) changed that. Starting with Python
3.13 there is an official **free-threaded** build (sometimes written
`python3.14t`) where the GIL can be turned off, and threads run Python in
parallel on every core. 3.13 was the experimental debut; 3.14 is where it got
fast, so that's what we require here. It's the most interesting thing to happen
to CPython in years --- and it has real consequences for how we package compiled
extensions.

You can check whether the GIL is active at runtime:

```pycon
>>> import sys
>>> sys._is_gil_enabled()
False
```

## An embarrassingly parallel example

To see it work, we need a CPU-bound task that splits cleanly across threads.
Estimating $\pi$ by throwing darts is perfect: throw random points into the
square $[-1, 1]^2$ and count how many land inside the unit circle. The fraction
inside approaches $\pi/4$. Each dart is independent, so we can run a batch per
thread and average the results.

## Pure Python

The pure-Python version is a plain loop, run across a thread pool:

```{literalinclude} ../../examples/6_01_free_threading/pure/freecomputepi/pi.py
:language: python
```

On a normal (GIL-enabled) interpreter, adding threads doesn't help --- the GIL
serializes them, so you get one core's worth of work no matter what. On a
free-threaded build, the same code speeds up with each core.

Run it with a free-threaded interpreter (uv will fetch one for the `t` suffix):

```bash
uv run --python 3.14t sample.py
```

```{literalinclude} ../../examples/6_01_free_threading/pure/sample.py
:language: python
```

```text
Python 3.14.6, GIL disabled
 1 threads: pi = 3.14159  (2.22 s)
 2 threads: pi = 3.14158  (1.13 s)
 4 threads: pi = 3.14150  (0.60 s)
 8 threads: pi = 3.14201  (0.43 s)
```

Drop the `t` (`uv run --python 3.14`) and the times stay flat no matter how many
threads you add --- that's the GIL.

## Compiled: releasing the GIL for real

Pure Python is now parallel, but it's still Python-slow. The real win is a
compiled inner loop that runs in parallel _and_ fast. There's a catch: **an
extension has to declare that it doesn't need the GIL.** If you import any
extension that hasn't opted in, CPython silently switches the GIL back on (with
a warning) to keep that extension safe --- so every extension in your process
has to be free-threading-aware, or nobody gets the speedup.

The compute is identical to the pure version, just in C++. The interesting part
is the one line that marks the module as GIL-free --- and it differs between the
two tools:

::::{tab-set}

:::{tab-item} pybind11
:sync: pybind11

pybind11 marks the module in the `PYBIND11_MODULE` macro with
`py::mod_gil_not_used()` (available since pybind11 2.13):

```{literalinclude} ../../examples/6_01_free_threading/pybind11/freecomputepi/_core.cpp
:language: cpp
```

:::

:::{tab-item} nanobind
:sync: nanobind

nanobind opts in from CMake instead, via the `FREE_THREADED` flag on
`nanobind_add_module` (see the build config below) --- the module code stays
unchanged:

```{literalinclude} ../../examples/6_01_free_threading/nanobind/freecomputepi/_core.cpp
:language: cpp
```

:::

::::

:::{note}

Declaring the module GIL-free is a _promise_, not a shield. You're telling
CPython your extension has no unguarded shared state. Our `pi` only uses local
variables, so it's safe --- but a function with global caches or shared buffers
would need real locking (`std::mutex`, atomics, or nanobind's `nb::ft_mutex`)
before making that promise.

With the raw C API you make the same promise with a module slot:
`{Py_mod_gil, Py_MOD_GIL_NOT_USED}`.
:::

A thin Python wrapper spreads the work over a thread pool, exactly as the pure
version did --- it just imports `pi` from the compiled `_core` instead:

```{literalinclude} ../../examples/6_01_free_threading/pybind11/freecomputepi/pi.py
:language: python
```

### Build configuration

The CMake is a standard scikit-build-core extension build. nanobind is where the
free-threading opt-in lives (`FREE_THREADED`); pybind11 needs nothing special
here:

::::{tab-set}

:::{tab-item} pybind11
:sync: pybind11

```{literalinclude} ../../examples/6_01_free_threading/pybind11/CMakeLists.txt
:language: cmake
```

:::

:::{tab-item} nanobind
:sync: nanobind

```{literalinclude} ../../examples/6_01_free_threading/nanobind/CMakeLists.txt
:language: cmake
```

:::

::::

The `pyproject.toml` differs only in the binding tool it requires:

::::{tab-set}

:::{tab-item} pybind11
:sync: pybind11

```{literalinclude} ../../examples/6_01_free_threading/pybind11/pyproject.toml
:language: toml
```

:::

:::{tab-item} nanobind
:sync: nanobind

```{literalinclude} ../../examples/6_01_free_threading/nanobind/pyproject.toml
:language: toml
```

:::

::::

### Build and run

`uv run` builds the extension and runs the same benchmark:

```bash
uv run --python 3.14t sample.py
```

```text
Python 3.14.6, GIL disabled
 1 threads: pi = 3.14141  (0.26 s)
 2 threads: pi = 3.14199  (0.13 s)
 4 threads: pi = 3.14148  (0.07 s)
 8 threads: pi = 3.14149  (0.07 s)
```

Same near-linear scaling as pure Python, but an order of magnitude faster per
thread. pybind11 and nanobind produce identical timings --- the choice is about
the binding style, not the parallelism.

## Building wheels

Free-threaded wheels use a distinct ABI tag (`cp314t`), so they're separate
artifacts from the regular `cp314` wheels. [cibuildwheel](../compiled/04_cibuildwheel.md)
builds them for you --- as of 3.14 free-threading is no longer experimental, so
they're on by default with no `enable` needed:

```toml
[tool.cibuildwheel]
build = "cp314*"
```

The `cp314*` pattern matches both the `cp314` and `cp314t` identifiers, so each
job emits both a normal and a free-threaded wheel. Users on a free-threaded
interpreter automatically get the `t` wheel; the GIL stays off, and their
threads finally use every core.
