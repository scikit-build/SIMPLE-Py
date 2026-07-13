# A minimal compiled package with scikit-build

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/2_01_compiled>`

You've seen how to build a package already. Let's try a compiled package now!

## First simple package

First, you'll need a file to compile. C++, C, Fortran, Rust, etc. You also need
to pick a binding tool; if you are in C, you can use the C-API, but don't --
it's verbose, and you'll have to manage reference counting; binding tools
handle this for you.

Let's use a trivial pybind11 extension. Name the source file `collatz.cpp`:

```c++
#include <pybind11/pybind11.h>

// Even -> n/2, odd -> 3n+1. Conjectured to always terminate.
int collatz_steps(long long n) {
  int steps = 0;
  while (n != 1) {
    n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    ++steps;
  }
  return steps;
}

PYBIND11_MODULE(collatz, m) {
  m.def("collatz_steps", &collatz_steps, "Steps to reach 1 in the Collatz sequence");
}
```

It's a tight integer loop, something that's fast in a compiled language.
Hopefully you noticed that we set a module name, `collatz`, in
`PYBIND11_MODULE`. Every CPython module needs to know the name it will be
compiled to, since that's also how you look up the main entry point into the
code.

pybind11 isn't the only choice. [nanobind](https://nanobind.readthedocs.io) is
a lighter, faster tool from the same author, and it also builds with
CMake, so everything below works with a one-line swap. The binding itself looks
almost identical:

::::{tab-set}

:::{tab-item} pybind11

```c++
#include <pybind11/pybind11.h>

PYBIND11_MODULE(collatz, m) {
  m.def("collatz_steps", &collatz_steps);
}
```

:::

:::{tab-item} nanobind

```c++
#include <nanobind/nanobind.h>

NB_MODULE(collatz, m) {
  m.def("collatz_steps", &collatz_steps);
}
```

:::

::::

We'll stick with pybind11 for this chapter. The [next chapter](./02_binding.md)
compares the binding tools (including Rust via PyO3) in depth; here we care
about the _packaging_ around them, which is the same either way.

Now, you need a `pyproject.toml`. We use `scikit-build-core` as the build
backend, and list `pybind11` so CMake can find it while building:

```toml
[build-system]
requires = ["scikit-build-core", "pybind11"]
build-backend = "scikit_build_core.build"

[project]
name = "collatz"
version = "0.1.0"
```

And, a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.15...4.3)
project(collatz LANGUAGES CXX)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(collatz collatz.cpp)
install(TARGETS collatz DESTINATION .)
```

And that should be it! Before you run it, take a guess: starting from `27`, how
many steps do you think the sequence takes to reach `1`? (The number climbs to
`9232` on the way, so don't feel bad if you're off.) Now try it:

```console
$ uv run python
>>> from collatz import collatz_steps
>>> collatz_steps(27)
111
```

:::{exercise} Build the collatz package
:label: pkg-build

Put the three files above (`collatz.cpp`, `pyproject.toml`, `CMakeLists.txt`)
in an empty directory, then get a working import.

1. Run the module and confirm `collatz_steps(27)` returns `111`.
2. What is the largest number of steps you can find for a starting value
   under 100?

:::

:::{solution} pkg-build
:class: dropdown

```console
$ uv run python -c "from collatz import collatz_steps; print(collatz_steps(27))"
111
```

`uv run` sees the `pyproject.toml`, builds the extension with
scikit-build-core, and installs it into a temporary environment before running
your command — no manual compile step. For the second part, `97` takes 118
steps, the most under 100.

:::

## Was it worth it?

We claimed a tight integer loop is "fast in a compiled language." Let's check.
Here's the same function in pure Python:

```python
def collatz_steps(n):
    steps = 0
    while n != 1:
        n = n // 2 if n % 2 == 0 else 3 * n + 1
        steps += 1
    return steps
```

`timeit` makes the comparison easy — point it at each version and let it pick a
sensible number of loops:

```console
$ python -m timeit -s "from collatz import collatz_steps" "collatz_steps(97)"
1000000 loops, best of 5: 210 nsec per loop

$ python -m timeit -s "from mymodule import collatz_steps" "collatz_steps(97)"
100000 loops, best of 5: 5.8 usec per loop
```

Exact numbers depend on your machine, but the shape holds: the compiled version
is roughly **20–30× faster** here. That gap is the whole reason to reach for a
compiled extension — and it's why the loop lives in C++ while the packaging,
which runs once, stays in Python.

