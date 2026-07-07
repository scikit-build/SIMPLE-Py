# Best development practices and the SP guide

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/1_04_sp_guide>`

## Intro to the Scientific Python guide

The [Scientific Python Development Guide][] is packed with great guides for
package authors! It collects modern best practices into a single place, keeps
them in sync with a project template ([cookie][]), and provides a tool
([sp-repo-review][]) that grades your repo against them.

You've already built a package. This chapter is a whirlwind tour of what a
_healthy_ package looks like around that code: tests, static checks, and types.
Each section links to the full guide page if you want to go deeper.

:::{tip} Three tools, one guide
The guide is the recommendations. [cookie][] is a template that _starts_ a new
project already following them. [sp-repo-review][] _checks_ an existing project
against them. We'll meet all three.
:::

### Testing

Tests let you refactor with confidence and immediately know when a new Python or
platform breaks. The whole ecosystem has standardized on [pytest][]; since tests
are a developer-only requirement, there's no reason not to use it.

The killer feature is that plain `assert` gives rich failure output, no
`self.assertEqual` zoo:

```{code} python
:filename: tests/test_basic.py
def test_funct():
    assert 4 == 2**2
```

Put tests in a `tests/` folder (no `__init__.py`), name files `test_*.py`, and
configure pytest strictly in `pyproject.toml`:

```{code} toml
:filename: pyproject.toml
[tool.pytest]
minversion = "9.0"
addopts = ["-ra", "--showlocals"]
strict = true
filterwarnings = ["error"]
testpaths = ["tests"]
log_level = "INFO"
```

The most important line is `filterwarnings = ["error"]`: it turns warnings into
failures, so you fix deprecations _before_ your users hit them.

A few patterns you'll reach for constantly:

::::{tab-set}

:::{tab-item} Approx

```python
from pytest import approx


def test_approx():
    assert 1 / 3 == approx(0.3333333333333)
```

Works on NumPy arrays too: prefer `array1 == approx(array2)`.

:::

:::{tab-item} Raises

```python
import pytest


def test_raises():
    with pytest.raises(ZeroDivisionError):
        1 / 0
```

Test that errors _do_ fire. See also `pytest.warns`.

:::

:::{tab-item} Parametrize

```python
import pytest


@pytest.mark.parametrize("n", [1, 2, 3])
def test_square(n):
    assert n**2 == n * n
```

One test, three cases, three names in the report.

:::

:::{tab-item} Fixtures

```python
def test_printout(capsys):
    print("hello")
    assert "hello" in capsys.readouterr().out
```

Fixtures are just named arguments; `capsys`, `tmp_path`, and `monkeypatch` are
built in.

:::

::::

> [!NOTE]
> Tests should run against the **installed** version of your code, not the local
> folder. This is a big reason to use a `src/` layout: `pytest` won't
> accidentally import your source tree instead of the installed package.

:::{exercise} Run a real test suite
:label: sp-pytest

Clone `https://github.com/pypa/packaging` and run its test suite with `uv`,
without manually creating a venv or installing anything.

:::

:::{solution} sp-pytest
:class: dropdown

```bash
git clone https://github.com/pypa/packaging
cd packaging
uv run pytest
```

`uv run` builds the environment, installs the package editable, and runs pytest.

:::

Once you have tests, measure how much of your code they exercise with
[coverage][]. Run it via `coverage run -m pytest` (or `pytest --cov=<pkg>`) and
upload to [Codecov][] in CI. Chase _meaningful_ coverage; weak tests added just
to bump the number aren't worth it.

### Static checks

Static checks run without executing your code. The guide drives them all through
[pre-commit][] (or the faster Rust rewrite [prek][], which you already
installed). One config file, checks in isolated environments, one command:

```bash
prek run -a      # run every hook on every file
prek install     # optional: run automatically on git commit
```

A minimal `.pre-commit-config.yaml` with the essentials:

```{code} yaml
:filename: .pre-commit-config.yaml
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
        args: ["--fix",]
      - id: ruff-format
```

