#include <nanobind/nanobind.h>

#include <Minuit2/MnMigrad.h>
#include <Minuit2/MnApplication.h>
#include <Minuit2/MnUserParameters.h>
#include <Minuit2/FCNBase.h>
#include <Minuit2/FunctionMinimum.h>

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace ROOT::Minuit2;

void init_MnMigrad(nb::module_ &m) {
    nb::class_<MnApplication>(m, "MnApplication")
        .def("__call__",
             &MnApplication::operator(),
             "Minimize the function, returns a function minimum",
             "maxfcn"_a = 0,
             "tolerance"_a = 0.1);

    nb::class_<MnMigrad, MnApplication>(m, "MnMigrad")
        .def("__init__", [](MnMigrad *self, const FCNBase &fcn, const MnUserParameters &par, unsigned int stra) {
                 new (self) MnMigrad(fcn, par, MnStrategy(stra));
             },
             "fcn"_a, "par"_a, "stra"_a = 1)
    ;
}
