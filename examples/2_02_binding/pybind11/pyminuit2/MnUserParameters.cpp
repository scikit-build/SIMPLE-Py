#include <pybind11/pybind11.h>

#include <Minuit2/MnUserParameters.h>

namespace py = pybind11;
using namespace ROOT::Minuit2;

void init_MnUserParameters(py::module &m) {
    py::class_<MnUserParameters>(m, "MnUserParameters")
        .def(py::init<>())
        .def("Add", py::overload_cast<const std::string &, double>(&MnUserParameters::Add))
        .def("Add", py::overload_cast<const std::string &, double, double>(&MnUserParameters::Add))
    ;
}
