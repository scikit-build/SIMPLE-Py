---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Setting up for development

---

## Before you begin

<div class="columns">
<div>

### Required

- `uv` — fast package manager (Rust)
- `pixi` — conda package manager (Rust)
- A system compiler

</div>
<div>

### Recommended

- `prek` — linter/formatter runner
- `nox` — task runner

_(install via `uv tool install`)_

</div>
</div>

---

## Low level concepts

### How not to break your computer

These topics give you an understanding of what's going on.
We'll introduce high-level abstractions next!

---

## Virtual environments

How to install stuff — three options:

|                          | Location          | Pros                | Cons                    |
| ------------------------ | ----------------- | ------------------- | ----------------------- |
| **System site-packages** | `/...`            | Works from anywhere | Can break your machine  |
| **User site-packages**   | `~/.local/...`    | User permission     | Can break other Pythons |
| **Virtual environment**  | `.venv/` (common) | Many!               | More effort             |

---

## Why virtual environments?

System or user installs sound nice, but:

- Can't install incompatible packages
- Can break your system
- Can't control per-project needs
- Hard to know your real requirements
- Difficult to update

**Solution:** a virtual environment — a folder (`.venv`) that isolates dependencies.

---

## Virtual environment structure

```text
.venv
├── .gitignore
├── CACHEDIR.TAG
├── bin
│   ├── activate      # shell activation script
│   ├── python        # symlinks to your Python
│   ├── python3
│   └── python3.14
├── lib
│   └── python3.14
│       └── site-packages  # installed libraries
└── pyvenv.cfg        # marks this as a venv
```

---

## Using a virtual environment

**Direct usage:**

```bash
.venv/bin/python ...
```

**Activation:**

```bash
. .venv/bin/activate
python ...
deactivate
```

> Activation sets `PATH` so the venv's `bin/` comes first.

---

## Creating a virtual environment

| Command                 | Speed  | Notes                                |
| ----------------------- | ------ | ------------------------------------ |
| `python3 -m venv .venv` | Slow   | Built in (may need separate install) |
| `virtualenv .venv`      | Medium | Better defaults, requires install    |
| `uv venv`               | Fast   | Empty (no pip), defaults to `.venv`  |

**We'll use `uv`.**

---

## Requirements

A venv is expendable — you should be able to delete and recreate it.

Instead of manually installing, list packages in a file:

<div class="columns">
<div>

### Project (app)

- **requirements.txt** — classic list
- **requirements.in** — manual locking
- **Lock file** — pinned versions
- **dependency-groups** — named collections

</div>
<div>

### Package (library)

- **dependencies** — minimum to install
- **optional-dependencies** — extra requestable sets

Libraries also use "Project" patterns for dev needs (tests, docs).

</div>
</div>

---

## Lock files

- Every dependency **fully specified**, ideally with a SHA
- Great way to reproduce an environment exactly
- **Cannot** be used for libraries — two libraries with conflicting pins couldn't coexist

---

## Low-level summary

| Concept          | Description                        |
| ---------------- | ---------------------------------- |
| **venv**         | Virtual environment isolating deps |
| **requirements** | List of packages to install        |
| **lock files**   | Fully pinned set of packages       |

Creating, activating, installing, and locking are all **separate steps** with different tools.

---

## High level packaging

---

## Persistent apps

Apps you install and run — never need to import alongside unrelated packages.

1. Make a venv somewhere
2. Install the app in it
3. Put just that app on your PATH

This is what `pipx` and `uv tool` do!

```bash
uv tool install cowsay
uv tool list
uv tool upgrade cowsay
uv tool uninstall cowsay
```

---

## Single-use apps

Combine install + run — `uvx`:

```bash
uvx cowsay
```

Makes a venv, downloads the app, runs it. Recreates if over a week old.

**All of PyPI at your fingertips** — no need to remember to update!

---

## Single-file scripts

```python
# /// script
# dependencies = ["numpy"]
# ///

import numpy as np

if __name__ == "__main__":
    print(np.array([1, 2, 3]))
```

```bash
uv run simple.py    # dependencies downloaded into a temp venv
```

---

## Projects (websites, etc.)

Very similar to libraries — key difference: **always commit your lockfile**.

You can make an unpublishable library:

```toml
[project]
classifiers = [
  "Private :: Do Not Upload",
]
```

---

## Libraries

Shared with others — must coexist in an environment with whatever else the user needs.

Basic `pyproject.toml`:

```toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "example"
version = "0.1.0"
```

The `[build-system]` tells tools how to build the package.

---

## Dependency locations

```toml
[build-system]
requires = ["hatchling"]           # 1: build-time only

[project]
dependencies = []                  # 2: always installed

[project.optional-dependencies]
extra = []                         # 3: user-requested extras

[dependency-groups]
dev = []                           # 4: dev-only, not in metadata
```

---

## Dependency comparison

|                           | `b-s.r` | `p.d` | `p.o-d` | `d-g` |
| ------------------------- | ------- | ----- | ------- | ----- |
| Public metadata           | ✅      | ✅    | ✅      | ❌    |
| Always installed          | ❌      | ✅    | ❌      | ❌    |
| Named groups              | ❌      | ❌    | ✅      | ✅    |
| Independently installable | ❌      | ❌    | ✅      | ✅    |

---

## High-level project management with `uv`

If `dependency-groups` has a `dev` group, `uv run` installs it by default:

```toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "example"
version = "0.1.0"
requires-python = ">=3.12"
dependencies = ["numpy"]

[dependency-groups]
dev = ["pytest"]
```

---

## `uv run pytest` does all of this:

- Downloads Python if needed
- Creates `.venv` if it doesn't exist
- Installs from `uv.lock` if it exists
- Creates `uv.lock` from dependencies if it doesn't exist
- Makes an editable install of your project
- Runs the command in that venv

Edit dependencies? Just `uv run` again — lockfile and venv update automatically.

---

## Summary

- **Virtual environments** isolate dependencies per project
- **`uv`** handles venvs, installs, locking, and running
- **`uv tool` / `uvx`** for apps
- **`uv run`** for project development
- **`pyproject.toml`** declares dependencies at four levels
- **Lock files** ensure reproducibility
