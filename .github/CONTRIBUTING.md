# Contributing to SIMPLE-Py

Contributions are welcome! This project is a workshop book built with [MyST](https://mystmd.org/) and accompanied by Marp slides.

## Setup

### Prerequisites

- [Node.js](https://nodejs.org/) (for building the book and slides)
- [prek](https://github.com/j178/prek) (for linting)

## Building the book

```bash
npx mystmd build --html
```

The output is placed in `_build/html/`.

## Building the slides

```bash
npx @marp-team/marp-cli@latest --input-dir slides --output _output
```

The output is placed in `_output/`.

## Linting

Run prek on all files:

```bash
prek -a
```

## Making changes

- **Book content**: Edit the Markdown files in `content/`. The table of contents is defined in `myst.yml`.
- **Slides**: Edit the Markdown files in `slides/`. Slides use [Marp](https://marp.app/) format with `marp: true` frontmatter.
