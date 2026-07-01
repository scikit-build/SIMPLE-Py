# Contributing to SIMPLE-Py

Contributions are welcome! This project is a workshop book built with [MyST](https://mystmd.org/) and accompanied by Marp slides.

## Setup

### Prerequisites

- [bun](https://bun.sh/) (for building the book and slides)
- [prek](https://github.com/j178/prek) (for linting)

Install dependencies with:

```bash
bun install
```

## Building the book

```bash
bun run build-book
```

The output is placed in `_build/html/`.

## Building the slides

```bash
bun run build-slides
```

The output is placed in `_build/html/slides/`.

To build both at once, run `bun run build`.

## Linting

Run prek on all files:

```bash
prek -a
```

## Making changes

- **Book content**: Edit the Markdown files in `content/`. The table of contents is defined in `myst.yml`.
- **Slides**: Edit the Markdown files in `slides/`. Slides use [Marp](https://marp.app/) format with `marp: true` frontmatter.
