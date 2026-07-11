#pragma once
#include <pybind11/pybind11.h>

#include <sstream>

#include <Minuit2/MnPrint.h>
#include <Minuit2/FunctionMinimum.h>

namespace py = pybind11;
using namespace ROOT::Minuit2;

void init_FunctionMinimum(py::module &m) {
    py::class_<FunctionMinimum>(m, "FunctionMinimum")
        .def("__str__", [](const FunctionMinimum &self) {
            std::stringstream os;
            os << self;
            return os.str();
        })
    ;
}