The star is [Ruff][]: a single ultra-fast Rust binary that replaces Flake8,
isort, pyupgrade, and dozens of other tools. It is both a linter (`ruff-check`)
and a formatter (`ruff-format`, a near-perfect Black clone). Configure the
linter in `pyproject.toml` by opting into rule families:

```{code} toml
:filename: pyproject.toml
[tool.ruff]
show-fixes = true
lint.extend-select = [
  "B",    # flake8-bugbear: likely bugs
  "I",    # isort: sorted imports
  "UP",   # pyupgrade: modernize syntax
  "RUF",  # Ruff-specific rules
]
```

> [!TIP]
> pre-commit runs hooks top-to-bottom, so put content-modifying hooks (fixers,
> formatters) _above_ hooks that just report. And `prek auto-update` bumps every
> `rev:` to the latest tag.

The guide covers many more optional hooks, like spelling ([codespell][]/[typos][]),
[prettier][] for YAML/Markdown/JSON, `check-jsonschema` for CI files,
`clang-format` for C++, and more. Add them as your project grows.

:::{exercise} Lint an untidy file
:label: sp-ruff

Make a file with an unsorted, unused import and a bare `print`, then let Ruff
fix what it can:

```{code} python
:filename: messy.py
import sys
import os

print("hi")
```

:::

:::{solution} sp-ruff
:class: dropdown

```bash
uvx ruff check --fix --show-fixes messy.py   # removes unused `os`, sorts imports
uvx ruff format messy.py                      # normalizes formatting
```

Ruff removes the unused `os` import and sorts the rest; `--show-fixes` reports
exactly what changed. (The `print` is flagged only if you enable the `T20`
rules.)

:::

### Typing

Static type hints are the biggest shift in modern Python. They document intent,
help readers, and---via a type checker---catch whole classes of bugs _before_
you run anything:

```python
def f(x: int) -> int:
    return x * 5
```

Types do nothing at runtime (with `from __future__ import annotations` they're
not even evaluated), but a checker like [MyPy][] verifies you're not lying about
them across every branch; including code your tests rarely hit. Add it as a
pre-commit hook and configure it strictly in `pyproject.toml`:

```{code} toml
:filename: pyproject.toml
[tool.mypy]
files = "src"
python_version = "3.10"
strict = true
```

Typing is **gradual**: MyPy checks almost nothing by default, so you can adopt
it one module at a time and tighten toward `strict = true`.

:::{tip} Fast alternatives
MyPy is the reference checker, but the Rust-based [ty][] (from Astral) and
[Pyrefly][] (from Meta) are probably faster and configured in `pyproject.toml`
as well. Try them if you'd like.
:::

Two features worth knowing early:

::::{tab-set}

:::{tab-item} Narrowing

```python
def g(x: str | int) -> None:
    if isinstance(x, str):
        print(x.lower())  # checker knows x is str here
    else:
        print(x + 1)  # ...and int here
```

The checker "narrows" the type inside each branch.

:::

:::{tab-item} Protocols

```python
from typing import Protocol


class Duck(Protocol):
    def quack(self) -> str: ...


def pester(d: Duck) -> None:
    print(d.quack())
```

Anything with a matching `quack` _is_ a `Duck`; structural typing, no
inheritance or shared dependency required.

:::

::::

A good rule of thumb: **accept the most general type you can, return the most
specific.** Take an `Iterable[str]`, return a `dict[str, int]`, not the other
way around.

> [!IMPORTANT]
> For your package's types to be visible to users, ship an empty `py.typed`
> file in the package. Even typed libraries sometimes forget this step!

See the full [typing guide][MyPy page] for stubs, `TypedDict`, generics, and
more.

### Other pages

The guide keeps going well past this tour. Skim these when the need arises:

| Page                | What it covers                                                                    |
| ------------------- | --------------------------------------------------------------------------------- |
| [CI setup][gha_basic] | GitHub Actions basics, then wheels for [pure][gha_pure] and [compiled][gha_wheels] projects |
| [Docs][docs]          | Setting up and hosting documentation                                            |
| [Task runners][tasks] | Automating jobs with [nox][] (which you installed)                              |
| [Security][security]  | Hardening your project and its supply chain                                     |
| [AI][ai]              | Using agentic AI responsibly on a package (WIP)                                 |