:::{exercise} Benchmark it yourself
:label: pkg-bench

Save the pure-Python version as `mymodule.py`, then run both `timeit` lines
above. How does the ratio change if you benchmark a _cheaper_ input like
`collatz_steps(1)` instead of `collatz_steps(97)`? Why?

:::

:::{solution} pkg-bench
:class: dropdown

With `collatz_steps(1)` the loop body never runs, so you're mostly timing the
call overhead — crossing the Python↔C boundary. The compiled version still
wins, but by much less, because there's no hot loop for it to speed up. The
lesson: compiled extensions pay off when there's real work _inside_ the call,
not for tiny wrappers around trivial operations.

:::

## Make the package better

You don't have to add anything more, but there are config settings that
really help, let's look at a few.

### Minimum version

How do you ensure new users get good defaults, but old users don't break when defaults change? CMake handles this elegantly:

```cmake
cmake_minimum_required(VERSION 3.15)
```

That 3.15 is special: it's the _minimum version_ of CMake supported. It doesn't
just add an error if CMake is too old, though; it actually changes the defaults
(called Policies). It even handles removals; you can still use
`FindPythonInterp`/`FindPythonLibs` if it's set below 3.26, but if it's 3.27 or
higher, those modules are no longer available.

(There's also an optional upper number: that lets the minimum version float up
to the upper number, based on the CMake that's running. Using it extends the
lifespan of your code, but does mean you should test once on either end of the
range. Update it as you test on newer versions.)

Scikit-build-core adopts a similar mechanism:

```toml
[tool.scikit-build]
minimum-version = "1.0"
```

Any changes we make are gated behind this version; if you set it, then we will
continue to behave identically. But if you increase it (or leave it unset), the
behavior will change to the latest recommendations. For example, we improved
the SDist inclusion mode to make it more useful and faster, but if you set
something older than `"0.12"` here, you'll get the old mode.

If we have to change something due to PyPI or some other tool we don't control
(like non-normalized SDist names no longer being uploadable on PyPI), then that
won't be gated by this value.

Since you should also provide a minimum in your build system requirements,
there's a great trick you can use to avoid repeating yourself:

```text
[build-system]
requires = ["scikit-build-core>=1.0"]
build-backend = "scikit_build_core.build"

[tool.scikit-build]
minimum-version = "build-system.requires"
```

Setting it to this special string will read the value from the `requires` list.

### Structuring the files

Just like pure Python packages (though probably even more important), you can use `src` layout:

```text
example
├── pyproject.toml
├── CMakeLists.txt
└── src
    └── example
        ├── __init__.py
        └── _core.cpp
```

This ensures that Python doesn't pick up the local folder if you run `import
example` inside the project directory. This is especially important for
compiled code, since you (and your tests, etc) can't run the uncompiled version!

Just like hatchling (which we took a lot of inspiration from!), the name matters! Your
package name must match the project name, and be in `/`, `/src`, or `/python`. If not,
you need to tell it where to discover it:

```toml
[tool.scikit-build]
wheel.packages = ["some/path/to/package"]
```

The final directory name is the package. If you need more complex structure, a
table is supported here as well.

:::{exercise} Move collatz into src layout
:label: pkg-src

Restructure the package from the first exercise so the source lives in
`src/collatz/`. Add a `src/collatz/__init__.py` that re-exports
`collatz_steps`, and compile the extension as a submodule so both
`import collatz` and `from collatz import collatz_steps` still work.

:::

:::{solution} pkg-src
:class: dropdown

```text
example
├── pyproject.toml
├── CMakeLists.txt
└── src
    └── collatz
        ├── __init__.py
        └── _core.cpp
```

Rename the module to `_core` in the C++ file, so it builds as
`collatz._core`:

```c++
PYBIND11_MODULE(_core, m) {
  m.def("collatz_steps", &collatz_steps, "Steps to reach 1 in the Collatz sequence");
}
```

Re-export it from `__init__.py`:

```python
from ._core import collatz_steps

