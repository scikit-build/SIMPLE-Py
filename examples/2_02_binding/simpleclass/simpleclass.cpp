#include <pybind11/pybind11.h>

#include "SimpleClass.hpp"

namespace py = pybind11;

PYBIND11_MODULE(simpleclass, m) {
    py::class_<Simple>(m, "Simple")
        .def(py::init<int>())
        .def("get", &Simple::get);
}
