---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Lazy imports

---

## Importing costs you

Every `import` at the top of a file **runs that module** — right then.

```python
import argparse
import numpy  # loaded even for `--help`


def main():
    args = argparse.ArgumentParser().parse_args()
    if args.foo:
        print(numpy.array([1, 2, 3]))
```

- A library pays this **once**
- A CLI pays it on **every** invocation, including `--help`
- `uv` doesn't pre-compile bytecode → the first import is slower still

---

## Where it hurts

<div class="columns">
<div>

The re-export `__init__.py`:

```python
from . import a
from . import b

__all__ = ["a", "b"]
```

`import lib` drags in **`b`** even if
you only touch `lib.a`.

</div>
<div>

Subcommand CLIs:

- Each subcommand needs
  **different** dependencies
- Importing the package pulls in
  **all** of them

</div>
</div>

Careful libraries (like `rich`) ask for explicit imports — many older ones don't.

---

## Python 3.15: `lazy import`

[PEP 810](https://peps.python.org/pep-0810/) makes imports **opt-in lazy**.

```python
lazy import argparse
lazy import numpy
```

- The `import` statement does **nothing** — the module might not even be installed
- First **attribute access** turns it into a real, imported object
- Run `--help`, never touch `numpy` → it's **never imported**

```bash
uv python install 3.15   # beta, but one command away
```

---

## Works on older Pythons too

A back-compat spelling — just not lazy before 3.15:

```python
__lazy_modules__ = ["argparse", "numpy"]

import argparse
import numpy
```

- A plain list of **absolute** module names → generate it dynamically if you like
- Ruff already allows it above your imports without an import-order complaint

> Testing knobs: `-X lazy_imports=all` / `PYTHON_LAZY_IMPORTS=all`
> (also `normal`, `none`) — flip `all` on to estimate the payoff.

---

## When *not* to be lazy

<div class="columns">
<div>

**Import side effects** — a guarded optional dep can't be lazy:

```python
try:
    import numpy
except ModuleNotFoundError:
    ...
```

Semi-lazy alternative:

```python
if find_spec("numpy") is None:
    ...
lazy import numpy
```

</div>
<div>

**Top-level use** buys nothing:

```python
lazy import re

REGEX = re.compile(...)  # loaded now
```

Defer it behind a cache:

```python
@functools.cache
def regex() -> re.Pattern:
    return re.compile(...)
```

</div>
</div>

Making these lazy just **relocates** the import — leave them eager.

---

## A tool: flake8-lazy

Deciding what to defer by hand is fiddly. `flake8-lazy` finds it for you.

```bash
uvx flake8-lazy <files>                  # flake8-style errors
uvx flake8-lazy --format=lazy-modules    # the lines to add
uvx flake8-lazy --apply=list <files>     # just add them
```

| Group | Checks                                                      |
| ----- | ---------------------------------------------------------- |
| `1xx` | module **should** be lazy (`LZY101` stdlib, `LZY102` other) |
| `2xx` | `__lazy_modules__` sorted / unique / absolute / positioned  |
| `3xx` | native `lazy` keyword issues (3.15+ host)                   |
| `4xx` | declared lazy but used at top level — **not** worth it      |

`--apply` writes `list`, `set`, `native`, or `dynamic` — in place.

---

## Tips

- **Don't** run it on your test suite — tests use what they import
- Profile with **`-X importtime`**; what went lazy drops off the list
- Skip `typing` at runtime:

  ```python
  TYPE_CHECKING = False
  if TYPE_CHECKING:
      import numpy
  ```

- Relative imports stay static — build absolute names:

  ```python
  __lazy_modules__ = [f"{__spec__.parent}.thing"]
  from . import thing
  ```

---

## Results

Timing `--help` on Python 3.15 after running the tool:

| Package      | Before  | After | Speedup |
| ------------ | ------- | ----- | ------- |
| flake8-lazy  | 100+ ms | 50 ms | 2x      |
| repo-review  | 113 ms  | 35 ms | 3x      |
| cibuildwheel | 179 ms  | 61 ms | 3x      |

Floor is ~**15 ms** (Python startup) — and `uv`'s no-precompile means the
**first-run** savings are bigger than these warm numbers show.

---

## Summary

- Imports **run on import** — wasted for code paths you don't hit
- **PEP 810 / 3.15**: `lazy import` defers until first attribute access
- **`__lazy_modules__`** is the back-compat spelling (works everywhere)
- Skip laziness for **side effects** and **top-level use**
- **`flake8-lazy`** finds and applies it — `uvx flake8-lazy --apply=list`
- CLI `--help` gets **2–3x** faster; floor is Python's own startup
