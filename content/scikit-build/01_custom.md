# Customizing the SDist and wheel

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/4_01_custom>`

There are a lot of controls to help you customize the output. We covered a few
of them in [the first compiled chapter](../compiled/01_compiled.md) —
`wheel.packages`, `wheel.install-dir`, and the special wheel directories. This
chapter goes through the rest, one distribution at a time: first what goes into
the SDist, then what goes into the wheel.

## SDist

Remember the baseline: scikit-build-core starts from "everything not
`.gitignore`d." Everything below adjusts that.

### Inclusion modes

On top of the `.gitignore` baseline, you can explicitly add or remove files:

```toml
[tool.scikit-build]
sdist.include = ["src/some_generated_file.txt"]
sdist.exclude = [".github"]
```

`sdist.include` wins over your ignore files — it's the right way to ship a
generated file (like a version file written by `setuptools_scm`) that you keep
out of git.

If the gitignore-based behavior isn't what you want at all, you can change the
mode:

```toml
[tool.scikit-build]
sdist.inclusion-mode = "manual"
```

The modes are:

- `"default"`: Process the git ignore files. Skips ignored directories
  entirely, so a huge ignored `build/` tree doesn't slow anything down.
- `"classic"`: The behavior before 0.12 — like `"default"`, but walks into
  ignored directories anyway (an `include` pattern can rescue files inside
  them).
- `"manual"`: Ignore files are not read; everything is included unless it
  matches `sdist.exclude`.
- `"explicit"`: Opt-in only. Nothing is included unless it matches an
  `sdist.include` pattern, and `exclude` is applied after. This is the mode for
  people who want full, flit-style control. (New in 1.0.)

### CMake in the SDist phase

Normally CMake only runs when building a wheel. You can also request a
configure step while producing the SDist:

```toml
[tool.scikit-build]
sdist.cmake = true
```

Why would you want that? To _vendor_ something into the SDist. If your
`CMakeLists.txt` downloads a dependency with `FetchContent` into the source
directory, running CMake at SDist time materializes it, so the SDist can build
without network access:

```cmake
include(FetchContent)

if(NOT SKBUILD_STATE STREQUAL "sdist"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pybind11/CMakeLists.txt")
  message(STATUS "Using integrated pybind11")
  add_subdirectory(pybind11)
else()
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.12.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pybind11)
  FetchContent_MakeAvailable(pybind11)
