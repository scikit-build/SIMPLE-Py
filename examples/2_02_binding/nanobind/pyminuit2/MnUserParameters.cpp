#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>  // std::string <-> Python str conversion

#include <Minuit2/MnUserParameters.h>

namespace nb = nanobind;
using namespace ROOT::Minuit2;

void init_MnUserParameters(nb::module_ &m) {
    nb::class_<MnUserParameters>(m, "MnUserParameters")
        .def(nb::init<>())
        .def("Add", nb::overload_cast<const std::string &, double>(&MnUserParameters::Add))
        .def("Add", nb::overload_cast<const std::string &, double, double>(&MnUserParameters::Add))
    ;
}
