---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Making a basic package

---

## The goal for this hour

Build a package **by hand**, so every file holds no mysteries:

- Standard `src` layout
- Installs (editably) into an environment
- Complete, standards-based metadata
- A command-line script
- Version from git
- Builds into an SDist and a wheel

_Next chapter: the template that generates all of this for you._

---

## The starting point

A function in a notebook or script:

```python
import numpy as np


def rescale(input_array):
    """Rescale an array so its values span [0, 1]."""
    low = np.min(input_array)
    high = np.max(input_array)
    return (input_array - low) / (high - low)
```

Works fine — but you can't `pip install` it, `import` it elsewhere, or share it.

---

## The layout

```text
rescale
├── pyproject.toml
└── src
    └── rescale
        ├── __init__.py
        └── core.py
```

`__init__.py` marks the package and defines the public API:

```python
from rescale.core import rescale

__all__ = ["rescale"]
```

---

## Why a `src` layout?

Without it, `python` and `pytest` import the **local folder**, not the
installed package:

- Hides packaging bugs (forgotten files) until a _user_ hits them
- `src` forces everything through a real install
- Matches how compiled projects are laid out — pays off later!

---

## The pyproject.toml

```toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "rescale"
version = "0.1.0"
dependencies = ["numpy"]
```

- `[build-system]` selects a **build backend** — the tool that turns source into something installable
- `name` = install name, `src/rescale` = import name — keep them matched

---

## Build backends

All read the same standard `[project]` table — switching is easy:

| Backend        | Notes                                             |
| -------------- | ------------------------------------------------- |
| **hatchling**  | Great default: fast, extendable, respects git     |
| **uv_build**   | uv's own; very fast, intentionally minimal        |
| **flit-core**  | Tiny, dependency-free; used by core PyPA tools    |
| **setuptools** | The classic; modern versions read `[project]` too |

They differ in file selection, dynamic versioning, and extras.

---

## Install it and use it

**High level:**

```bash
uv run python
>>> from rescale import rescale
```

**Low level:**

```bash
uv venv
uv pip install -e .
```

`-e` = editable: edits to `src/` are visible on the next `import`.

---

## 🧑‍💻 Hands on: build the package

Create the `rescale` package:

- `src` layout, the two Python files, minimal `pyproject.toml`
- `git init` and commit
- Import it and rescale `numpy.linspace(0, 100, 5)`

**Bonus:** delete `dependencies = ["numpy"]`, remove `.venv` + `uv.lock`,
and see what breaks.

---

## A first test

```python
# tests/test_core.py
import numpy as np

from rescale import rescale


def test_rescale():
    np.testing.assert_allclose(
        rescale(np.linspace(0, 100, 5)),
        np.array([0.0, 0.25, 0.5, 0.75, 1.0]),
    )
```

```toml
[dependency-groups]
dev = ["pytest"]
```

```bash
uv run pytest
```

---

## Metadata

`name` + `version` are required; a real package says much more.

Same standard `[project]` table for **every** backend:

- **Informational** — description, readme, authors, license, URLs, classifiers
- **Functional** — `requires-python`, dependencies, extras, entry points

---

## Informational metadata

```toml
[project]
name = "rescale"
version = "0.1.0"
description = "Rescale NumPy arrays to span [0, 1]."
readme = "README.md"
authors = [{ name = "My Name", email = "me@email.com" }]
license = "BSD-3-Clause"
license-files = ["LICENSE"]
classifiers = [
  "Development Status :: 3 - Alpha",
  "Private :: Do Not Upload",
]

[project.urls]
Homepage = "https://github.com/me/rescale"
```

---

## License and classifiers

**License:** modern form is an SPDX expression

- `"BSD-3-Clause"`, `"MIT AND (Apache-2.0 OR BSD-2-Clause)"`
- Older style: `License ::` classifiers — don't mix old and new
- Never write your own license — pick one at choosealicense.com

**Classifiers:** tags from a fixed list on PyPI

- `Private :: Do Not Upload` makes PyPI **reject** the package
- Perfect for tutorials — remove it when you mean to publish

---

## Functional metadata

```toml
[project]
requires-python = ">=3.10"
dependencies = ["numpy>=1.24"]

[project.optional-dependencies]
plot = ["matplotlib"]
```

- `requires-python`: lets installers **back-solve** for old Pythons — **never upper-cap it**
- `dependencies`: lower bounds you test; avoid upper caps; pins belong in lockfiles
- extras: users opt in with `pip install 'rescale[plot]'`

---

## `project.dependencies` vs. `build-system.requires`

