---
marp: true
theme: simplepy
paginate: true
_paginate: skip
---

# SIMPLE-Py

## Free-threading

---

## The GIL

For most of Python's history, one lock — the **Global Interpreter Lock** — has
let only **one thread run Python at a time**.

- Great for overlapping **I/O**
- Useless for **CPU-bound** work across cores
- The workaround was **multiprocessing**: pickling, process startup, no shared memory

Threads existed, but they couldn't make your computation faster.

---

## Free-threading arrives

[PEP 703](https://peps.python.org/pep-0703/) makes the GIL **optional**.

- **3.13** — experimental debut
- **3.14** — it got *fast* (what we require here)
- A separate build, tagged with a `t`: **`python3.14t`**

```pycon
>>> import sys
>>> sys._is_gil_enabled()
False
```

With the GIL off, threads run Python in **parallel on every core**.

---

## An embarrassingly parallel example

Estimate $\pi$ by throwing darts into $[-1, 1]^2$ and counting hits inside the
unit circle — the fraction approaches $\pi/4$.

Each dart is **independent**, so we run a batch per thread and average.

```python
def pi_in_threads(threads: int, trials: int) -> float:
    chunks = [trials // threads] * threads
    with ThreadPoolExecutor(max_workers=threads) as executor:
        return statistics.mean(executor.map(pi, chunks))
```

The same runner drives every version below — only `pi` changes.

---

## Pure Python

<div class="columns">
<div>

Just a loop over `random`:

```python
def pi(trials):
    ran = random.Random()
    inside = 0
    for _ in range(trials):
        x = ran.uniform(-1, 1)
        y = ran.uniform(-1, 1)
        if x * x + y * y <= 1:
            inside += 1
    return 4.0 * inside / trials
```

</div>
<div>

On `python3.14t`:

```text
 1 threads: 2.22 s
 2 threads: 1.13 s
 4 threads: 0.60 s
 8 threads: 0.43 s
```

Drop the `t` and the times stay **flat** — that's the GIL serializing threads.

</div>
</div>

---

## The catch: extensions must opt in

An extension has to **declare it doesn't need the GIL**.

Import *any* extension that hasn't opted in, and CPython silently switches the
GIL back **on** (with a warning) to keep that code safe.

> Every extension in the process must be free-threading-aware,
> or **nobody** gets the speedup.

The compute moves to C++ — the opt-in is the interesting one line.

---

## Compiled: opting in with a binding tool

<div class="columns">
<div>

### pybind11

In the module macro:

```cpp
PYBIND11_MODULE(_core, m,
                py::mod_gil_not_used()) {
    m.def("pi", &pi);
}
```

</div>
<div>

### nanobind

In CMake — code unchanged:

```cmake
nanobind_add_module(
    _core FREE_THREADED
    freecomputepi/_core.cpp)
```

</div>
</div>

Same Monte Carlo loop, same near-linear scaling — **an order of magnitude
faster per thread** (`0.26 s → 0.07 s`, 1→8 threads).

---

## Opting in with the raw C API (3.15)

No wrapper writes anything for you. In Python 3.15, the module export is a
**slot array** returned from a hook ([PEP 793](https://peps.python.org/pep-0793/)):

```cpp
static PySlot slots[] = {
    PySlot_STATIC_DATA(Py_mod_name, (void *)"_core"),
    PySlot_STATIC_DATA(Py_mod_methods, methods),
    PySlot_DATA(Py_mod_abi, &abi_info),
    PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),  // ← the opt-in
    PySlot_END,
};

PyMODEXPORT_FUNC PyModExport__core(void) { return slots; }
```

Bonus: 3.15's **stable ABI** covers free-threaded builds
([PEP 803](https://peps.python.org/pep-0803/)) — one `_core.abi3t.so` keeps
working on future `t` Pythons.

---

## A promise, not a shield

Declaring the module GIL-free tells CPython **"I have no unguarded shared
state."**

- Our `pi` uses only **local variables** → safe
- Global caches or shared buffers → need real locking first
  - `std::mutex`, atomics, or nanobind's `nb::ft_mutex`

Every opt-in above — macro, CMake flag, or slot — makes the **same promise**.

---

## Building wheels

Free-threaded wheels get their own ABI tag: **`cp314t`**, separate from `cp314`.

As of 3.14 it's **no longer experimental** — no `enable` needed:

```toml
[tool.cibuildwheel]
build = "cp314*"
```

`cp314*` matches both `cp314` and `cp314t`, so each job emits **both** wheels.
Free-threaded users automatically get the `t` wheel.

---

## Summary

- The **GIL** let only one thread run Python; free-threading (**PEP 703**) makes
  it optional — fast in **3.14t**
- **Pure Python** threads finally scale across cores without the GIL
- **Compiled** extensions must **opt in**, or importing them turns the GIL back on
  - pybind11: `py::mod_gil_not_used()` · nanobind: `FREE_THREADED` · C API:
    `Py_mod_gil` slot (3.15 export, stable-ABI `abi3t` capable)
- The opt-in is a **promise** — guard any shared state before making it
- Ship it with cibuildwheel's **`cp314*`** for both regular and `cp314t` wheels
