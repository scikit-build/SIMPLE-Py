---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# Binding tools

## pybind11 & nanobind, up close

---

## Why pybind11 / nanobind?

Both are header-only C++ binding libraries — you write real (advanced) C++, not a
new language:

- No dependencies, no pre-process step
- Easy to start; built up one `.def(...)` at a time
- Great CMake support

Other tools solve different problems:

- **Cython** — custom language + preprocessor; better for *writing* fast Python
- **F2Py** — Fortran wrapper (`f2py-cmake`)
- **SWIG** — preprocessor, wraps everything at once

---

## pybind11: a simple class

Start with a minimal C++ class, then bind it:

<div class="columns">
<div>

```c++
class Simple {
  int x;

public:
  Simple(int x) : x(x) {}
  int get() const { return x; }
};
```

</div>
<div>

```c++
#include <pybind11/pybind11.h>
#include "SimpleClass.hpp"
namespace py = pybind11;

PYBIND11_MODULE(simpleclass, m) {
  py::class_<Simple>(m, "Simple")
    .def(py::init<int>())
    .def("get", &Simple::get);
}
```

</div>
</div>

`py::class_<Simple>` exposes the class, `py::init<int>()` binds the constructor,
each `.def(...)` binds a method. The module name **must** match the compiled file.

---

## pybind11: properties & operators

A richer `Vector2D` shows off more of the API:

```c++
py::class_<Vector2D>(m, "Vector2D")
    .def(py::init<double, double>(), "x"_a, "y"_a)       // named args
    .def_property("x", &Vector2D::get_x, &Vector2D::set_x)  // getter + setter
    .def(py::self += py::self)                           // <pybind11/operators.h>
    .def(py::self + py::self)
    .def("__repr__", [](py::object self) {               // bind a lambda
        return py::str("{0.__class__.__name__}({0.x}, {0.y})").format(self);
    });
```

- `"..."_a` names arguments (`using namespace pybind11::literals`)
- `py::self` binds operators; a lambda handles `__repr__`
- `py::str`/`.attr` reach real Python types and attributes

---

## nanobind: same design, tighter

