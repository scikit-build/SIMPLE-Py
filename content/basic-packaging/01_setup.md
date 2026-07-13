# Setting up for development

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/1_01_setup>`

## Before you begin

You'll want to have the following tools installed:

- `uv`: A fast package manager written in Rust that replaces `pip`, `venv`, `pipx`, and more.
- `pixi`: A fast package manager written in Rust for conda packages (high-level only replacement for conda/mamba)
- A system compiler for the later parts of the workshop. (xcode-clt for macOS, Visual Studio for Windows, GCC for Linux)

These tools will be handy too, but they can be all installed via `uv tool install` so you don't need to pre-install them:

- `prek`: A linter/formatter runner, written in Rust (similar to pre-commit)
- `nox`: A task runner

## Low level concepts

(Or how not to break your computer)

The topics in this session are there just to give you an understanding of what's going on. We'll introduce high level abstractions in the next section!

### Virtual environments

The first thing you should know when packaging is how to install stuff.
There are three options:

|                          | Location          | Pros                | Cons                    |
| ------------------------ | ----------------- | ------------------- | ----------------------- |
| **System site-packages** | `/...`            | Works from anywhere | Can break your machine  |
| **User site-packages**   | `~/.local/...`    | User permission     | Can break other Pythons |
| **Virtual environment**  | `.venv/` (common) | Many!               | More effort             |

A system or user install sounds nice, but you can't install incompatible
packages, they can break your system, you can't control what each project
needs, you can end up not knowing what your requirements are, it's hard to
update, etc.

The standard solution is a virtual environment. It places files inside a folder
with a name you choose (`.venv` in the project folder is the standard choice)
that looks like this:

```bash
.venv
├── .gitignore      # These make sure tools know not to include/archive
├── CACHEDIR.TAG    # this folder
│
├── bin
│   ├── activate    # "activation" scripts for different shells (bash default)
│   ...
│   ├── python      # Symlinks to your local Python install
│   ├── python3
│   └── python3.14
│
├── lib
│   └── python3.14
│       └── site-packages  # This is where installed libraries go
│           ...
│
└── pyvenv.cfg      # Special file telling Python this is a virtual environment
```

When Python runs, it checks to see if there's a `pyvenv.cfg` above it. If there is, it knows
it is in a virtual environment (venv) and reads site-packages from there. There are two ways to use it:

::::{tab-set}

:::{tab-item} Direct usage

```bash
.venv/bin/python ...
```

:::

:::{tab-item} Activation

```bash
. .venv/bin/activate  # Shell specific
python ...
deactivate
```

:::

::::

> [!WARNING]
> Some applications might require the environment be set to work. It's rare, but keep that in mind if you use the direct syntax. Things that use `sys.executable` work correctly.

The `.` at the start (most shells support `source` as well) allows the activation script to set environment variables, something a normal application could not do. It sets `PATH` (and `VIRTUALENV`, but that is informational) so things inside the virtual environment's bin are at the beginning of the PATH.

> [!NOTE]
> Conda environments: the `base` environment has many of the same problems. One environment per project is best.
> Conda lets you make environments by name (stored in a directory) or by path.

To create one of these, you have several options:

:::::{card} Create a virtual environment

::::{tab-set}

:::{tab-item} venv

```bash
python3 -m venv .venv
```

This is the slowest, but it's built in![^1]

:::

:::{tab-item} virtualenv

```bash
virtualenv .venv
```

This is faster than `venv`, has better default installs inside, but does require installation.

:::

:::{tab-item} uv

```bash
uv venv
```

This is really fast, though it's completely empty (no pip). And it defaults to `.venv`. Also requires installation (a single Rust binary or pip install).

:::

::::

:::::

We will be using `uv`, which can do a lot of this for us.

[^1]: Distributions may strip it out and make it a separate intallable package.

### Requirements

Now that you know how to make virtual environments, how should you install stuff into them? If the environment is active, then installers will target it. If an environment is not active, `uv pip install` will check for a `.venv` folder, and will target that by default (which is why it doesn't need to install `pip` in the venv).

But a virtual environment is meant to be expendable. You should be able to delete it and recreate it any time. So instead of manually installing, you want to list packages in some format:

:::::{grid} 1 1 2 2

::::{grid-item}

:::{card} Project (app)
These are for making a virtual env. They don't affect libraries.

- **requirement.txt**: Classic, very old format
  - Basically a list of args to pass to `pip install`
- **requirements.in**: Manual locking
  - You make a locked requirements.txt from this file
- **Lock file**: Versions are pinned exactly
- **dependency-groups**: Multiple collections of packages
  :::

  ::::

  ::::{grid-item}

  :::{card} Package (library)
  These are for libraries.

- **dependencies**: The minimum requirements to install your package
- **optional-dependencies**: Sets of extra requirements that can be requested on install

Most libraries also have developer environments, which follows the "Project
(app)" patterns for things like tests and documentation.

> [!WARNING]
> For historical reasons, `optional-dependencies` is sometimes used for
> for these too; this is due to it pre-dating `dependency-groups`.

:::

::::

:::::

Locked dependencies means that every dependency is fully specified, ideally
with a SHA to make sure users get exactly the versions you used. Using a
lockfile is a great way to recover a virtual environment exactly on another
machine. However, dependencies cannot be locked for libraries; it would be a
problem if your two favorite libraries could not be installed together in one
environment because they conflicted on pins!

### Summary

So far, we have discussed:

:::{glossary}
venv
: Virtual environment that isolates dependencies

requirements
: A list of packages to install

lock files
: Fully pinned set of packages
:::

This might seem simple, but creating, activating, installing, and locking are all separate steps with different incantations and different tools.

:::{exercise} Basic virtual environment
:label: basic-venv

Use uv to create a `.venv` virtual environment. Install the `cowsay`
package and then run `cowsay`. Remove the venv folder when done.

:::

:::{solution} basic-venv
:class: dropdown

Assuming a unix-like system and bash:

```bash
uv venv                 # defaults to .venv
uv pip install cowsay   # defaults to .venv
.venv/bin/cowsay        # run without activation is fine
rm -r .venv             # venvs can be removed
```

:::

## High level packaging

### Apps

#### Persistent apps

Let's break up applications and libraries. An application is something you
install and run, but (assuming you use virtual environments) never needs to be
installed with other _unrelated_ things (apps that support plugins are okay).
Generally, you won't `import` an app inside Python, you'll run it from the
command line (or a graphical interface, etc).

This special property allows us to do something interesting. Imagine we:

1. Made a venv somewhere
2. Installed our application in it
3. Put _just_ that application somwehere on our PATH

Since we never need to import it, we can get away without activation. This is exactly what `pipx` (pip for executables) and `uv tool` do!

Use `uv tool install` to install apps. Use `uv tool list` to see what you've
installed. `uv tool upgrade` to upgrade, and `uv tool uninstall` to remove
them. (See `uv tool --help`).

:::{exercise} Tool install
:label: tool-install

Use `uv tool` to install `cowsay`. Run it. Remove the tool when done.

If you have trouble, uv's tool directory might not be on your PATH. It should
warn you and tell you how to fix it.

:::

:::{solution} tool-install
:class: dropdown

Assuming a unix-like system and bash:

```bash
uv tool install cowsay
cowsay
uv tool uninstall cowsay
```

:::

#### Single use apps

We can do one better if we have something we don't want to run all the time.
The install and run steps can be combined! This is such a common need that
`uv` comes with a separate CLI `uvx` that does this. Running this:

```bash
uvx cowsay
```

Will make a venv, download the app, then run a command with the same name in
the venv. If you run it again, it will recreate the venv if it's over a week
old.

With this, you basically have all of PyPI at your fingertips, and you don't
have to remember to update things too!

#### Single file scripts

Another thing we can do is single-file scripts. They look like this:

```{code} python
:filename: simple.py
# /// script
# dependencies = ["numpy"]
# ///

