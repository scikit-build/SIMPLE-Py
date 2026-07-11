# Binding tools

Let's take a deeper look at binding tools. We'll focus on pybind11 and nanobind. These are:

* No dependencies required
* No pre-process step
* Not a custom language -- just (advanced) C++
* Easy to get started with
* Built up one piece at a time
* Great CMake support

:::{note}

Other choices include:

* **Cython**: Custom language, preprocessor -- better for making fast Python than binding
  * See `cython-cmake` for CMake support
* **F2Py**: Fortran, preprocessor
  * See `f2py-cmake` for CMake support
* **SWIG**: Preprocessor, all-at-once wrapping
  * CMake built-in helper

:::

## Intro to pybind11

Before we tackle a real library, let's see the binding patterns on a couple of
toy classes.

pybind11 is similar to Boost::Python, but much easier to build: it's pure C++11
with no dependencies, no preprocessing step, and no new language to learn. It's
used in projects like SciPy, PyTorch, boost-histogram, and GooFit (including for
CUDA). The main downside is that it's a little verbose -- but that verbosity buys
you a highly customizable interface.

### A simple class

Here's a minimal C++ class:

```{literalinclude} ../../examples/2_02_binding/simpleclass/SimpleClass.hpp
:language: cpp
```

Binding it takes just a few lines:

```{literalinclude} ../../examples/2_02_binding/simpleclass/simpleclass.cpp
:language: cpp
```

The `PYBIND11_MODULE` macro defines the module -- its name must match the
compiled file. `py::class_<Simple>` exposes the class, `py::init<int>()` binds
the constructor, and each `.def(...)` binds a method.

### More binding situations

A slightly richer class shows off more features -- properties and operators:

```{literalinclude} ../../examples/2_02_binding/vectorclass/VectorClass.hpp
:language: cpp
```

And the binding code:

```{literalinclude} ../../examples/2_02_binding/vectorclass/vectorclass.cpp
:language: cpp
```

A few new points:

* Constructors are easy, even when overloaded: just use `py::init<...>()`.
* You can name arguments with `"..."_a` and `using namespace pybind11::literals`.
* `def_property` takes a getter and a setter.
* The `pybind11/operators.h` header lets you bind operators with `py::self`.
* You can bind a lambda instead of a real method -- handy for `__repr__`.
* pybind11 provides Python types like `py::str` (with methods like `.format`),
  and `.attr` reaches any Python attribute.

:::{note}

pybind11 has excellent documentation and handles a lot of situations, including
smart pointers, `std::variant`, NumPy (without needing NumPy at compile time!),
and Eigen. It focuses on making _small_ extensions -- there's a little call
overhead per function to handle overloads, so if faster means bigger, it may not
be the right fit.
:::

We are going to try a non-trivial project! Let's wrap Minuit2.

## Intro to Minuit2

Before we see it in Python, let's see what Minuit2 is.

You should know what the C++ looks like, and know what you want the Python to look like. For now, let's replicate the C++ experience.

For example: a simple minimizer for $f(x) = x^2$ (should quickly find 0 as minimum), the procedure should be:

* Define FCN
* Setup parameters
* Minimize
* Print result

### Define the FCN

We define the function to minimize by subclassing `FCNBase`:

```{literalinclude} ../../examples/2_02_binding/cpponly/SimpleFCN.hpp
:language: cpp
```

### Run the minimizer

Then we set up the parameters, minimize, and print the result:

```{literalinclude} ../../examples/2_02_binding/cpponly/simpleminuit.cpp
:language: cpp
```

### Build configuration

We use CMake with `FetchContent` to grab Minuit2:

```{literalinclude} ../../examples/2_02_binding/cpponly/CMakeLists.txt
:language: cmake
```

### Build and run

To build it:

```bash
cmake -S . -B build
cmake --build build
```

And run:

```bash
./build/simpleminuit
```

You should see something like this:

