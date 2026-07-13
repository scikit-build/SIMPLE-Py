---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Agentic AI in scikit-build-core's development

---

## What is Agentic AI?

An **interface** (like a text editor) — but instead of writing code, you prompt
an **agent**: an LLM with tooling that lets it perform actions.

<div class="columns">
<div>

> make a PR

Inspects the changes, makes a branch, commits, pushes, opens a PR — message
and description generated.

</div>
<div>

> investigate #123

Looks up the issue, tries to build a reproducer, fixes it if the reproducer
works.

</div>
</div>

**Marked in scikit-build repos**: an "AI text below" header on generated PR
text, and an `Assisted-by:` trailer on commits.

---

## Sweeping code reviews

One prompt, run over the whole project:

> Review this project for bugs, performance, simplifications, and
> modernizations

- scikit-build-core: **15 bugs, 4 serious**
  ([#1317](https://github.com/scikit-build/scikit-build-core/issues/1317)) — on
  a from-scratch codebase with high test coverage
- 30+ projects reviewed this way: **~400 bugs** + as many perf/cleanup fixes
- Near-**100% merge rate** on the resulting PRs

Model choice matters: the strongest models find far more, with a very low
false-positive rate.

---

## Testing ~20 downstream projects

Does the development branch break anyone downstream?

- Already had `nox -s downstream -- <...>` to build a project against
  scikit-build-core
- Claude wrote a **`SKILL.md`**: build with last release, build with `main`,
  try editable installs, report any regressions
- One prompt fires off **many subagents** — capped in parallel to manage
  system resources

Tedious, parallelizable work — exactly what agents are good at.

---

## Downstream results

[#1439](https://github.com/scikit-build/scikit-build-core/pull/1439):
20 projects, `main` vs. `v0.12.2`

- ✅ **16 parity** — iminuit, spglib, rapidfuzz, gemmi, boost-histogram,
  nanobind, pyzmq, ...
- ⚪ **2 upstream-only failures** — astyle & symusic fail identically on both
  sides (not us)
- 🔴 **2 real regressions** in unreleased `main`:
  - **manifold3d** — new path check hard-errored on a CMake-installed package
    (fixed in [#1440](https://github.com/scikit-build/scikit-build-core/pull/1440))
  - **coreforecast** — a warning became an aborting error without a pinned
    `minimum-version`

Both caught **before** they shipped.

---

## Developing against downstream: PyTorch

[pytorch/pytorch#180247](https://github.com/pytorch/pytorch/pull/180247) moves
PyTorch from setuptools to scikit-build-core.

- Claude wrote a **pain-points report** on the transition
  ([#1367](https://github.com/scikit-build/scikit-build-core/issues/1367))
- The report turned into **4–5 upstream PRs**
- For one, Claude **adapted the PyTorch PR** against the upstream change —
  surfacing a problem early enough to address
- It even contributed the "diff for 1.0+ support" back to the PyTorch PR

---

## Working through the issue backlog

scikit-build-core had **140+ open issues** — now around **20**.

- Claude found all the **reproducible** bug reports
- Subagents produced fixes: **10 PRs** → 6–7 mergeable as-is, ~2 needed extra
  work, 1 closed
- A full [categorization of the backlog](https://github.com/orgs/scikit-build/discussions/1343)
  targeted the remaining work for the **1.0 release**

---

## Porting scikit-build (classic)

Classic scikit-build is now a thin layer on scikit-build-core's setuptools
plugin.

- Conversion spanned **three repositories**, including
  [scikit-build-sample-projects](https://github.com/scikit-build/scikit-build-sample-projects)
- The agent made all the test adjustments and tested the samples repo
- Found and fixed **seven bugs** in the setuptools plugin along the way —
  locating the local checkout, applying fixes there, and offering a PR

Agents are happy (proactive, even) to work **across repos** — the coordination
that usually makes a port drag on.

---

## Other wins

- Test suite **40% faster**, same coverage
- Long-standing **editable-install** bugs worked out
- Bugs from up to **six years ago** — the kind that never reach the top of a
  maintainer's list — finally fixed

---

## Takeaways

- **It scales** — build a skill or working procedure once, then launch many
  agents in parallel
- **Make it do what you wouldn't** — sweeping reviews, 20-project regression
  sweeps, six-year-old bugs, three-repo ports: the tedious wins are the
  biggest
- **Verify like it's a contributor** — regression tests, CI, and human review
  still apply; that's where the ~100% merge rate comes from
