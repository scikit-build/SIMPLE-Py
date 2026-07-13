---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## uv vs. Pixi: A showdown

---

## Two tools, one question

Setup asked you to install **two** environment managers:

- `uv` — fast Python package manager (Rust)
- `pixi` — conda package manager (Rust)

Which should you use, and when?

It comes down to your **goals** and your **constraints** — and the two share a strikingly similar high-level design.

---

## Feature set: installing

| Capability          | uv                    | Pixi                     |
| ------------------- | --------------------- | ------------------------ |
| Python packages     | ✅                    | ✅ (via uv's Rust libs)  |
| conda packages      | ❌                    | ✅                       |
| Python itself       | ✅                    | ✅                       |
| Environment format  | `.venv` (virtualenv)  | `.pixi/envs` (conda)     |
| Activate in a shell | `. .venv/bin/activate` | `pixi shell`            |

---

## Feature set: locking & dependencies

| Capability            | uv                  | Pixi                    |
| --------------------- | ------------------- | ----------------------- |
| Lock file             | `uv.lock` (TOML)    | `pixi.lock` (YAML)      |
| Auto-produces locks   | ✅ (`run` / `sync`) | ✅ (any workspace op)   |
| Declare a dependency  | `uv add`            | `pixi add`              |
| Low-level escape hatch | `uv pip install`   | ❌                      |

---

## Feature set: tools & shipping

| Capability        | uv                       | Pixi                   |
| ----------------- | ------------------------ | ---------------------- |
| Global tools      | `uv tool install`        | `pixi global`          |
| Throwaway env     | `uvx`                    | `pixi exec`            |
| Task runner       | ❌                       | ✅                     |
| Multi-environment | ❌ (delegates to nox/tox) | ✅ (arbitrary envs)   |
| Build             | `uv build` → sdist/wheel | Pixi Build → conda pkg |
| Publish           | `uv publish` → PyPI      | `pixi upload` → conda  |
| Speed             | Fastest!                 | Fast!                  |

---

## "Lock file" means two different things

Both lock — but not the same way:

- **uv** records one **universal** resolution with environment markers, valid on _every_ platform
- **Pixi** records a **separate solve for each platform** the workspace declares

Neither is the standard [PEP 751](https://peps.python.org/pep-0751/) `pylock.toml` — though `uv export --format pylock.toml` can emit one.

---

## First: what these tools are _not_

`uv` and Pixi resolve, install, and manage **environments**.

They do **not** replace your **build backend** — `hatchling`, `scikit-build-core`, `setuptools` all still do the building.

So the real choice is about your **project** and its dependencies for building, running, and testing.

---

## Reach for `uv` when…

**Everything you need — including at build time — is a Python package.**

- Broader than pure-Python: CMake and Ninja ship as wheels, so a `scikit-build-core` compiled extension works with `uv` given a system compiler
- Traditional virtual environments + [PEP 735](https://peps.python.org/pep-0735/) dependency groups
- Python-only focus → Python-specific optimizations
- As of 2026, still the **fastest** Python dependency manager

---

## Reach for Pixi when…

**You depend on things that aren't Python packages** — a compiler stack, or large non-Python deps (e.g. Python bindings to a C++ framework like Geant4).

- Pixi Build pins **consistent compiler stacks** across time
- **Multiple named environments** in one workspace, composed from "features" — a Python-version test matrix or an optional CUDA env, no `nox`/`tox` needed

**Costs:** per-platform solves make lock files large and slower to grow; a PyPI **source** dep with dynamic metadata forces installing the env just to lock.

---

## Developing a package for distribution

<div class="columns">
<div>

### `uv`

- `uv add` writes `[project.dependencies]` — the **same** metadata your sdist/wheel ship, so dev env and package can't drift
- `uv sync` installs your project **editable by default**
- `uv build` → sdist + wheel; `uv publish` → PyPI

</div>
<div>

### Pixi

- conda deps live in `[tool.pixi.dependencies]` — **not** package metadata, so PyPI runtime deps get declared **twice**
- editable install must be **explicit**
- Pixi Build → **conda packages** for conda channels

</div>
</div>

```toml
[tool.pixi.pypi-dependencies]
your-package = { path = ".", editable = true }
```

---

## You can use both

The choice isn't exclusive:

- Pixi already uses **uv's resolver** internally for its PyPI dependencies
- Some projects ship **both manifests** so contributors pick their tool
- Both have first-party CI actions with caching keyed on the lock file:
  - [`astral-sh/setup-uv`](https://github.com/astral-sh/setup-uv)
  - [`prefix-dev/setup-pixi`](https://github.com/prefix-dev/setup-pixi)

---

## Pixi CLI, if you know `uv`

| Task            | uv                     | Pixi                  |
| --------------- | ---------------------- | --------------------- |
| Init a project  | `uv init`              | `pixi init`           |
| Add a dep       | `uv add numpy`         | `pixi add numpy`      |
| Add a PyPI dep  | `uv add cowsay`        | `pixi add --pypi cowsay` |
| Run a command   | `uv run pytest`        | `pixi run pytest`     |
| Activate shell  | `. .venv/bin/activate` | `pixi shell`          |
| Global tool     | `uv tool install ruff` | `pixi global install ruff` |
| Throwaway env   | `uvx cowsay`           | `pixi exec cowsay`    |

---

## Summary

- Same job — **resolve, install, manage environments** — neither replaces your build backend
- **`uv`**: everything (incl. build-time) is a Python package; venvs + PEP 735 groups; fastest
- **Pixi**: non-Python deps, compiler stacks, multiple named environments from conda
- Lock files differ: **universal** (uv) vs. **per-platform** (Pixi)
- Distribution favors `uv` (deps + editable by default); Pixi ships **conda packages**
- Not exclusive — Pixi runs uv's resolver, and projects can ship both
