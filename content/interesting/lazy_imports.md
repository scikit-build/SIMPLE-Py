# Modern Python: lazy imports

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/6_02_lazy_imports>`

Importing a module runs it. Every `import numpy` at the top of a file pays the
full cost of loading that module --- even if the code path that needs it never
runs. For a library that's a one-time cost; for a CLI tool it's paid on _every_
invocation, including `--help`.

[PEP 810](https://peps.python.org/pep-0810/) adds **lazy imports** to Python
3.15. A lazy import does nothing when the `import` statement runs; the module is
loaded the first time you actually touch it. Unlike the earlier, rejected
attempt at implicit laziness, this one is explicit and opt-in --- libraries mark
which imports are safe to defer.

:::{note}
Python 3.15 is still in alpha, but you can try lazy imports today:
`uv python install 3.15` fetches it on every major platform, and
`uv run --python 3.15 ...` runs against it.
:::

## The problem

Here is a standard argparse CLI:

```python
import argparse
import numpy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--foo", action="store_true")
    args = parser.parse_args()
    if args.foo:
        print(numpy.array([1, 2, 3]))
```

Run this with `--help` and `numpy` is imported anyway, even though it is never
used. With modern tooling this hurts more than it used to: `uv` doesn't
pre-compile bytecode unless you ask it to, so the first import is slower.

The same waste shows up in the classic re-export `__init__.py`:

```python
# __init__.py
from . import a
from . import b

__all__ = ["a", "b"]
```

This lets a user write `lib.a.stuff` after just `import lib`, but they pay to
import `b` even if they only ever touch `a`. Careful libraries like `rich` avoid
this and ask for explicit imports; many older ones don't. Subcommand CLIs have
the same shape --- each subcommand needs different dependencies, but importing
the top-level package drags in all of them.

## Using lazy imports

In Python 3.15 you add the `lazy` keyword:

```python
lazy import argparse
lazy import numpy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--foo", action="store_true")
    args = parser.parse_args()
    if args.foo:
        print(numpy.array([1, 2, 3]))
```

Now nothing happens when these imports run --- the modules might not even be
installed. The first time you access an attribute, the module becomes a real,
imported object. Run `--help` and `numpy` is never touched, so it's never
imported.

There is also a backward-compatible spelling that works on _any_ Python version
(it's just not lazy before 3.15):

```python
__lazy_modules__ = ["argparse", "numpy"]

import argparse
import numpy
```

`__lazy_modules__` is a plain list of absolute module names, so you can generate
or manipulate it dynamically. Linters like Ruff already know to allow it above
your imports without complaining about import order.

:::{note}
Two escape hatches force the mode globally, mostly for testing: the `-X
lazy_imports=all` flag and the `PYTHON_LAZY_IMPORTS=all` environment variable
(both also accept `normal` and `none`). Turning laziness `all` on before you
start is a quick way to estimate how much time you might save.
:::

## When _not_ to be lazy

You don't have to mark everything lazy, and sometimes you shouldn't.

**Import side effects.** If a module needs to run at the import site, it can't
be lazy. The most common case is a guarded optional dependency:

```python
try:
    import numpy
except ModuleNotFoundError:
    ...
```

Make this lazy and the `ModuleNotFoundError` moves to the first _use_ of numpy,
which is not what you want. The semi-lazy alternative checks for the module
without importing it:

```python
import importlib.util

if importlib.util.find_spec("numpy") is None:
    ...  # whatever you wanted to do if numpy is missing

lazy import numpy
```

This costs a little more than doing nothing (which is why laziness doesn't do it
for you), and it imports parent packages to reach subpackages (`a.b` imports
`a`), but it's a good way to detect an installed package.

**Top-level use.** If you use the module while the file is being imported,
laziness buys nothing:

```python
lazy import re

REGEX = re.compile(...)  # re is loaded right here anyway
```

You can defer this by moving the work behind a cache:

::::{tab-set}

:::{tab-item} Python 3.15+

```python
import functools

lazy import re


@functools.cache
def regex() -> re.Pattern:
    return re.compile(...)
```

:::

:::{tab-item} Older

```python
__lazy_modules__ = ["re"]

import functools
import re


@functools.cache
def regex() -> re.Pattern:
    return re.compile(...)