<div class="columns">
<div>

### `build-system.requires`

- Installed into a **temporary** isolated env while building
- Thrown away after
- Never reaches your users
- (A wheel doesn't even contain `pyproject.toml`!)

</div>
<div>

### `project.dependencies`

- Becomes wheel **metadata**
- Installers pull these in whenever someone installs you

</div>
</div>

---

## Entry points: a command line script

```toml
[project.scripts]
rescale = "rescale.__main__:main"
```

```python
# src/rescale/__main__.py
def main() -> None: ...
```

- Installer generates a real `rescale` executable in `bin/`
- Using `__main__.py` makes `python -m rescale` work for free

---

## 🧑‍💻 Hands on: metadata and a CLI

- Add the full metadata (adjust the author!)
- Add the `rescale` script entry point
- Inspect: `uv run --with pip pip show -v rescale`
- Run it: `uv run rescale 1 2 3`

Stale metadata? uv cached the old build:
`uv sync --reinstall-package rescale`

---

## Versioning schemes

**SemVer** (`major.minor.patch`) — read it as author intent:

- patch: "nothing to see" · minor: "new stuff" · major: "look first"
- _Not_ a promise nothing breaks — with enough users, every change breaks someone
- So don't preemptively pin `package<2`

**CalVer** — date-based (pip's `25.1`); communicates age and deprecation windows

---

## Single-sourcing the version

Two copies (`pyproject.toml` + `__version__`) drift. Let the backend compute it:

```toml
[build-system]
requires = ["hatchling", "hatch-vcs"]
build-backend = "hatchling.build"

[project]
name = "rescale"
dynamic = ["version"]

[tool.hatch]
version.source = "vcs"
build.hooks.vcs.version-file = "src/rescale/_version.py"
```

---

## Versions from git tags

```bash
git tag v0.2.0
```

- Tagging **is** the release process
- Commits after a tag get dev versions: `0.2.1.dev3+g1a2b3c4`
- `_version.py` is a build artifact — add it to `.gitignore`, re-export it:

```python
from rescale._version import __version__
```

_Alternative: `version.path` reads `__version__` out of your source file._

---

## 🧑‍💻 Hands on: version from git

- Switch to hatch-vcs versioning
- Commit, then `git tag v0.2.0`
- Check it:

```bash
uv sync --reinstall-package rescale
uv run python -c "import rescale; print(rescale.__version__)"
```

- Make one more commit — check again!

---

## The supporting files

| File           | What it's for                                            |
| -------------- | -------------------------------------------------------- |
| `README.md`    | Description, install, usage — rendered on GitHub & PyPI  |
| `LICENSE`      | Exact license text — without it, nobody may use the code |
| `.gitignore`   | GitHub's Python template; hatchling uses it too!         |
| `CHANGELOG.md` | Human-readable changes per version                       |

---

## 🧑‍💻 Hands on: round out the repo

- `README.md` — description, install instructions, usage example
- `LICENSE` — BSD-3-Clause text from choosealicense.com
- `.gitignore` — GitHub's Python template (+ `_version.py`)
- Make sure `readme` and `license-files` point at the right names
- Commit!

---

## Distributions

<div class="columns">
<div>

### SDist

- **Source** distribution: `.tar.gz` of source + metadata
- Installing runs the build backend on the user's machine

</div>
<div>

### Wheel

- **Built** distribution: `.whl` zip, just unpacked into `site-packages`
- No code runs at install — fast and safe
- Pure Python: one wheel works everywhere (`py3-none-any`)

</div>
</div>

Compiled packages need a wheel _per platform_ — most of the rest of this workshop!

---

## Building and inspecting

```bash
uv build
```

```bash
tar -tf dist/*.tar.gz    # SDist contents
unzip -l dist/*.whl      # wheel contents
```

- Wheel metadata lives in `rescale-0.2.0.dist-info/METADATA` — your `[project]` table, rendered
- `RECORD` lists every file with a hash — exact uninstalls/upgrades

---

## 🧑‍💻 Hands on: inspect your distributions

Build, then find:

1. Where the `[project]` table ended up in the wheel
2. Whether `tests/` made it into each artifact — and whether that's a problem
3. What `RECORD` is

```bash
unzip -p dist/*.whl '*/METADATA' | head -20
```

---

## Summary

- **src layout** — nothing works without a real install
- **`[project]` table** — standard metadata, identical across backends
- **Entry points** — installers generate your executables
- **Dynamic versioning** — tag once, version everywhere
- **SDist + wheel** — ship both

You did it by hand once — next chapter: the template that does it for you.
