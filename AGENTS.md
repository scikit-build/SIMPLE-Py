# Agent instructions

## What this is

SIMPLE-Py is a workshop that teaches simple compiled Python packaging (C++, Rust, publishing). It is **content, not code**: a [MyST](https://mystmd.org/) book plus [Marp](https://marp.app/) slides, deployed to GitHub Pages. There is no Python package to build or test here — edits are to Markdown.

## Commands

Package manager is **bun** (see `bun.lock`; CI runs `bun install --frozen-lockfile`).

```bash
bun run serve         # live-preview the book (myst start)
bun run build         # build book + slides into _build/html/ (build-book then build-slides)
bun run build-book    # myst build --html
bun run build-slides  # marp slides/ -> _build/html/slides/
bun run clean         # rm -rf _build
prek -a --quiet       # lint/format everything (ruff-format, blacken-docs, prettier, codespell, etc.)
```

## Structure

- **`content/`** — book chapters as Markdown, grouped by section directory (`basic-packaging/`, `compiled/`, `scikit-build/`, `other-tools/`, `interesting/`). Files are prefixed with an order number (`01_`, `02_`, …). Images live alongside their chapter, prefixed with the chapter stem (e.g. `04_distros-shipping.jpg`).
- **`myst.yml`** — book config and the **table of contents**. Adding or reordering a chapter file requires editing the `toc` here; the filesystem order is not authoritative.
- **`slides/`** — Marp decks (`marp: true` frontmatter, `theme: simplepy` → `slides/simplepy.css`). Named `<section>_<chapter>_<name>.md` where the leading digit ties the deck to its content section.

## Conventions

- `blacken-docs` formats Python code blocks inside Markdown, and `ruff-format` runs on code — keep embedded code snippets valid and formatted.
- CI (`.github/workflows/cd.yml`) builds with `BASE_URL: /SIMPLE-Py` and deploys to Pages on push to `main`.
