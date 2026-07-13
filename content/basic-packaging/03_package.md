# Making a basic package

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/1_03_package>`

In the last chapters you learned how environments and dependencies work, and
you saw a minimal `pyproject.toml`. Now you'll build a real package from
scratch, by hand, so that every file in it is something you understand. By the
end of this hour you will have a package that:

- lives in a standard `src` layout,
- installs (editably) into an environment,
- has complete, standards-based metadata,
- provides a command-line script,
- gets its version from git, and
- builds into an SDist and a wheel you can inspect.

We'll build everything manually this time; the next chapter shows the
template that generates all of this (and more) for you.

## From script to package

Much code starts life in a notebook or a script. Ours is a single function:

```python
import numpy as np


def rescale(input_array):
    """Rescale an array so its values span [0, 1]."""
    low = np.min(input_array)
    high = np.max(input_array)
    return (input_array - low) / (high - low)
```

It works (`rescale(np.linspace(0, 100, 5))` gives `[0, 0.25, 0.5, 0.75, 1]`),
but you can't `pip install` it, can't `import` it from another project, and
can't share it. Let's fix that.

### The layout

Make a new directory with a git repo, and lay out the files like this:

```text
rescale
├── pyproject.toml
└── src
    └── rescale
        ├── __init__.py
        └── core.py
```

- `core.py` holds the function above.
- `__init__.py` marks the directory as a package, and defines your public API:

```{code} python
:filename: src/rescale/__init__.py
from rescale.core import rescale

__all__ = ["rescale"]
```

> [!NOTE]
> Why the extra `src` level? If your package sits in the project root, `python`
> and `pytest` will happily import the local folder instead of the installed
> package, hiding packaging bugs (like forgetting to include a file) until a
> user hits them. The `src` layout forces everything through a real install.
> It also matches how compiled projects are laid out, which will pay off later
> in this workshop.

### The pyproject.toml

The only other required file is `pyproject.toml`. Two tables matter:

```{code} toml
:filename: pyproject.toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "rescale"
version = "0.1.0"
dependencies = ["numpy"]
```

`[build-system]` selects a **build backend**: the tool that turns your source
tree into something installable. The frontend (pip, uv, ...) installs
`requires` into an isolated environment and asks the `build-backend` to do the
build. All backends read the same standard `[project]` table, so switching is
easy; they differ in file selection, dynamic versioning, and extras:

::::{tab-set}

:::{tab-item} Hatchling

```toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"
```

A great default: fast, extendable, good file-inclusion defaults (it reads
your `.gitignore`). We'll use it in this chapter.

:::

:::{tab-item} uv_build

```toml
[build-system]
requires = ["uv_build>=0.7.19"]
build-backend = "uv_build"
```

uv's own backend; very fast, intentionally minimal, pairs well with `uv init`.

:::

:::{tab-item} Flit-core

```toml
[build-system]
requires = ["flit_core>=3.12"]
build-backend = "flit_core.buildapi"
```

Tiny and dependency-free; the backend many core PyPA tools use themselves.

:::

:::{tab-item} Setuptools

```toml
[build-system]
requires = ["setuptools>=77"]
build-backend = "setuptools.build_meta"
```

The classic. Modern setuptools reads `[project]` too; you only need
`setup.py`/`MANIFEST.in` for legacy features.

:::

::::

> [!NOTE]
> `name` is the install name (`pip install rescale`), and the folder under
> `src/` is the import name (`import rescale`). Keep them identical if you can
> (normalizing `-` to `_`); backends auto-detect the package when they match.

### Install it and use it

Now install your package into an environment. You have a low-level and a
high-level option:

::::{tab-set}

:::{tab-item} uv run (high level)

```bash
uv run python -c "import rescale, numpy; print(rescale.rescale(numpy.linspace(0, 100, 5)))"
```

Remember from the setup chapter: `uv run` makes the venv, installs your
package editably along with its dependencies, and runs the command. There is
no step two.

:::

:::{tab-item} pip install -e (low level)

```bash
uv venv
uv pip install -e .
.venv/bin/python -c "import rescale, numpy; print(rescale.rescale(numpy.linspace(0, 100, 5)))"
```

The `-e` makes it an editable install (covered in the setup chapter): edits
to `src/rescale/` are visible on the next `import`, no reinstall needed.

:::

::::

:::{exercise} Build the package
:label: pkg-minimal

Create the `rescale` package exactly as above: the `src` layout, the two
Python files, and the minimal `pyproject.toml`. Initialize a git repo and
commit. Then prove it works by importing it and rescaling
`numpy.linspace(0, 100, 5)`.

Bonus: temporarily delete `dependencies = ["numpy"]`, remove `.venv` and
`uv.lock`, and see what happens when you try again.

:::

:::{solution} pkg-minimal
:class: dropdown

```bash
mkdir rescale && cd rescale
git init
mkdir -p src/rescale
# create core.py, __init__.py, and pyproject.toml as above
git add -A && git commit -m "feat: initial package"
uv run python
>>> import numpy as np
>>> from rescale import rescale
>>> rescale(np.linspace(0, 100, 5))
array([0.  , 0.25, 0.5 , 0.75, 1.  ])
```

Without the `dependencies` line, the import fails with
`ModuleNotFoundError: No module named 'numpy'`: your package installed fine,
but nothing told the installer that NumPy must come along.

:::

### A first test

Let's lock in that manual check as a real test, in a `tests/` folder next to
`src/` (not inside it):

```{code} python
:filename: tests/test_core.py
import numpy as np

