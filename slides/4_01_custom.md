---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Customizing the SDist and wheel

---

## Two artifacts, lots of knobs

You've built both already; the defaults are good. This chapter is about the
controls when they aren't enough:

<div class="columns">
<div>

### SDist — what you build *from*

- Inclusion modes
- CMake at SDist time
- Symlinks
- Force include
- Reproducible (default **on**)

</div>
<div>

### wheel — what you *install*

- Packages
- Reproducible (default **off**)
- Force include
- Special dirs
- Install components

</div>
</div>

---

## SDist: include & exclude

Baseline: everything not `.gitignore`d. Adjust on top:

```toml
[tool.scikit-build]
sdist.include = ["src/some_generated_file.txt"]
sdist.exclude = [".github"]
```

`sdist.include` **wins over your ignore files** — the right way to ship a
generated file (a `setuptools_scm` version file, say) that you keep out of git.

---

## SDist: inclusion modes

```toml
[tool.scikit-build]
sdist.inclusion-mode = "explicit"
```

- **`default`** — process git ignore files; skips ignored directories entirely
  (fast even with a huge `build/`)
- **`classic`** — pre-0.12: walks into ignored dirs (an `include` can rescue
  files inside them)
- **`manual`** — no ignore files; everything in unless it matches `exclude`
- **`explicit`** *(1.0)* — nothing in unless it matches `include`; full
  flit-style control

---

## CMake in the SDist phase

`sdist.cmake = true` runs a configure while making the SDist — to **vendor** a
FetchContent download so the SDist builds offline:

```cmake
if(NOT SKBUILD_STATE STREQUAL "sdist"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pybind11/CMakeLists.txt")
  add_subdirectory(pybind11)   # vendored copy present: use it
else()
  FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.12.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pybind11)
  FetchContent_MakeAvailable(pybind11)
endif()
```

`SKBUILD_STATE` tells CMake which phase it's in (`sdist`, `wheel`,
`editable`, ...).

---

## ... and rescue the download

The downloaded `pybind11/` is in `.gitignore`, so pull the needed parts back in:

```toml
[tool.scikit-build]
sdist.cmake = true
sdist.include = [
  "pybind11/tools",
  "pybind11/include",
  "pybind11/CMakeLists.txt",
]
```

Checkout builds download; SDist builds use the vendored copy — no network.

---

## SDist: symlinks

A symlink pointing outside the project breaks once the SDist is extracted.
Since 1.0, symlinks are resolved into real files by default:

```toml
[tool.scikit-build]
sdist.resolve-symlinks = "all"  # the default (>= 1.0)
```

- **`all`** — resolve every symlink, copying its target's contents
- **`external`** — resolve only links pointing outside the project
- **`none`** — store every symlink as-is
- **`classic`** — 0.x behavior: keep file links, follow directory links

Monorepo pattern: `ln -s ../../shared src/shared` — same relative path works
from a checkout *and* an unpacked SDist.

---

## SDist: force include *(1.0)*

Include/exclude **filter** the tree; `force-include` **places** a file or
directory at a destination — even from outside the project:

```toml
[tool.scikit-build.sdist.force-include]
"../shared/data.json" = "mypackage/data.json"
```

- Sources relative to the project root; may be outside it, or absolute
- Directories copy recursively; a **missing source is an error**
- A force-included *file* beats a matching `exclude`; a *directory* can still
  be trimmed by one

---

## Reproducible SDists

Build twice → byte-identical archives. **On by default**: respects
`SOURCE_DATE_EPOCH`, locks timestamps to a fixed value if unset.

```toml
[tool.scikit-build]
sdist.reproducible = false  # if you'd rather keep real timestamps
```

---

## Wheel: customizing packages

Auto-discovery: `src/<name>`, `python/<name>`, `<name>`. Override with
`wheel.packages` — each entry is **one top-level package**, subpackages come
along:

```toml
[tool.scikit-build]
wheel.packages = ["src/pkg_a", "src/pkg_b"]
```

Table form remaps source → wheel path; `[]` leaves everything to CMake:

```toml
[tool.scikit-build.wheel.packages]
"mypackage/subpackage" = "python/src/subpackage"
```

`wheel.exclude` patterns match paths **in the wheel** — they apply to CMake
output too.

---

## Reproducible wheels *(1.0)*

Unlike SDists, **opt-in**:

```toml
[tool.scikit-build]
wheel.reproducible = true
```

Normalizes archive timestamps & permissions, and exports `SOURCE_DATE_EPOCH`
to the CMake build.

Why opt-in? The archive is the easy part — identical *binaries* also need a
recent compiler and flags like `-ffile-prefix-map`.

---

## Wheel: force include *(1.0)*

Same rules as the SDist table; destinations are relative to the **platlib**,
or prefix with a tree variable:

```toml
[tool.scikit-build.wheel.force-include]
"vendor/lib.so" = "mypackage/_lib.so"
"tools/run.sh" = "${SKBUILD_SCRIPTS_DIR}/run.sh"
```

Vendor into the SDist, then ship it — reference the *SDist destination* as the
wheel source and both build modes work:

```toml
[tool.scikit-build.sdist.force-include]
"../shared/data.json" = "mypackage/data.json"

[tool.scikit-build.wheel.force-include]
"mypackage/data.json" = "mypackage/data.json"
```

---

## Wheel: special dirs

Installers place each tree differently; CMake (and `pyproject.toml`) can
target them all:

- `${SKBUILD_PLATLIB_DIR}` — site-packages (the default)
- `${SKBUILD_DATA_DIR}` — root of the environment (use with care)
- `${SKBUILD_HEADERS_DIR}` — Python's header directory
- `${SKBUILD_SCRIPTS_DIR}` — `bin` (Unix) / `Scripts` (Windows)
- `${SKBUILD_METADATA_DIR}` — dist-info (licenses under `licenses/`)
- `${SKBUILD_NULL_DIR}` — dropped from the wheel

```toml
[tool.scikit-build]
wheel.install-dir = "${SKBUILD_DATA_DIR}/mypackage"
```

Quote it on a command line, or your shell eats `${...}`.

---

## Wheel: install components

Adding bindings to an existing CMake library? Its `install()` rules ship
headers and static libs you don't want in a wheel. Tag yours:

```cmake
install(TARGETS _core DESTINATION collatz COMPONENT python)
```

```toml
[tool.scikit-build]
install.components = ["python"]
```

Only the listed components land in the wheel — no touching the library's
install rules, no `wheel.exclude` cleanup after the fact.

---

## Summary

- SDist baseline is `.gitignore`; `sdist.include` **overrides** it, and
  `inclusion-mode` swaps the whole strategy
- `sdist.cmake = true` + `SKBUILD_STATE` **vendors downloads** into the SDist
- Symlinks resolve to real files by default (1.0) — monorepo-friendly
- `force-include` **places** files (even external ones) in either artifact
- Reproducible: SDists **on** by default, wheels **opt-in**
- `${SKBUILD_<TREE>_DIR}` targets scripts, headers, data... from CMake *or*
  `pyproject.toml`
- `install.components` picks a subset of an existing CMake install