import numpy as np

if __name__ == "__main__":
    print(np.array([1, 2, 3]))
```

When you run it:

::::{tab-set}

:::{tab-item} uv

```bash
uv run simple.py
```

:::

:::{tab-item} pipx

```bash
pipx run simple.py
```

:::

::::

The dependencies will be downloaded into a temporary venv.

#### Projects (websites, etc)

These are very similar to libraries (below); the key difference is you should
always commit your lockfile (vs. being optional for libraries).

How you specify your dependencies depends a bit on the tool you are using,
since it is not standardized like libraries are. But you can always use `[project]`
and make it an unpublishable library by adding this:

```{code} toml
:filename: pyproject.toml
[project]
classifiers = [
  "Private :: Do Not Upload",
]
```

### Libraries

A library is something that can be shared with others. The key feature is that
it must be able to share an environment with whatever else the user of the
library needs. If a user is expected to `import` your code, it's a library.

Later sections will explain a lot more about packages, so let's just present a basic `pyproject.toml`:

```{code} toml
:filename: pyproject.toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "example"
version = "0.1.0"
```

Notice the `[build-system]` section; this tells tools how to build the package
into something you can distribute and install. There are quite a few build
backends; the one above uses `hatchling`.

There are a lot of places to put dependencies; here's an expanded version with annotations:

```{code} toml
:linenos:
:filename: pyproject.toml
:emphasize-lines: 2,8,11,14
[build-system]
requires = ["hatchling"]  # 1
build-backend = "hatchling.build"