from rescale import rescale


def test_rescale():
    np.testing.assert_allclose(
        rescale(np.linspace(0, 100, 5)),
        np.array([0.0, 0.25, 0.5, 0.75, 1.0]),
    )
```

pytest is a development-only dependency, so it goes in a dependency group,
not in `dependencies`:

```{code} toml
:filename: pyproject.toml
[dependency-groups]
dev = ["pytest"]
```

Since uv installs the `dev` group by default, running the tests is just:

```bash
uv run pytest
```

The next chapter covers testing properly; for now, one passing test is enough
to tell us we haven't broken anything while we work on the metadata.

## Filling out the metadata

`name` and `version` are the only required fields, but a real package should
tell its users (and PyPI) much more. Everything goes in the same standard
`[project]` table, regardless of backend.

### Informational metadata

These fields describe your package to humans and search engines:

```{code} toml
:filename: pyproject.toml
[project]
name = "rescale"
version = "0.1.0"
description = "Rescale NumPy arrays to span [0, 1]."
readme = "README.md"
authors = [{ name = "My Name", email = "me@email.com" }]
license = "BSD-3-Clause"
license-files = ["LICENSE"]
keywords = ["arrays", "normalization"]
classifiers = [
  "Development Status :: 3 - Alpha",
  "Intended Audience :: Science/Research",
  "Programming Language :: Python :: 3",
  "Topic :: Scientific/Engineering",
  "Private :: Do Not Upload",
]

[project.urls]
Homepage = "https://github.com/me/rescale"
"Bug Tracker" = "https://github.com/me/rescale/issues"
Changelog = "https://github.com/me/rescale/releases"
```

A few of these deserve a closer look:

license
: The modern form is an [SPDX identifier expression][spdx] like
`"BSD-3-Clause"` or `"MIT AND (Apache-2.0 OR BSD-2-Clause)"`, plus
`license-files` globs pointing at the license text. You'll see older packages
use a `License ::` classifier or `license = { file = "..." }` table instead;
don't mix the old and new styles. And never write your own license text; pick
a standard one from [choosealicense.com][].

classifiers
: Free-form-ish tags from a [fixed list][classifiers]. They help searching
and communicate stability ("Development Status"). The special
`Private :: Do Not Upload` classifier isn't real, which means PyPI will
_reject_ the package; perfect for tutorials and internal code. Remove it when
you mean to publish.

readme
: Points at your `README.md`; its contents become the package's long
description, rendered on the PyPI page.

### Functional metadata

These fields change how installers treat your package:

```{code} toml
:filename: pyproject.toml
[project]
# ...
requires-python = ">=3.10"
dependencies = ["numpy>=1.24"]

[project.optional-dependencies]
plot = ["matplotlib"]
```

requires-python
: Installers use this to _back-solve_: `pip install rescale` on an old Python
walks back through your releases until it finds one whose `requires-python`
passes. That's what lets you drop old Pythons safely. It only works in that
direction; **never add an upper cap** like `<4` here, it breaks the
back-solving logic and helps nobody.

dependencies
: Version specifiers support lower bounds (`>=1.24`), caps (`<3`), and pins
(`==2.1.0`). For a library: set lower bounds you actually test, and avoid
upper caps unless you _know_ the next major version breaks you (see
[bound version constraints][] for why). Exact pins belong in lockfiles, not
here; remember from the setup chapter that your users must be able to share
an environment with other packages.

optional-dependencies
: The extras from the setup chapter: users opt in with
`pip install 'rescale[plot]'`. Unlike dependency groups, extras are published
metadata and available to your users from PyPI.

:::{exercise} project.dependencies vs. build-system.requires
:label: pkg-deps-quiz

You've now seen both. What's the difference between putting `numpy` in
`build-system.requires` versus `project.dependencies`?

:::

:::{solution} pkg-deps-quiz
:class: dropdown

`build-system.requires` is installed into a temporary, isolated environment
_while building_ your package, then thrown away; it never reaches your users
(a wheel doesn't even contain `pyproject.toml`). `project.dependencies`
becomes wheel metadata, and installers pull those packages in whenever someone
installs yours. A pure Python package almost never needs anything besides the
backend in `build-system.requires`; compiled packages will use it more.

:::

### Entry points: adding a command line script

To give your package a shell command, add a `scripts` entry point mapping a
command name to `package.module:function`:

```{code} toml
:filename: pyproject.toml
[project.scripts]
rescale = "rescale.__main__:main"
```

with a matching function:

```{code} python
:filename: src/rescale/__main__.py
import sys