__all__ = ["collatz_steps"]
```

And install the target into the package directory instead of the root:

```cmake
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_core src/collatz/_core.cpp)
install(TARGETS _core DESTINATION collatz)
```

scikit-build-core discovers `src/collatz/` automatically because the package
name matches the project name.

:::

:::{caution} Three names that must line up

The most common beginner error is a mismatch between three separate names.
Getting any one wrong gives an unhelpful `ImportError` at runtime, not a build
failure:

1. The name in `PYBIND11_MODULE(name, m)` (or `NB_MODULE`) — this is what the
   compiled `.so`/`.pyd` is called.
2. The `install(TARGETS ... DESTINATION ...)` path — where CMake puts it in the
   wheel.
3. The name you `import` in Python.

In the src-layout above, the module is `_core`, it installs to `collatz/`, and
you import it as `collatz._core`. If you rename the C++ module but forget the
`__init__.py` re-export, `from collatz import collatz_steps` breaks even though
the build succeeded. When something imports oddly, check these three first.

:::

### The two distributions

A package ships as two artifacts, and it's worth seeing how they relate before
we build each one. The {term}`SDist` is the source you build _from_; the
{term}`wheel` is the pre-built result you _install_:

```{mermaid}
flowchart LR
  src["source tree<br/>collatz.cpp<br/>CMakeLists.txt<br/>pyproject.toml"]
  sdist["SDist<br/>collatz-0.1.0.tar.gz"]
  wheel["wheel<br/>collatz-0.1.0-*.whl"]
  site["site-packages<br/>(installed)"]
  src -->|"uv build --sdist"| sdist
  src -->|"uv build --wheel"| wheel
  sdist -.->|"build from sdist"| wheel
  wheel -->|"pip install"| site
