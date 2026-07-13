---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Scientific Installation & Modern Packaging for Language Extensions in Python

_Support for this work was provided by NSF grant OAC-2209877. Any opinions, findings, and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the National Science Foundation._

---

## Welcome

Some logistics:

- Everyone should have gotten a badge
- We'll be sharing catering with SciPy tutorials
- We have enough leftover funds for a workshop dinner (need to find a place!)


---

## Who we are

- Around 18 of us here
- **Wide** range of prior knowledge, from novice to expert!


It's hard to target everyone! So let's try a few things:

- Focus on interactivity - questions and talking is good (workshop)
- Will have a mix of overviews and in-depth dives
- Everyone might see some new stuff!
- Groups can break out to work on stuff if you want


---

## What we'll cover

1. **Basic Packaging** — venvs, `uv`, `pyproject.toml`, publishing
2. **Compiled** — C++ bindings, Rust, building wheels everywhere
3. **scikit-build-core** — CMake-driven builds, stable ABI, editable installs
4. **Other Tools** — plugins, conda-forge, distros
5. **Interesting topics** — free-threading, lazy imports, AI, and CUDA

And we have an open hack on day 2!

---

## Prerequisites

<div class="columns">
<div>

### Install ahead of time

- `uv` — package manager
- `pixi` — conda package manager
- A system compiler

</div>
<div>

### Handy (via `uv tool install`)

- `prek` — linter runner
- `nox` — task runner

</div>
</div>

The first talk walks through setup in detail.

---

## How this works

- **Slides** frame each topic; the **book** has the full details
- Every chapter links to its slides
- Follow along hands-on workspace (and exercises)
- Ask questions any time!

📖 <https://scikit-build.org/SIMPLE-Py>

---

## Henry: material preparation

- [scikit-build-core 1.0](https://scikit-build-core.readthedocs.io/en/latest/about/changelog.html) took priority for a while
- A lot of my material was collected from other things I've written
- Slides were summarized from page content
- If you get bored with the plain slides, go to <https://scikit-build.org>, dark mode, and scroll up and down. :)

---

## Sources

- https://iscinumpy.dev
- https://hsf-training.github.io/hsf-training-cmake-webpage/
- https://intersect-training.org/packaging/
- https://cliutils.gitlab.io/modern-cmake/
- https://learn.scientific-python.org/development
- https://github.com/scikit-build/scikit-build-core
- https://github.com/scikit-build/scikit-build-sample-projects
- https://github.com/henryiii/python-compiled-minicourse

---

## Let's get started

Next up: **Setting up for development** →