import numpy as np

from rescale.core import rescale


def main() -> None:
    values = np.array([float(x) for x in sys.argv[1:]])
    print(rescale(values))


if __name__ == "__main__":
    main()
```

On install, the installer generates a real `rescale` executable in the
environment's `bin/`. Using `__main__.py` as the file also makes
`python -m rescale` work for free.

:::{exercise} Metadata and a CLI
:label: pkg-metadata

Give your package the full metadata above (adjust the author!), plus the
`rescale` script. Then:

1. Check the metadata with `uv run --refresh-package rescale --with pip pip show -v rescale`
   (or `uv run --with pip pip show -v rescale` if you installed manually).
2. Run your new command: it's an app in your venv, so `uv run rescale 1 2 3`.

:::

:::{solution} pkg-metadata
:class: dropdown

```bash
uv run rescale 1 2 3
[0.  0.5 1. ]
uv run --with pip pip show -v rescale
Name: rescale
Version: 0.1.0
Summary: Rescale NumPy arrays to span [0, 1].
...
Entry-points:
  [console_scripts]
  rescale = rescale.__main__:main
```

If the metadata looks stale, uv cached the previous build; `uv sync --reinstall-package rescale`
or the `--refresh-package` flag forces a rebuild.

:::

## Versioning

A quick word on choosing version numbers, then the fun part: never writing
them by hand again.

The common schemes are **SemVer** (`major.minor.patch`) and **CalVer**
(date-based, like pip's `25.1`). Treat SemVer as an abbreviated changelog
expressing author intent — patch: "nothing to see", minor: "new stuff
available", major: "you should look before upgrading" — not as a promise that
nothing will ever break; with enough users, _every_ change breaks someone.
That's also why you shouldn't preemptively pin `package<2` in your
dependencies.

### Single-sourcing the version

Right now the version lives in `pyproject.toml`, and users may also expect
`rescale.__version__`. Two copies drift. Backends can compute the version for
you; you declare it `dynamic` and configure the backend:

::::{tab-set}

:::{tab-item} From git tags (hatch-vcs)

```{code} toml
:filename: pyproject.toml
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

Now `git tag v0.2.0` _is_ the release process; commits after a tag get dev
versions like `0.2.1.dev3`, so every commit is uniquely identifiable. The
generated `_version.py` is a build artifact: add it to `.gitignore`, and
re-export it:

```{code} python
:filename: src/rescale/__init__.py
from rescale._version import __version__
```

:::

:::{tab-item} From a file (hatchling)

```{code} toml
:filename: pyproject.toml
[project]
name = "rescale"
dynamic = ["version"]

[tool.hatch]
version.path = "src/rescale/__init__.py"
```

Hatchling reads `__version__ = "0.1.0"` out of your source, so the source is
the single copy and the metadata follows it.

:::

::::

:::{dropdown} Versions in GitHub's tarballs

Git tags aren't included in `git archive` output, which is what GitHub's
"Download source" tarballs are. Two small files fix that. `.git_archival.txt`:

```text
node: $Format:%H$
node-date: $Format:%cI$
describe-name: $Format:%(describe:tags=true,match=*[0-9]*)$
```

and a line in `.gitattributes`:

```text
.git_archival.txt  export-subst
```

Git substitutes the real values when creating the archive, and hatch-vcs
knows to read them.

:::

:::{exercise} Version from git
:label: pkg-version

Switch your package to hatch-vcs versioning. Commit everything, tag `v0.2.0`,
and confirm the installed package reports it:

```bash
uv run python -c "import rescale; print(rescale.__version__)"
```

Then make one more commit and check the version again.

:::

:::{solution} pkg-version
:class: dropdown

```bash
git add -A && git commit -m "build: version from VCS"
git tag v0.2.0
uv sync --reinstall-package rescale
uv run python -c "import rescale; print(rescale.__version__)"
0.2.0
```

