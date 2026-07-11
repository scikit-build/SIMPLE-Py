# PyO3 example: a Rust extension module built with pixi

A minimal Rust extension module for Python, built with [PyO3](https://pyo3.rs) and [maturin](https://www.maturin.rs). Every tool it needs - the Rust compiler, maturin, Python, and pytest - is installed from conda-forge by [pixi](https://pixi.sh), so no rustup or system Rust is required. The module (`src/lib.rs`) exposes a function that releases the GIL (`count_primes`), a class (`Point`), and a function that raises a Python exception (`checked_div`); `bench.py` times the Rust `count_primes` against the identical trial-division algorithm in pure Python.

## Commands

```sh
pixi install    # solve and install the toolchain from conda-forge
pixi run test   # build the extension (debug) and run the pytest suite
pixi run bench  # build the extension (release) and run the benchmark
```

## Benchmark

Output of `pixi run bench` on an AMD Ryzen AI 9 HX 370 (Linux, Python 3.14, rustc 1.97):

```text
count_primes(1_000_000), best of 3 runs:
  pure Python: 1.601 s
  Rust (PyO3): 0.084 s
  speedup:     19x
```

**Benchmark release builds only.** `maturin develop` produces an unoptimized debug build - often many times slower than `--release`, sometimes slow enough to lose to pure Python. The `bench` task therefore depends on `develop-release` (`maturin develop --release`), so it always measures the optimized build.
