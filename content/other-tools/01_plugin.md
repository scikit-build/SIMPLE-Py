# Using scikit-build-core as a plugin

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/5_01_plugin>`

Scikit-build-core is primarily a native build-backend, but you can also use it
to create plugins for other build backends. We have two built-in: hatchling
and setuptools. Hatchling was added by popular request, and setuptools exists
mostly to allow us to replace the backend of scikit-build (classic). Both are
useful, though, if you want to add a compiled extension to an existing
project, or if a project is mostly Python, or if you need to combine plugins
(such as building mypyc + C++).

The plugins reuse the same CMake machinery and the same `[tool.scikit-build]`
configuration as the standalone backend, so most of what you learned in the
[scikit-build chapters](../scikit-build/01_custom.md) carries over. The
difference is who _owns_ the build: the host backend builds the wheel and SDist,
and scikit-build-core contributes the compiled bits as a hook.

## Hatchling

[Hatchling](https://hatch.pypa.io) is the build backend behind Hatch, and it
has a nice plugin system. To add compiled extensions to a Hatchling project,
put `scikit-build-core[hatchling]` in your
`build-system.requires`, then enable the plugin by adding the
`tool.hatch.build.targets.wheel.hooks.scikit-build` table:

```{code} toml
:filename: pyproject.toml

[build-system]
requires = ["scikit-build-core[hatchling]"]
build-backend = "hatchling.build"

[project]
name = "hatchling_example"
version = "0.1.0"

[tool.hatch.build.targets.wheel.hooks.scikit-build]
```

:::{tip}

Always use the `[hatchling]` extra (rather than depending on `hatchling`
yourself). It pins a compatible version and protects you if the plugin ever
moves to a separate package. You'll see a warning if you don't.

:::

The empty `hooks.scikit-build` table is enough to switch the plugin on; that is
where you'd point CMake at a non-default source directory or set other options.

Configuration lives in either
`tool.hatch.build.targets.wheel.hooks.scikit-build` or the usual
`tool.scikit-build` table (or environment variables). A few backend features
don't apply, because hatchling owns that part of the build:

- `wheel.cmake` must stay on — there's no pure-Python (`platlib = false`) build.
- Building CMake during the SDist step isn't supported.
- `scikit-build.generate` and `scikit-build.metadata` are handled by hatchling's
  own plugins instead.
- Config-settings (`-C...`) aren't available, since hatchling doesn't pass them
  to plugins.

Inside `CMakeLists.txt`, the hatchling version is available as
`${SKBUILD_HATCHLING}`.

### A full example

Here's a mostly-Python package that adds a single C extension. The Python lives
in a normal `src` layout, and the compiled bits sit in their own `cpp`
directory:

```text
hatchling
├── pyproject.toml
├── cpp
│   ├── CMakeLists.txt
│   └── example.c
└── src
    └── hatchling_example
        └── __init__.py
```

```{literalinclude} ../../examples/5_01_plugin/hatchling/pyproject.toml
:language: toml
:caption: pyproject.toml
```

`cmake.source-dir` points at the `CMakeLists.txt`, and `wheel.install-dir` sends
the CMake output into the package directory so `_core` lands next to
`__init__.py`.

```{literalinclude} ../../examples/5_01_plugin/hatchling/cpp/CMakeLists.txt
:language: cmake
:caption: cpp/CMakeLists.txt
```

```{literalinclude} ../../examples/5_01_plugin/hatchling/cpp/example.c
:language: c
:caption: cpp/example.c
```

```{literalinclude} ../../examples/5_01_plugin/hatchling/src/hatchling_example/__init__.py
:language: python
:caption: src/hatchling_example/__init__.py
```

## Setuptools

Put `scikit-build-core[setuptools]` in `build-system.requires`. You can keep
`setuptools.build_meta` as the backend, but using
`scikit_build_core.setuptools.build_meta` gets you the auto-inclusion of `cmake`
and `ninja` and config-settings, so it's recommended if you can:

```{code} toml
:filename: pyproject.toml

[build-system]
requires = ["scikit-build-core[setuptools]"]
build-backend = "scikit_build_core.setuptools.build_meta"
```

The plugin only activates with a `cmake.source-dir` setting, or with the
classic `cmake_source_dir` keyword in a minimal `setup.py`:

::::{tab-set}

:::{tab-item} pyproject.toml (recommended)

```{code} toml
:filename: pyproject.toml

[tool.scikit-build]
cmake.source-dir = "."
```

:::

:::{tab-item} setup.py

```{code} python
:filename: setup.py

from setuptools import setup

setup(cmake_source_dir=".")
```

:::

::::

Everything else configures through `[tool.scikit-build]` as normal.
Config-settings (`-C cmake.build-type=Debug`) works when building through the
PEP 517 backend, but not through `setuptools.build_meta` or a direct `setup.py`
invocation. A handful of classic `setup.py` keywords (`cmake_args`,
`cmake_install_dir`, `cmake_install_target`, …) are still honored for
compatibility.

Since setuptools owns the SDist and editable machinery, those follow
setuptools' rules: the SDist file list comes from setuptools (only
`sdist.inclusion-mode = "explicit"` is supported), and editable installs
(`pip install -e .`) require opting in with `editable.mode = "inplace"`, since
they write build artifacts into your source tree.

### A full example

The same package as above, but on setuptools and configured entirely from
`pyproject.toml` — no `setup.py`:

```text
setuptools
├── pyproject.toml
├── CMakeLists.txt
├── example.c
└── src
    └── setuptools_example
        └── __init__.py
```

```{literalinclude} ../../examples/5_01_plugin/setuptools/pyproject.toml
:language: toml
:caption: pyproject.toml
```

Setuptools still discovers the Python package (here via its normal `find`
config), while `cmake.source-dir` activates the plugin and CMake installs
`_core` into the package. The `editable.mode = "inplace"` line is what lets
`pip install -e .` (or `uv run`) work, since the setuptools plugin only supports
in-place editable installs.

```{literalinclude} ../../examples/5_01_plugin/setuptools/CMakeLists.txt
:language: cmake
:caption: CMakeLists.txt
```

The `example.c` and `__init__.py` are identical to the hatchling example.

## About plugins and build systems

The plugins exist because no single backend is the right answer for every
project. A backend "owns" the build: it produces the SDist and wheel, and it
decides which hooks get to contribute files along the way. Scikit-build-core as
a plugin is one such hook — it runs CMake and drops the resulting extension
modules into the wheel — while hatchling or setuptools handles the metadata,
the Python sources, and the packaging.

That split is exactly what you want when:

- **The project is mostly Python.** Keep your existing hatchling/setuptools
  setup and add compiled bits only where you need them.
- **You need to combine build steps.** For example, a hatchling hook for mypyc
  alongside the scikit-build hook for C++.
- **You're migrating.** Setuptools projects (including scikit-build classic) can
  move onto CMake incrementally without rewriting their packaging.

If you're starting fresh and the project is primarily a compiled extension, the
standalone `scikit_build_core.build` backend is still the simplest path — you
only reach for the plugins when you need to live inside another backend's world.
