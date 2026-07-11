#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "VectorClass.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(vectorclass, m) {
    py::class_<Vector2D>(m, "Vector2D")
        .def(py::init<double, double>(), "x"_a, "y"_a)
        .def_property("x", &Vector2D::get_x, &Vector2D::set_x)
        .def_property("y", &Vector2D::get_y, &Vector2D::set_y)
        .def(py::self += py::self)
        .def(py::self + py::self)
        .def("__repr__", [](py::object self) {
            return py::str("{0.__class__.__name__}({0.x}, {0.y})").format(self);
        });
}