## Using the cookiecutter

Rather than assembling all of this by hand, [cookie][] generates a new package
that already follows the guide. It supports **eleven** build backends,
including the compiled ones (`pybind11`, `scikit-build-core`, `maturin`, …)
you'll use later in this workshop.

Run it with no install using whichever templating tool you prefer:

::::{tab-set}

:::{tab-item} copier

```bash
uvx copier copy gh:scientific-python/cookie my-package
```

The modern choice: the generated repo can pull template updates later.

:::

:::{tab-item} cookiecutter

```bash
uvx cookiecutter gh:scientific-python/cookie
```

The classic choice; no ongoing link to the template.

:::

::::

It asks a handful of questions (name, license, backend, …) and writes a ready-to-go
repository.

:::{exercise} Generate a package
:label: sp-cookie

Generate a package with cookie using the **hatchling** backend. Then create its
environment and run the generated test suite.

:::

:::{solution} sp-cookie
:class: dropdown

```bash
uvx copier copy gh:scientific-python/cookie my-package   # pick "hatchling"
cd my-package
uv run pytest
```

You get passing tests, pre-commit, and CI out of the box. Try `prek run -a` too.

:::

## Using sp-repo-review

Already have a project? [sp-repo-review][] checks it against the guide and gives
you a checklist of what's followed and what's missing. Green means good, red is a
suggestion; some failures are fine, the goal is to _surface_ issues, not force
compliance.

Run it locally on any repository:

```bash
uvx 'sp-repo-review[cli]' <path to repo>
```

Every check has a code (like `PY006` for pre-commit or `PP302` for the pytest
`minversion`) that links back to the exact spot in the guide explaining it.

:::{tip} Run it in your browser
The guide hosts a [live version][repo-review web] that runs entirely in your
browser via WebAssembly; type in any public GitHub repo and branch to grade it
with no install at all.
:::

:::{exercise} Grade a repo
:label: sp-review

Run `sp-repo-review` on the package you generated from cookie, then on an older
project of your own. Compare the results.

:::

:::{solution} sp-review
:class: dropdown

```bash
uvx 'sp-repo-review[cli]' my-package        # freshly generated: mostly green
uvx 'sp-repo-review[cli]' ~/some/old/repo   # older: plenty of red to work through
```

The cookie-generated project should be almost all green; it's built from the
same guide. Your older project shows what modernizing would involve.

:::

[Scientific Python Development Guide]: https://learn.scientific-python.org/development/
[cookie]: https://github.com/scientific-python/cookie
[sp-repo-review]: https://learn.scientific-python.org/development/guides/repo-review/
[repo-review web]: https://learn.scientific-python.org/development/guides/repo-review/
[pytest]: https://learn.scientific-python.org/development/guides/pytest/
[coverage]: https://learn.scientific-python.org/development/guides/coverage/
[MyPy page]: https://learn.scientific-python.org/development/guides/mypy/
[docs]: https://learn.scientific-python.org/development/guides/docs/
[tasks]: https://learn.scientific-python.org/development/guides/tasks/
[security]: https://learn.scientific-python.org/development/guides/security/
[ai]: https://learn.scientific-python.org/development/guides/ai/
[gha_basic]: https://learn.scientific-python.org/development/guides/gha-basic/
[gha_pure]: https://learn.scientific-python.org/development/guides/gha-pure/
[gha_wheels]: https://learn.scientific-python.org/development/guides/gha-wheels/
[pre-commit]: https://pre-commit.com
[prek]: https://prek.j178.dev
[Ruff]: https://docs.astral.sh/ruff/
[MyPy]: https://mypy.readthedocs.io/
[ty]: https://docs.astral.sh/ty/
[Pyrefly]: https://pyrefly.org
[Codecov]: https://about.codecov.io/
[codespell]: https://github.com/codespell-project/codespell
[typos]: https://github.com/crate-ci/typos
[prettier]: https://prettier.io
[nox]: https://nox.thea.codes/
