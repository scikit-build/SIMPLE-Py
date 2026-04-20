# Setting up for development

## Before you begin

You'll want to have the following tools installed:

- `uv`: A fast package manager written in Rust that replaces `pip`, `venv`, `pipx`, and more.
- `pixi`: A fast package manager written in Rust for conda packages (high-level only replacement for conda/mamba)
- A system compiler for the later parts of the workshop. (xcode-clt for macOS, Visual Studio for Windows, GCC for Linux)

These tools will be handy too, but they can be all installed via `uv tool install` so you don't need to pre-install them:

- `prek`: A linter/formatter runner, written in Rust (similar to pre-commit)
- `nox`: A task runner

## Basic concepts

(Or how not to break your computer)

### Virtual environments

The first thing you should know when packaging is how to install stuff.