```

:::

::::

Note there's no `from __future__ import annotations` needed for the `re.Pattern`
return annotation --- annotations became lazy by default in Python 3.14, so they
don't trigger the import.

You _can_ make top-level imports lazy, but you're just relocating the eventual
import for no benefit, so it's cleaner to leave them eager.

## flake8-lazy

Deciding what belongs in `__lazy_modules__` by hand is fiddly --- you have to
find every module that isn't used at top level, keep the list sorted, and avoid
listing things twice. [flake8-lazy](https://github.com/henryiii/flake8-lazy)
does that for you. It's a flake8 plugin with a standalone runner, which is
likely how you'll use it early in the 3.15 lifecycle:

::::{tab-set}

:::{tab-item} uv

```bash
# Show flake8-style errors
uvx flake8-lazy <filenames>
# Show the lines you need to add
uvx flake8-lazy --format=lazy-modules
# Just add them!
uvx flake8-lazy --apply=list <filenames>
```

:::

:::{tab-item} pipx

```bash
# Show flake8-style errors
pipx run flake8-lazy <filenames>
# Show the lines you need to add
pipx run flake8-lazy --format=lazy-modules
# Just add them!
pipx run flake8-lazy --apply=list <filenames>
```

:::

::::

`--apply` rewrites files in place; it supports `list`, `set`, `native` (the
`lazy` keyword), and `dynamic` output formats.

The checks fall into four groups. The 1xx checks find imports that _should_ be
lazy:

| Code     | Missing lazy declarations                                          |
| -------- | ------------------------------------------------------------------ |
| `LZY101` | stdlib module should be listed in `__lazy_modules__`               |
| `LZY102` | third-party or local module should be listed in `__lazy_modules__` |

The 2xx checks keep an existing `__lazy_modules__` tidy and correct:

| Code     | `__lazy_modules__` validation                                   |
| -------- | --------------------------------------------------------------- |
| `LZY201` | `__lazy_modules__` is not sorted                                |
| `LZY202` | module listed in `__lazy_modules__` is never imported           |
| `LZY203` | module listed in `__lazy_modules__` is duplicated               |
| `LZY204` | `__lazy_modules__` is assigned after importing modules it names |
| `LZY205` | module listed in `__lazy_modules__` must be an absolute name    |

The 3xx checks cover the native `lazy` keyword and only run on a 3.15+ host:

| Code     | Native `lazy` keyword (Python 3.15+)                               |
| -------- | ------------------------------------------------------------------ |
| `LZY301` | lazy import inside `suppress(ImportError)` is misleading            |
| `LZY302` | module declared lazy by both `lazy` keyword and `__lazy_modules__` |
| `LZY303` | module imported both eagerly and lazily                            |

The 4xx checks are the inverse of 1xx --- laziness that isn't buying anything:

| Code     | Lazy import safety and semantics                                    |
| -------- | ------------------------------------------------------------------- |
| `LZY401` | module is declared lazy but accessed at the top level               |
| `LZY402` | module is an enclosing package for this file and should not be lazy |

:::{tip}
Don't run this on your test suite --- tests generally use everything they
import, so there's nothing to defer.
:::

## Tips

**Look beyond what the tool finds.** The `re` cache above is a case the checker
can't spot on its own. Watch the _actual_ imports too, since one library often
pulls in another --- lots of things import `re` (including `typing`), and `re`
isn't cheap. Profile with `python -X importtime`; anything successfully made
lazy drops off the list.

**Skip `typing` at runtime.** Type checkers always treat `TYPE_CHECKING` as
`True`, so you can gate type-only imports behind a local that's `False` at
runtime:

```python
TYPE_CHECKING = False
if TYPE_CHECKING:
    import numpy
```

Ruff can even enforce this with `flake8-tidy-imports`:

```toml
[tool.ruff.lint.flake8-tidy-imports.banned-api]
"typing.TYPE_CHECKING".msg = "Use TYPE_CHECKING=False instead"
```

**Relative imports stay static.** `__lazy_modules__` needs absolute names, so
build them from the package name:

```python
__lazy_modules__ = [f"{__spec__.parent}.thing"]
from . import thing
```

(`__package__` is the older spelling of `__spec__.parent`.) Don't do this in
`__main__.py` --- use absolute imports there, since `__spec__` can be `None`.

## Results

Making imports lazy pays off most for CLI startup. Running `flake8-lazy` on real
projects and timing `--help` on Python 3.15:

| Package      | Before  | After   | Speedup |
| ------------ | ------- | ------- | ------- |
| flake8-lazy  | 100+ ms | 50 ms   | 2x      |
| repo-review  | 113 ms  | 35 ms   | 3x      |
| cibuildwheel | 179 ms  | 61 ms   | 3x      |
| check-sdist  | 45 ms   | 34 ms   | 30%     |

Python itself takes around 15 ms to start (on an M1), so that's the floor --- you
still need a few things like argparse. The `uv`-doesn't-precompile effect means
the first-run savings can be larger than these warm numbers show. To measure a
change yourself, [hyperfine](https://github.com/sharkdp/hyperfine) makes an
easy before/after:

```bash
hyperfine --warmup 10 \
    -n main --prepare "git checkout main"      "python3.15 -m <pkg> --help" \
    -n PR   --prepare "git checkout some-branch" "python3.15 -m <pkg> --help"
```

Add `-X lazy_imports=none` or `all` to bound the best and worst case without
touching the code.

:::{note}
There are edge cases where laziness silently breaks: dataclasses resolve their
annotations to look for `typing.ClassVar`, which forces those imports. And the
`from a import b` form is ambiguous --- only a type checker knows whether `b` is
a module or an attribute --- so flake8-lazy assumes it isn't and may miss it. Use
`import a.b as b` to be unambiguous. When in doubt, err toward marking too much
lazy rather than too little.
:::
