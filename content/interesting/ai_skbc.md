# Agentic AI in scikit-build-core's development

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/6_03_ai_skbc>`

Agentic AI was a big part of the 1.0 push for scikit-build-core. This chapter
highlights some of the work that was helped by Agentic AI. For a general
introduction, see [Starting with Agentic
AI](https://iscinumpy.dev/post/starting-with-agentic-ai/) and [Claude Code
Reviews](https://iscinumpy.dev/post/claude-code-reviews/).

If you are not familiar with _Agentic AI_, it's an _interface_ (like a text
editor), but instead of directly writing code, you are prompting an "agent",
which is an LLM with tooling that allows it to perform actions. An example: if
you open it and type "make a PR", it will inspect the current changes, make a
branch, commit, push, and open a PR, including a generated commit message and
PR description. If you say "investigate #123", it will look up the issue,
try to make a reproducer, and fix it if the reproducer works. Etc.

:::{note}
All AI-assisted work in the scikit-build repos is marked: PR descriptions carry
an "AI text below" header if generated text is included, and commits get an
`Assisted-by:` trailer, so you can see exactly which changes were made this
way.
:::

## Sweeping code reviews

A single prompt, run over the whole project:

> Review this project for bugs, performance, simplifications, and
> modernizations

On scikit-build-core --- a codebase written from scratch by its maintainer, with
high test coverage --- one sweep found **15 bugs, 4 of them serious**
([#1317](https://github.com/scikit-build/scikit-build-core/pull/1317)). Across
30+ projects reviewed the same way, that's roughly 400 bugs plus as many
performance and cleanup fixes, with a near-100% merge rate on the resulting PRs.

Model choice matters a lot here: the strongest models find substantially more
with a very low false-positive rate, while smaller open models find some issues
with a moderate false-positive rate.

## Testing ~20 downstream projects

Before a release, does the development branch break anyone downstream? This is
exactly the kind of tedious, parallelizable work agents are good at:

- There was already a `nox -s downstream -- <...>` session that can build a
  downstream project against scikit-build-core.
- Claude wrote a `SKILL.md` describing the procedure: build with the last
  released scikit-build-core, build with the current development branch, try
  editable installs too, and produce a report describing any regressions.
- With the skill in place, one prompt fires off many subagents (with a cap on
  how many run in parallel, to manage system resources).

The result
([#1439](https://github.com/scikit-build/scikit-build-core/pull/1439)): 20
downstream projects tested, `main` vs. `v0.12.2`:

- ✅ **16 parity** --- iminuit, spglib, rapidfuzz, gemmi, boost-histogram,
  nanobind, pyzmq, ...
- ⚪ **2 upstream-only failures** --- astyle and symusic fail identically on
  both sides (not scikit-build-core's fault)
- 🔴 **2 real regressions** in unreleased `main`:
  - **manifold3d** --- a new path check hard-errored on a CMake-installed
    package that was previously skipped silently (fixed in
    [#1440](https://github.com/scikit-build/scikit-build-core/pull/1440))
  - **coreforecast** --- a renamed-field warning became an aborting error for
    projects without a pinned `minimum-version`

## Developing against downstream: PyTorch

[pytorch/pytorch#180247](https://github.com/pytorch/pytorch/pull/180247) moves
PyTorch from setuptools to scikit-build-core --- a large, demanding downstream
user. Agents helped close the loop between the two projects:

- Claude produced a report on the pain points of the transition
  ([#1367](https://github.com/scikit-build/scikit-build-core/issues/1367)).
- That report turned into 4--5 upstream PRs in scikit-build-core.
- For one of them, Claude adapted the PyTorch PR against the upstream change,
  which surfaced a problem early enough to address it.
- It even contributed the "diff for 1.0+ support" back to the PyTorch PR.

## Working through the issue backlog

scikit-build-core had accumulated **140+ open issues**; it is now at around 20.

- Claude found all the reproducible bug reports.
- Subagents produced fixes: 10 PRs, of which 6--7 were mergeable as-is, ~2
  needed extra work, and 1 was closed.
- A full
  [categorization of the backlog](https://github.com/orgs/scikit-build/discussions/1343)
  was then used to target the remaining work for the 1.0 release.

## Porting scikit-build (classic)

Classic scikit-build is now a thin layer on scikit-build-core's setuptools
plugin. The conversion spanned **three repositories**, including
[scikit-build-sample-projects](https://github.com/scikit-build/scikit-build-sample-projects),
with test changes driven by removals (mostly old `setup.py` commands). The
agent:

- made all the test adjustments and replacements, and tested the samples repo;
- found and fixed **seven bugs** in scikit-build-core's setuptools plugin along
  the way --- locating the local checkout of the other repo, applying the fixes
  there, and offering a PR.

Working across multiple repos at once --- something agents turn out to be
happy (proactive, even) to do --- is exactly the kind of coordination that
usually makes this sort of port drag on.

## Other wins

- The test suite got **40% faster** with the same coverage.
- Long-standing editable-install bugs were worked out.
- Bugs from as far back as six years ago --- the kind that never make it to the
  top of a maintainer's list --- finally got fixed.

## Takeaways

- **It scales.** Build a skill or a working procedure once, then launch many
  agents in parallel (downstream testing, backlog fixes).
- **Make it do what you wouldn't.** Sweeping reviews, 20-project regression
  sweeps, six-year-old bugs, three-repo ports --- the tedious wins are the
  biggest.
- **Verify like it's a contributor.** Regression tests before fixes, CI, and
  human review still apply; the near-100% merge rate comes from treating agent
  output like any other PR.