[project]
name = "example"
version = "0.1.0"
dependencies = [] # 2

[project.optional-dependencies]
extra = []  # 3

[dependency-groups]
dev = [] # 4
```

:::{glossary}
build-system.requires
: Requirements that are installed when building distributions. This is your
build-backend, and anything else require to assemble your package from source.
These are not available at runtime for users. Noted with `# 1` above.

project.dependencies
: Libraries that must always be installed to use your package. Any installation
of your package will also install these. Noted with `# 2` above.

project.optional-dependencies
: This is a table with arbitrary keys. When a user is installing your package,
they can add `[extra]` to install the list of dependencies named `extra`.
These will not neccisarly be present if the user didn't request then. Also
known as "extras". These are part of the public package metadata. Noted with
`# 3` above.

dependency-groups
: This is a table with arbitrary keys. Unlike project.optional-dependencies, these
do not become part of the package metadata; you must have the `pyproject.toml` file
to install them. They also do not require you install the package. Commonly
used for development dependencies, like tests, docs, coverage, and the like.
Noted with `# 4` above.
:::

A quick comparison:

|                                | `b-s.r` | `p.d` | `p.o-d` | `d-g` |
| ------------------------------ | ------- | ----- | ------- | ----- |
| Public metadata                | ✅      | ✅    | ✅      | ❌    |
| Always installed               | ❌      | ✅    | ❌      | ❌    |
| Named groups                   | ❌      | ❌    | ✅      | ✅    |
| Can be independently installed | ❌      | ❌    | ✅      | ✅    |

#### High level project management with uv

When dependency-groups were introduced, uv made a brilliant decision: if there's
a group named `dev`, it is installed by default when using `uv run`. This enables
the following file:

```{code} toml
:filename: pyproject.toml
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

to be all you need to use `uv run`. For example,

```bash
uv run pytest
```

will:

- Download Python if you don't have a valid copy
- Make a virtual environment at `.venv` if it doesn't exist
- Install dependencies from a `uv.lock` lockfile if it exists
- Create a `uv.lock` file from `project.dependencies` and `dependency-groups.dev` if it doesn't exist and install
- Make an editable install of your project
- Run whatever command you give it from that virtual environment

If you edit the dependencies, then the lockfile and `.venv` will be updated
when you `uv run` again.

Any command works, so `uv run python` starts up `python`, if you have command
line apps you can use `uv run ...`, etc.

:::{exercise} Development environment
:label: uv-dev-env

The most downloaded Python package in the world is probably `packaging` (not
counting AWS). Download the `https://github.com/pypa/packaging` repository from
github, and run the test suite with `pytest` using uv.

:::

:::{solution} uv-dev-env
:class: dropdown

Assuming a unix-like system and bash:

```bash
git clone https://github.com/pypa/packaging
cd packaging
uv run pytest
```

:::

## Editable installs

We will cover editable installs in detail, but it's something worth calling out.
When you install a package for development (including via `uv run`), you do an
editable install, which is a bit different than a normal install. It creates
a special wheel that simply exists in order to put special code into site-packages
to enable Python to find your files. Pretty much everything about this is left up
to the build backend. Some common mechanisms, compared to a classic install, are:

::::{tab-set}

:::{tab-item} Standard

```text
site-packages
├── example                    # Your package, copied out of the wheel
│   └── __init__.py
└── example-0.1.0.dist-info    # Metadata (version, RECORD of files, ...)
    ├── METADATA
    ├── RECORD
    └── ...
```

Your files are copied into site-packages; editing your source does nothing
until you reinstall.

:::

:::{tab-item} Editable (path file)

```text
site-packages
├── _editable_impl_example.pth  # One line: the path to your project's src/
└── example-0.1.0.dist-info
    └── ...
```

At startup, Python adds every path listed in a `.pth` file to `sys.path`, so
your source directory is importable directly. This is what hatchling does (and
setuptools with a src layout, named `__editable__.example-0.1.0.pth` instead).

:::

:::{tab-item} Editable (import hook)

```text
site-packages
├── _editable_example-0.1.0.pth        # Runs .install() on the finder below
├── _editable_example_0_1_0_finder.py  # Maps "example" to your source
└── example-0.1.0.dist-info
    └── ...
```

A line in a `.pth` file starting with `import` is executed instead, which is
(ab)used here to register a custom import finder that maps just your package
to its source.

:::

:::{tab-item} Python 3.15+ (import hook)

```text
site-packages
├── _editable_example-0.1.0.start      # Runs an entry-point on the finder below
├── _editable_example_0_1_0_finder.py  # Maps "example" to your source
└── example-0.1.0.dist-info
    └── ...
```

A line in a `.start` file defines an entry point to run, which is used here to
register a custom import finder that maps just your package to its source.

:::

::::