```

The two differ in what they contain: an SDist keeps build inputs (source,
`CMakeLists.txt`, tests), while a wheel keeps only the runtime package plus the
compiled extension. Deciding what lands in each is the tricky part, so let's
take them one at a time.

### Building the SDist

An SDist is the source of your package, and it needs to contain everything
required to build a wheel — but _everything required_ is surprisingly hard to
pin down automatically. This is one of the main differences between backends.

Scikit-build-core and hatchling start by including anything not `.gitignored`,
and they include your `.gitignore`. That's a good baseline, doesn't require
git, and enables sdists to recreate themselves if unpacked. Then there are other
ways to add/remove files (and scikit-build-core includes other modes to pick from).

flit-core includes a few common files and your package, and otherwise requires explicit
includes/excludes. Setuptools is similar, but uses `MANIFEST.in`.

To build an sdist:

```bash
uv build --sdist
```

To see what's in your SDist, use `tar -tf dist/*.tar.gz`. To measure what's in
your SDist against git as a source of truth, you can use `uvx check-sdist`.
It's really useful for flit-core/setuptools, less so for scikit-build-core and
hatchling, due to the fact they tend to map to git by default.

If you need files from somewhere else, you can use `sdist.force-include`
(spelling slightly different in hatchling).

:::{note}

* Do your tests go in the SDist? Yes.
* Do your docs go in the SDist? Ehh. Depends. Kindof.
* Do your CI files go in the SDist? No, but who cares, they are small.
:::

:::{exercise} Build and inspect the SDist
:label: pkg-sdist

Build an SDist for your collatz package and list its contents:

```bash
uv build --sdist
tar -tf dist/*.tar.gz
```

Which files ended up inside? Was anything included that surprised you — or
missing that you expected?

:::

:::{solution} pkg-sdist
:class: dropdown

You should see your source (`collatz.cpp` or `src/collatz/`), `CMakeLists.txt`,
`pyproject.toml`, and a generated `PKG-INFO`. Because scikit-build-core starts
from "everything not `.gitignore`d," a stray file in your working tree (a scratch
script, a `.venv` you forgot to ignore) can sneak in — which is exactly why the
`.gitignore` baseline matters. Notably, your build artifacts (`dist/`,
`build/`) should _not_ appear, since they're git-ignored.

:::

### Building the wheel

Figuring out what files go in the wheel is also a hard problem, though at least
it's better defined; only your package goes in, tests, docs, etc. do not.

Scikit-build-core includes everything in the auto-discovered or explicitly
named `wheel.packages`. It also contains anything CMake installs.

You can adjust quite a bit, though. For example, `wheel.install-dir` sets where
CMake's install tree is grafted into the wheel, so you can keep `DESTINATION .`
in CMake and redirect everything from one place:

```toml
[tool.scikit-build]
wheel.install-dir = "collatz"
```

```cmake
install(TARGETS _core DESTINATION .)
```

Now the target installed to `.` lands in `collatz/` inside the wheel. Be
careful: paths here are relative to the wheel root, and an absolute path is an
error.

You can also use `wheel.force-include` to move things around, and
`wheel.exclude` to strip out items you don't want. There are controls over what
"components" CMake installs, which can allow you to pick a subset from CMake.

:::{note}
Wheels have multiple directories that are handled differently, and are all
available using CMake (style) variables:

* `${SKBUILD_PLATLIB_DIR}`: The original platlib directory. Anything here goes directly to site-packages when a wheel is installed.
* `${SKBUILD_DATA_DIR}`: The data directory. Anything here goes to the root of the environment when a wheel is installed (use with care).
* `${SKBUILD_HEADERS_DIR}`: The header directory. Anything in here gets installed to Python’s header directory.
* `${SKBUILD_SCRIPTS_DIR}`: The scripts directory. Anything placed in here will go to bin (Unix) or Scripts (Windows).
* `${SKBUILD_METADATA_DIR}`: The dist-info directory. Licenses go in the licenses subdirectory.
* `${SKBUILD_NULL_DIR}`: Anything installed here will not be placed in the wheel.

:::

To build a wheel:

```bash
uv build --wheel
```

If you leave off the flags, it builds both, by the way. Unlike `python -m
build` — which builds the SDist and then builds the wheel _from_ that SDist —
`uv build` builds the two independently, both directly from the source
directory. If you want to build a wheel from an SDist (a good check that your
SDist is complete), pass the archive, such as: `uv build dist/collatz-0.1.0.tar.gz`.

To see what's in your wheel, use `unzip -l dist/*.whl`.

:::{exercise} Build and inspect the wheel
:label: pkg-wheel

Build a wheel and list its contents:

```bash
uv build --wheel
unzip -l dist/*.whl
```

Compare it to the SDist from the previous exercise. What's _in_ the wheel that
wasn't obvious from your source tree, and what's _missing_ that was in the SDist?

:::

:::{solution} pkg-wheel
:class: dropdown

The wheel contains your compiled extension (a platform-tagged `.so` or `.pyd` —
note the wheel filename encodes your platform and Python version), the package's
`__init__.py`, and a `collatz-0.1.0.dist-info/` metadata directory. It does
_not_ contain `CMakeLists.txt`, `collatz.cpp`, or any tests — those are build
inputs, not runtime files. That's the core difference: the SDist is what you
build _from_, the wheel is what you install.

:::

### Iterating: the full compiled edit loop

So far each change meant a fresh build. For real development you want an
_editable_ install so Python picks up your package in place — and for compiled
code, scikit-build-core can even rebuild the extension automatically when you
re-import it. That's covered in [its own chapter](../scikit-build/04_editable_installs.md);
the short version is:

```bash
uv pip install --no-build-isolation -e .
```

When a build goes wrong, two knobs help you see what happened:

```toml
[tool.scikit-build]
build.verbose = true      # show the full compiler command lines
cmake.build-type = "Debug"
```

You can also pass CMake defines through the build without editing files, which
is handy for one-off experiments:

```bash
uv build --wheel -C cmake.define.CMAKE_CXX_STANDARD=20
```

:::{exercise} Add a second function
:label: pkg-extend

Extend the extension with `collatz_max(n)` that returns the _largest_ value the
sequence reaches (the peak, not the number of steps). Rebuild, re-import, and
confirm `collatz_max(27)` returns `9232`. Don't forget to re-export it from
`__init__.py` if you're using the src layout.

:::

:::{solution} pkg-extend
:class: dropdown

Add the function and a second binding in the C++ file:

```c++
long long collatz_max(long long n) {
  long long peak = n;
  while (n != 1) {
    n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    peak = n > peak ? n : peak;
  }
  return peak;
}

PYBIND11_MODULE(_core, m) {
  m.def("collatz_steps", &collatz_steps);
  m.def("collatz_max", &collatz_max);
}
```

If you're on the src layout, add `collatz_max` to the `__init__.py` re-export
and its `__all__`. Then rebuild — `collatz_max(27)` returns `9232`, the peak we
mentioned back when you first ran `collatz_steps(27)`.

:::

## Glossary

:::{glossary}

SDist
: Source distribution — a `.tar.gz` containing everything needed to build the
package. You build a wheel _from_ it.

wheel
: A pre-built, installable archive. For compiled packages it's platform- and
Python-specific, which is encoded in the filename tags.

platlib
: The platform-specific library directory inside a wheel. Its contents install
straight into site-packages; it's where your compiled extension goes.

policy
: A CMake versioned behavior switch. `cmake_minimum_required` selects a policy
set, so raising the minimum version opts into newer defaults all at once.

editable install
: An install that points at your working tree instead of copying files, so
source edits take effect without reinstalling. scikit-build-core can also
rebuild the compiled part on import.

:::