endif()
```

The `pybind11/` directory is in `.gitignore` (it's downloaded, not yours), so
the parts you need must be rescued with `sdist.include`:

```toml
[tool.scikit-build]
sdist.cmake = true
sdist.include = [
  "pybind11/tools",
  "pybind11/include",
  "pybind11/CMakeLists.txt",
]
```

Notice the `SKBUILD_STATE` check: the CMake variable tells you which phase you
are in (`sdist`, `wheel`, `editable`, ...), so the SDist configure can take the
download branch while a normal build uses the vendored copy if present.

### Symlinks

If part of your source tree is a symlink — common in monorepos, where a member
package links to a shared directory of native sources — a naive SDist would
store the link itself, which breaks as soon as it points outside the project.
Scikit-build-core resolves symlinks into real files by default:

```toml
[tool.scikit-build]
sdist.resolve-symlinks = "all"  # the default when minimum-version >= 1.0
```

The modes are:

- `"all"`: Resolve every symlink, copying its target's contents.
- `"external"`: Resolve only symlinks pointing outside the project (the ones
  that would be broken after extraction); keep internal ones as symlinks.
- `"none"`: Store every symlink as-is, including directory symlinks.
- `"classic"`: The 0.x behavior — store file symlinks as-is, but follow
  directory symlinks.

A symlink that can't be resolved (dangling, or a loop) is stored as a symlink
in every mode, with a warning if it was supposed to be resolved.

This gives a nice monorepo pattern: `ln -s ../../shared src/shared`, and the
same relative path works from a git checkout (through the link) and from an
unpacked SDist (real copied files).

`sdist.resolve-symlinks` is new in scikit-build-core 1.0; older versions
always behaved like `"classic"`.

### Force include

Include/exclude patterns filter the project tree; `force-include` instead
_places_ a specific file or directory at a specific path, even if the source
lives outside the project:

```toml
[tool.scikit-build.sdist.force-include]
"../shared/data.json" = "mypackage/data.json"
```

The keys are source paths relative to the project root (they may be outside
it, or absolute), and the values are destinations relative to the SDist root.
Directories are copied recursively, and a missing source is an error — if you
named it explicitly, silently skipping it would be a bug factory. A
force-included _file_ also beats a matching `sdist.exclude` pattern; a
force-included _directory_ can still be trimmed by excludes. (New in 1.0.)

### Reproducible

Build the same SDist twice and you should get byte-identical archives. That's
on by default: scikit-build-core respects [`SOURCE_DATE_EPOCH`], and locks
modification times to a fixed value if it's not set. You can turn it off if
you'd rather keep real timestamps:

```toml
[tool.scikit-build]
sdist.reproducible = false
```

[`SOURCE_DATE_EPOCH`]: https://reproducible-builds.org/docs/source-date-epoch/

:::{exercise} Rescue a generated file
:label: custom-sdist

Take your collatz package, add `generated.txt` to `.gitignore`, create the
file, and build an SDist. Confirm it's missing with `tar -tf dist/*.tar.gz`.
Then get it into the SDist _without_ removing it from `.gitignore`.

:::

:::{solution} custom-sdist
:class: dropdown

```toml
[tool.scikit-build]
sdist.include = ["generated.txt"]
```

Rebuild and `tar -tf dist/*.tar.gz` now lists it. `sdist.include` overrides the
ignore files — exactly the mechanism the `sdist.cmake` vendoring example relies
on. (`sdist.force-include` with `"generated.txt" = "generated.txt"` works too,
but that tool is for _relocating_ files; a pattern rescue only needs
`include`.)

:::

## Wheel

The wheel side has the same philosophy: sensible automatic behavior, with a
control for each piece.

### Customizing packages

As covered [before](../compiled/01_compiled.md), your package is discovered at
`src/<package_name>`, `python/<package_name>`, or `<package_name>`, and
`wheel.packages` overrides the search. Each entry names one top-level package —
subpackages and data files inside it come along automatically:

```toml
[tool.scikit-build]
wheel.packages = ["src/pkg_a", "src/pkg_b"]
```

The table form lets the source path and the wheel path differ (the final path
components must match, for editable installs to work):

```toml
[tool.scikit-build.wheel.packages]
"mypackage/subpackage" = "python/src/subpackage"
```

And `wheel.packages = []` disables Python file discovery entirely, leaving the
wheel contents to CMake's `install()` commands alone.

You can also trim the result — `wheel.exclude` patterns match paths _in the
wheel_ (so they apply to CMake output too, not just your package files):

```toml
[tool.scikit-build]
wheel.exclude = ["**.pyx"]
```

### Reproducible

Unlike SDists, reproducible wheels are opt-in:

```toml
[tool.scikit-build]
wheel.reproducible = true
```

When enabled, archive timestamps and file permissions are normalized, and
`SOURCE_DATE_EPOCH` is exported to the CMake build (if not already set) so
compilers that honor it can produce deterministic output. It's opt-in because
the hard part isn't the archive — it's the compiled binaries inside, which also
need a recent compiler and flags like `-ffile-prefix-map` to come out
identical. (New in 1.0.)

### Force include

The wheel has a `force-include` table too, with the same rules as the SDist
one (external/absolute sources allowed, missing source is an error, files beat
excludes, placed last so they override discovered files and CMake output):

```toml
[tool.scikit-build.wheel.force-include]
"vendor/lib.so" = "mypackage/_lib.so"
"tools/run.sh" = "${SKBUILD_SCRIPTS_DIR}/run.sh"
```

Destinations are relative to the platlib (the package area), and — as the
second line shows — a `${SKBUILD_<TREE>_DIR}` prefix targets one of the other
wheel trees (more on those below).

One subtlety worth knowing: a common pattern vendors an external file into the
SDist and then ships it in the wheel. Reference the _SDist destination_ as the
wheel source and it works in both build modes:

```toml
[tool.scikit-build.sdist.force-include]
"../shared/data.json" = "mypackage/data.json" # vendor it into the SDist

[tool.scikit-build.wheel.force-include]
"mypackage/data.json" = "mypackage/data.json" # ship the SDist output
```

Building from the unpacked SDist, the file is on disk and used directly.
Building from the source tree, it was never materialized — a missing
`wheel.force-include` source is resolved through `sdist.force-include` and read
from the original location instead. For layouts that trick can't express,
[overrides](./05_overrides.md) keyed on `from-sdist` handle the two build modes
separately. (Force-include is new in 1.0.)

### Special dirs

A wheel isn't just site-packages; it has several trees that installers place
differently, and scikit-build-core exposes each as a CMake variable:

- `${SKBUILD_PLATLIB_DIR}`: The platlib directory. Anything here goes directly
  to site-packages when a wheel is installed. Installs go here by default.
- `${SKBUILD_DATA_DIR}`: The data directory, placed at the root of the
  environment (use with care).
- `${SKBUILD_HEADERS_DIR}`: The header directory, installed to Python's header
  directory.
- `${SKBUILD_SCRIPTS_DIR}`: The scripts directory, placed in `bin` (Unix) or
  `Scripts` (Windows).
- `${SKBUILD_METADATA_DIR}`: The dist-info directory. Licenses go in the
  `licenses` subdirectory.
- `${SKBUILD_NULL_DIR}`: Anything installed here will not be placed in the
  wheel — handy for targets CMake insists on installing that you don't want.

You can use these from CMake in `install()` destinations, but the same
spelling also works in `pyproject.toml`: both `wheel.install-dir` and
`wheel.force-include` destinations accept a `${SKBUILD_<TREE>_DIR}` prefix.
For example, to graft the whole CMake install tree into the data dir:

```toml
[tool.scikit-build]
wheel.install-dir = "${SKBUILD_DATA_DIR}/mypackage"
```

:::{warning}
When passing this through `config-settings` on a command line, quote it so
your shell doesn't expand `${SKBUILD_DATA_DIR}` as an environment variable:
`-C 'wheel.install-dir=${SKBUILD_DATA_DIR}/mypackage'`.
:::

### Install components

CMake's `install()` commands can be tagged with a `COMPONENT`, and
scikit-build-core lets you install just the components you list:

```toml
[tool.scikit-build]
install.components = ["python"]
```

If unset (or an empty list), all default components are installed. This is the
clean solution when you're adding Python bindings to an existing CMake library:
the library's `install()` rules ship headers, static libraries, and CMake
config files you don't want in a wheel. Tag your binding's install rules:

```cmake
install(TARGETS _core DESTINATION collatz COMPONENT python)
```

and only that component lands in the wheel — no need to touch the existing
install rules or filter with `wheel.exclude` after the fact.

:::{exercise} Ship a script in the scripts directory
:label: custom-wheel

Add a small `tools/collatz-hello` shell script to your collatz package, and get
it into the wheel's scripts tree so installers put it on the user's `PATH`.
Check with `unzip -l dist/*.whl` — it should appear under
`collatz-0.1.0.data/scripts/`.

:::

:::{solution} custom-wheel
:class: dropdown

No CMake needed — force-include it into the scripts tree:

```toml
[tool.scikit-build.wheel.force-include]
"tools/collatz-hello" = "${SKBUILD_SCRIPTS_DIR}/collatz-hello"
```

Or, from the CMake side:

```cmake
install(PROGRAMS tools/collatz-hello DESTINATION ${SKBUILD_SCRIPTS_DIR})
```

Either way, `unzip -l dist/*.whl` shows
`collatz-0.1.0.data/scripts/collatz-hello`, and installers move it to
`bin`/`Scripts` and make it executable. (For Python entry points, prefer
`[project.scripts]` — this tree is for real files, like a compiled executable.)

:::

## See also

The scikit-build-core docs have the full story: the [configuration
guide](https://scikit-build-core.readthedocs.io/en/latest/configuration/) and
the [complete settings
list](https://scikit-build-core.readthedocs.io/en/latest/reference/configs.html).
