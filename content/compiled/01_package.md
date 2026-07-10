# A minimal compiled package with scikit-build

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

And that should be it! Try it:

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

## Make the package better

You don't have to do add anything more, but there are config settings that
really help, let's look at a few.

### Minimium-version

How do you ensure new users get good defaults, but old users don't break when defaults change? CMake handles this elegently:

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
(like non-normalized SDist names no longer being uploadabe on PyPI), then that
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

Just like hatchling (which we took a lot of insperation from!), the name matters! Your
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
your SDist against git as a source of truth, you can use `uvx check-dist`.
It's really useful for flit-core/setuptools, less so for scikit-build-core and
hatchling, due to the fact they tend to map to git by default.

If you need files from somewhere else, you can use `sdist.force-include`
(spelling slightly different in hatchling).

Try building an SDist, and list the files in it.

:::{callout}

* Do your tests go in the SDist? Yes.
* Do your docs go in the SDist? Ehh. Depends. Kindof.
* Do your CI files go in the SDist? No, but who cares, they are small.
:::

### Building the wheel

Figuring out what files go in the wheel is also a hard problem, though at least
it's better defined; only your package goes in, tests, docs, etc. do not.

Scikit-build-core includes everything in the auto-discovered or explicitly
named `wheel.packages`. It also contains anything CMake installs.

You can adjust quite a bit, though. For example, you can change what the install
targets:

```toml
[tool.scikit-build]
wheel.install-dir = "colatz"
```

```cmake
install(TARGETS _core DESTINATION .)
```

You can also use `wheel.force-include` to move things around, and
`wheel.exclude` to strip out items you don't want. There are controls over what
"components" CMake installs, which can allow you to pick a subset from CMake.

:::{callout}
Wheels have multiple directories that are handled differently, and are all
available using CMake (style) variables:

* `${SKBUILD_PLATLIB_DIR}`: The original platlib directory. Anything here goes directly to site-packages when a wheel is installed.
* `${SKBUILD_DATA_DIR}`: The data directory. Anything here goes to the root of the environment when a wheel is installed (use with care).
* `${SKBUILD_HEADERS_DIR}`: The header directory. Anything in here gets installed to Python’s header directory.
* `${SKBUILD_SCRIPTS_DIR}`: The scripts directory. Anything placed in here will go to bin (Unix) or Scripts (Windows).
* `${SKBUILD_METADATA_DIR}`: The dist-info directory. Licenses go in the licenses subdirectory.
* `${SKBUILD_NULL_DIR}`: Anything installed here will not be placed in the wheel.

:::

To build an wheel:

```bash
uv build --wheel
```

If you leave off the flags, it builds both, by the way. Unlike `python -m
build` — which builds the SDist and then builds the wheel _from_ that SDist —
`uv build` builds the two independently, both directly from the source
directory. If you want to build a wheel from an SDist (a good check that your
SDist is complete), pass the archive, such as: `uv build dist/collatz-0.1.0.tar.gz`.

To see what's in your wheel, use `unzip -l dist/*.whl`.

Try building a wheel, and list the files in it.