After another commit, you'll see something like `0.2.1.dev1+g1a2b3c4`: one
commit past the tag, at that git hash. (You need to reinstall to see version
changes; the version is computed at build time, and editable installs don't
rebuild on their own.)

:::

## The supporting files

Your repository needs a few files that aren't code but make it a project
someone else can use:

| File            | What it's for                                                                                                                                                       |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `README.md`     | Name, one-paragraph description, install command, tiny usage example. Keep it brief; details belong in docs. Rendered on GitHub and PyPI.                            |
| `LICENSE`       | The exact text of the license you declared in `pyproject.toml`. Without one, default copyright applies and nobody may legally use your code.                         |
| `.gitignore`    | Start from [GitHub's Python template][gitignore]. Doubly important with hatchling, which uses it to decide what _doesn't_ go into your package.                      |
| `CHANGELOG.md`  | Human-readable list of changes per version ([keep a changelog][]). GitHub Releases can also fill this role.                                                          |

:::{exercise} Round out the repo
:label: pkg-files

Add a `README.md` with a description, install instructions, and a usage
example, a BSD-3-Clause `LICENSE` (from [choosealicense.com][]), and GitHub's
Python `.gitignore`. Make sure `readme` and `license-files` in your
`pyproject.toml` point at the right names, then commit.

:::

:::{solution} pkg-files
:class: dropdown

```bash
curl -o .gitignore https://raw.githubusercontent.com/github/gitignore/main/Python.gitignore
echo "src/rescale/_version.py" >> .gitignore
# write README.md, paste LICENSE text (update year and name)
git add -A && git commit -m "docs: add README, LICENSE, gitignore"
```

`uv run --with pip pip show -v rescale` should now show your readme-derived description
metadata, and `git status` should be quiet even after running tests.

:::

## Building your first distributions

Installing directly from the source tree is for development. To _distribute_
a package, you build it into two standard artifacts:

SDist
: The **source distribution**: a `.tar.gz` of your source tree plus metadata.
Installing from it runs the build backend on the user's machine.

wheel
: A **built distribution**: a `.whl` zip file that is simply unpacked into
`site-packages`. No code runs at install time, making installs fast and safe.
For pure Python, one wheel (`py3-none-any`) works everywhere; compiled
packages need one per platform, which is most of the rest of this workshop.

Build both with:

```bash
uv build
```

(or `pipx run build`; both call your build backend per the standard
interface). The results land in `dist/`, and you can (and should!) look
inside:

```bash
tar -tf dist/*.tar.gz    # SDist contents
unzip -l dist/*.whl      # wheel contents
```

:::{exercise} Inspect your distributions
:label: pkg-build

Build your package and examine both artifacts. Find:

1. Where the `[project]` table ended up in the wheel.
2. Whether `tests/` made it into each artifact, and whether that's a problem.
3. What `RECORD` is.

:::

:::{solution} pkg-build
:class: dropdown

```bash
uv build
unzip -p dist/*.whl '*/METADATA' | head -20
```

1. The wheel's `rescale-0.2.0.dist-info/METADATA` file holds your metadata,
   rendered from `[project]` into the core-metadata format; your readme is its
   body. Note there's no `pyproject.toml` in the wheel at all.
2. Hatchling includes `tests/` and `README.md` in the SDist (anything
   tracked by git), but only `src/rescale/` ends up importable in the wheel.
   That's the right default: the SDist should be able to rebuild everything,
   including running tests, while the wheel ships only what users import.
3. `RECORD` lists every installed file with a hash, so uninstalls and
   upgrades are exact.

:::

## Summary

:::{glossary}
build backend
: The tool (hatchling here) that turns your source tree into installable
artifacts. Selected in `[build-system]`, configured under `[tool.*]`.

src layout
: Keeping code in `src/<package>/` so nothing works without a real install.

`[project]` table
: The standardized metadata: name, version, dependencies, entry points, and
everything PyPI displays. Identical across backends.

entry point
: Metadata mapping a command name to a Python function; installers generate
the executable.

SDist / wheel
: Source vs. built distribution. Ship both.
:::

You built all of this by hand once so it holds no mysteries. You will
probably never do it by hand again: the next chapter introduces the
[Scientific Python Development Guide][] and its template, which generates a
package like this, plus tests, linting, typing, and CI, in one command.

[spdx]: https://spdx.org/licenses
[classifiers]: https://pypi.org/classifiers/
[choosealicense.com]: https://choosealicense.com
[bound version constraints]: https://iscinumpy.dev/post/bound-version-constraints/
[gitignore]: https://github.com/github/gitignore/blob/main/Python.gitignore
[keep a changelog]: https://keepachangelog.com
[scientific python development guide]: https://learn.scientific-python.org/development/