```text
val = 1
val = 1.001
val = 0.999
val = 1.0006
val = 0.999402
val = -8.23008e-11
val = 0.000345267
val = -0.000345267
val = -8.23008e-11
val = 0.000345267
val = -0.000345267
val = 6.90533e-05
val = -6.90535e-05

  Valid         : yes
  Function calls: 13
  Minimum value : 6.773427082e-21
  Edm           : 6.773427082e-21
  Internal parameters: 	[ -8.230083282e-11]
  Internal covariance matrix:
[[              1]]]
  External parameters:
  Pos |    Name    |  type   |      Value       |    Error +/-
    0 |          x |  free   | -8.230083282e-11 | 0.7071067812
```

:::{note}

We are getting Minuit2 from the GooFit hosted standalone copy. It's an exact
standalone output of the contents of [ROOT](https://root.cern), at
`root-project/root` in the `math/minuit2` directory. You can also include it
from there, but the repo is _much_ bigger, so it's just a bit slower.

You can use either repo as a submodule and `add_subdirectory(...)` instead if
you prefer.
:::

## Binding Minuit2

The great thing about pybind11 is that we can just bind the parts we need.
There's a lot more to Minuit2, but we don't care. If we used an auto-binding
tool (like SWIG), we'd have to work out issues for all the parts we aren't
using first.

### The main module

Pybind11 programs are best split into a main module, which allows you
to build the parts separately, with minimal header overlap, and then link it all
at the end.

```{literalinclude} ../../examples/2_02_binding/pybind11/pyminuit2/pyminuit2.cpp
:language: cpp
```

Each `init_*` function fills in one piece of the module. We forward-declare them
here and call them in `PYBIND11_MODULE`; the definitions live in their own files.

### Binding the FCN

The FCN is an abstract base class in C++. To let Python subclass it, we use a
"trampoline" class that routes the virtual calls back into Python:

```{literalinclude} ../../examples/2_02_binding/pybind11/pyminuit2/FCNBase.cpp
:language: cpp
```

`PYBIND11_OVERLOAD_PURE_NAME` maps C++'s `operator()` to Python's `__call__`,
and `#include <pybind11/stl.h>` gives us automatic `std::vector<double>` ↔ list
conversion.

### Binding the parameters

`MnUserParameters` holds the parameters to minimize. We bind the constructor and
the two `Add` overloads (fixed and with an error) using `py::overload_cast`:

```{literalinclude} ../../examples/2_02_binding/pybind11/pyminuit2/MnUserParameters.cpp
:language: cpp
```

### Binding the minimizer

`MnMigrad` runs the minimization. It inherits from `MnApplication`, so we bind
both and tell pybind11 about the relationship. We use a lambda for the
constructor so we can take a plain `unsigned int` strategy instead of an
`MnStrategy` object, and the same `_a` literals for named arguments we saw with
`Vector2D`, now with defaults:

```{literalinclude} ../../examples/2_02_binding/pybind11/pyminuit2/MnApplication.cpp
:language: cpp
```

### Binding the result

Finally, `FunctionMinimum` is the result. We only need to print it, so we bind
`__str__` to stream the C++ object into a string:

```{literalinclude} ../../examples/2_02_binding/pybind11/pyminuit2/FunctionMinimum.cpp
:language: cpp
```

### Build configuration

The CMake is much like before, but we use `pybind11_add_module` instead of a
plain executable, glob the source files together, and `install` the resulting
module:

```{literalinclude} ../../examples/2_02_binding/pybind11/CMakeLists.txt
:language: cmake
```

We drive the build with scikit-build-core, so we need a `pyproject.toml`:

```{literalinclude} ../../examples/2_02_binding/pybind11/pyproject.toml
:language: toml
```

### Build and run

Install and run it in one step with uv:

```bash
uv run sample.py
```

Our sample script mirrors the C++ program, but in Python:

```{literalinclude} ../../examples/2_02_binding/pybind11/sample.py
:language: python
```

You should see the same minimization output as the C++ version, ending with the
printed `FunctionMinimum`.
