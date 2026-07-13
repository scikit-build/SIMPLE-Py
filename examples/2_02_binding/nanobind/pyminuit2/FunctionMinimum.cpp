#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>  // std::string return value -> Python str

#include <sstream>

#include <Minuit2/MnPrint.h>
#include <Minuit2/FunctionMinimum.h>

namespace nb = nanobind;
using namespace ROOT::Minuit2;

void init_FunctionMinimum(nb::module_ &m) {
    nb::class_<FunctionMinimum>(m, "FunctionMinimum")
        .def("__str__", [](const FunctionMinimum &self) {
            std::stringstream os;
            os << self;
            return os.str();
        })
    ;
}
