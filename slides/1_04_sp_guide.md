---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Best practices & the SP guide

---

## The Scientific Python guide

Modern best practices for package authors, all in one place.

<div class="columns">
<div>

### Three tools

- **Guide** — the recommendations
- **cookie** — a template that _starts_ a project following them
- **sp-repo-review** — _checks_ an existing project against them

</div>
<div>

### This chapter

A whirlwind tour of what a _healthy_ package looks like around your code:

- Tests
- Static checks
- Types

</div>
</div>

---

## Testing

Tests let you refactor with confidence and know instantly when a new Python or
platform breaks.

The ecosystem standardized on **pytest** — plain `assert` gives rich failure
output, no `self.assertEqual` zoo:

```python
# tests/test_basic.py
def test_funct():
    assert 4 == 2**2
```

Tests go in `tests/` (no `__init__.py`), named `test_*.py`.

---

## Configure pytest strictly

```toml
# pyproject.toml
[tool.pytest]
minversion = "9.0"
addopts = ["-ra", "--showlocals"]
strict = true
filterwarnings = ["error"]
testpaths = ["tests"]
log_level = "INFO"
```

The key line is `filterwarnings = ["error"]` — turns warnings into failures, so
you fix deprecations _before_ your users hit them.

---

## Patterns you'll reach for

<div class="columns">
<div>

**Approx** — fuzzy compares (works on NumPy arrays):

```python
assert 1 / 3 == approx(0.3333333333333)
```

**Raises** — test that errors _do_ fire:

```python
with pytest.raises(ZeroDivisionError):
    1 / 0
```

</div>
<div>

**Parametrize** — one test, many cases:

```python
@pytest.mark.parametrize("n", [1, 2, 3])
def test_square(n):
    assert n**2 == n * n
```

**Fixtures** — just named arguments:

```python
def test_printout(capsys):
    print("hello")
    assert "hello" in capsys.readouterr().out
```

</div>
</div>

---

## Testing tips

> Tests should run against the **installed** version of your code, not the local
> folder. A `src/` layout stops `pytest` from importing your source tree by
> accident.

Once you have tests, measure them with **coverage**:

```bash
pytest --cov=<pkg>          # then upload to Codecov in CI
```

Chase _meaningful_ coverage — weak tests added just to bump the number aren't
worth it.

---

## Static checks

Run without executing your code. The guide drives them all through
**pre-commit** (or the faster Rust rewrite **prek**):

```bash
prek run -a      # run every hook on every file
prek install     # optional: run automatically on git commit
```

One config file, checks in isolated environments, one command.

---

## A minimal `.pre-commit-config.yaml`

```yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: "v6.0.0"
    hooks:
      - id: check-added-large-files
      - id: check-merge-conflict
      - id: end-of-file-fixer
      - id: trailing-whitespace
  - repo: https://github.com/astral-sh/ruff-pre-commit
    rev: "v0.15.18"
    hooks:
      - id: ruff-check
        args: ["--fix"]
      - id: ruff-format
```

---

## Ruff

A single ultra-fast Rust binary that replaces Flake8, isort, pyupgrade, and
dozens of others. Both a **linter** (`ruff-check`) and a **formatter**
(`ruff-format`, a near-perfect Black clone).

```toml
# pyproject.toml
[tool.ruff]
show-fixes = true
lint.extend-select = [
  "B",    # flake8-bugbear: likely bugs
  "I",    # isort: sorted imports
  "UP",   # pyupgrade: modernize syntax
  "RUF",  # Ruff-specific rules
]
```

> Put content-modifying hooks (fixers, formatters) _above_ report-only hooks.
> `prek auto-update` bumps every `rev:`.

---

## Typing

Type hints document intent and — via a checker — catch whole classes of bugs
_before_ you run anything:

```python
def f(x: int) -> int:
    return x * 5
```

Types do nothing at runtime, but **MyPy** verifies you're not lying about them
across every branch — including code your tests rarely hit.

```toml
# pyproject.toml
[tool.mypy]
files = "src"
python_version = "3.10"
strict = true
```

Typing is **gradual** — adopt it one module at a time.

---

## Two features worth knowing

<div class="columns">
<div>

### Narrowing

```python
def g(x: str | int) -> None:
    if isinstance(x, str):
        print(x.lower())  # str here
    else:
        print(x + 1)  # int here
```

The checker narrows the type inside each branch.

</div>
<div>

### Protocols

```python
class Duck(Protocol):
    def quack(self) -> str: ...


def pester(d: Duck) -> None:
    print(d.quack())
```

Anything with a matching `quack` _is_ a `Duck` — structural typing, no
inheritance.

</div>
</div>

**Rule of thumb:** accept the most general type, return the most specific.

> Ship an empty `py.typed` file so your types are visible to users!

---

## And there's more

The guide keeps going well past this tour:

| Page             | What it covers                                    |
| ---------------- | ------------------------------------------------- |
| **CI setup**     | GitHub Actions, then wheels for pure and compiled |
| **Docs**         | Setting up and hosting documentation              |
| **Task runners** | Automating jobs with nox                          |
| **Security**     | Hardening your project and its supply chain       |
| **AI**           | Using agentic AI responsibly on a package (WIP)   |

---

## Using cookie

Rather than assembling all this by hand, **cookie** generates a new package that
already follows the guide — **eleven** build backends, including the compiled
ones (`pybind11`, `scikit-build-core`, `maturin`, …).

<div class="columns">
<div>

**copier** (modern — pull updates later):

```bash
uvx copier copy \
  gh:scientific-python/cookie \
  my-package
```

</div>
<div>

**cookiecutter** (classic — no ongoing link):

```bash
uvx cookiecutter \
  gh:scientific-python/cookie
```

</div>
</div>

Asks a handful of questions (name, license, backend, …) and writes a
ready-to-go repo.

---

## Using sp-repo-review

Already have a project? **sp-repo-review** checks it against the guide and gives
you a checklist of what's followed and what's missing.

```bash
uvx 'sp-repo-review[cli]' <path to repo>
```

- Green means good, red is a suggestion — some failures are fine
- Every check has a code (`PY006`, `PP302`, …) linking to the exact spot in the
  guide

> Run it in your browser — the guide hosts a WebAssembly version that grades any
> public GitHub repo with no install.

---

## Summary

- **pytest** for tests, configured strictly; measure with **coverage**
- **pre-commit / prek** drives static checks — **Ruff** lints and formats
- **MyPy** type-checks; adopt gradually, ship `py.typed`
- **cookie** starts a new project from the guide
- **sp-repo-review** grades an existing one

The [Scientific Python Development Guide](https://learn.scientific-python.org/development/)
ties it all together.