[nanobind](https://nanobind.readthedocs.io) — newer, same author. It expects code
to conform to *it* rather than supporting all of C++, and in return compiles
faster, makes **much smaller** binaries, and has lower call overhead.

The API is deliberately close — mostly `nb::` for `py::`, `NB_MODULE` for
`PYBIND11_MODULE`. Differences you'll hit wrapping Minuit2:

- STL casters are **opt-in per type** (`<nanobind/stl/vector.h>`) vs one
  `<pybind11/stl.h>`
- Virtual overrides use `NB_TRAMPOLINE` + `NB_OVERRIDE_*`
- Factory constructors bind `__init__` with a placement `new`

Next: a real project — wrap Minuit2, both ways side by side.

---

## Minuit2: the C++ we're wrapping

Minuit2 is a function minimizer. Subclass `FCNBase`, set params, minimize, print:

```c++
class SimpleFCN : public FCNBase {
  double Up() const override { return 0.5; }
  double operator()(const std::vector<double> &v) const override {
    return v.at(0) * v.at(0);   // minimize x^2  ->  finds 0
  }
};
```

We only bind the handful of classes we actually use. With an auto-binding tool
(SWIG) we'd have to fix up *everything* first — here we don't care about the rest.

---

## The main module

Split the module into pieces — each `init_*` fills in one part, forward-declared
here and defined in its own file:

<div class="columns">
<div>

**pybind11**

```c++
void init_FCNBase(py::module &);
void init_MnUserParameters(py::module &);
void init_MnMigrad(py::module &);
void init_FunctionMinimum(py::module &);

PYBIND11_MODULE(minuit2, m) {
  init_FCNBase(m);
  init_MnUserParameters(m);
  init_MnMigrad(m);
  init_FunctionMinimum(m);
}
```

</div>
<div>

**nanobind**

```c++
void init_FCNBase(nb::module_ &);
void init_MnUserParameters(nb::module_ &);
void init_MnMigrad(nb::module_ &);
void init_FunctionMinimum(nb::module_ &);

NB_MODULE(minuit2, m) {
  init_FCNBase(m);
  init_MnUserParameters(m);
  init_MnMigrad(m);
  init_FunctionMinimum(m);
}
```

</div>
</div>

Building the pieces separately keeps header overlap minimal, then it all links.

---

## Binding the FCN (trampoline)

`FCNBase` is abstract — a **trampoline** routes virtual calls back into Python:

<div class="columns">
<div>

**pybind11**

```c++
class PyFCNBase : public FCNBase {
public:
  using FCNBase::FCNBase;
  double operator()(
      const std::vector<double> &v) const override {
    PYBIND11_OVERLOAD_PURE_NAME(
      double, FCNBase, "__call__", operator(), v);
  }
};
```

</div>
<div>

**nanobind**

```c++
class PyFCNBase : public FCNBase {
public:
  NB_TRAMPOLINE(FCNBase, 2);
  double operator()(
      const std::vector<double> &v) const override {
    NB_OVERRIDE_PURE_NAME(
      "__call__", operator(), v);
  }
};
```

</div>
</div>

`*_OVERLOAD/OVERRIDE_PURE_NAME` maps C++'s `operator()` to Python's `__call__`.
The STL header (`<pybind11/stl.h>` / `<nanobind/stl/vector.h>`) gives
`std::vector<double>` ↔ list for free.

---

## Parameters & minimizer

- **Overloads:** `py::overload_cast` / `nb::overload_cast` picks the right `Add`
- **Inheritance:** declare it — `py::class_<MnMigrad, MnApplication>`
- **Factory ctor:** a lambda takes a plain `unsigned int` strategy

<div class="columns">
<div>

**pybind11** wraps the lambda:

```c++
.def(py::init([](const FCNBase &fcn,
       const MnUserParameters &par,
       unsigned int stra) {
    return MnMigrad(fcn, par, MnStrategy(stra));
  }), "fcn"_a, "par"_a, "stra"_a = 1);
```

</div>
<div>

**nanobind** placement-`new`s into `__init__`:

```c++
.def("__init__", [](MnMigrad *self,
       const FCNBase &fcn,
       const MnUserParameters &par,
       unsigned int stra) {
    new (self) MnMigrad(fcn, par, MnStrategy(stra));
  }, "fcn"_a, "par"_a, "stra"_a = 1);
```

</div>
</div>

`_a` literals give named arguments with defaults, just like `Vector2D`.

---

## Build it: CMake + scikit-build-core

`pybind11_add_module` / `nanobind_add_module` build and `install` the module;
scikit-build-core drives it from `pyproject.toml`:

<div class="columns">
<div>

```toml
[build-system]
requires = [
  "scikit-build-core",
  "pybind11",
]
build-backend = "scikit_build_core.build"
```

</div>
<div>

```toml
[build-system]
requires = [
  "scikit-build-core",
  "nanobind",
]
build-backend = "scikit_build_core.build"
```

nanobind also wants an explicit
`find_package(Python ...)` in CMake.

</div>
</div>

The result of `FunctionMinimum` just needs `__str__` — stream the C++ object into
a string (nanobind returns it via `<nanobind/stl/string.h>`).

---

## One Python API, either backend

Install and run in one step — the sample mirrors the C++ program:

```python
import minuit2


class SimpleFCN(minuit2.FCNBase):
    def Up(self):
        return 0.5

    def __call__(self, v):
        return v[0] ** 2


upar = minuit2.MnUserParameters()
upar.Add("x", 1.0, 0.1)
minimum = minuit2.MnMigrad(SimpleFCN(), upar)()
print(minimum)  # same output as the C++ version
```

```bash
uv run sample.py
```

---

## Summary

- **pybind11 / nanobind** = header-only C++ binding, no new language, grown one
  `.def` at a time
- Bind **only what you use** — no need to wrap the whole library
- **nanobind** mirrors pybind11 but compiles faster and ships smaller binaries;
  STL casters are opt-in, trampolines and factory ctors differ slightly
- Real bindings are **split into pieces**, use **trampolines** for virtuals, and
  handle overloads with `overload_cast`
- **scikit-build-core + CMake** package it; the **Python API is identical** either
  way
