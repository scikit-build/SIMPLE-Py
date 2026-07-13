#include <pybind11/pybind11.h>

#include <Minuit2/MnMigrad.h>
#include <Minuit2/MnApplication.h>
#include <Minuit2/MnUserParameters.h>
#include <Minuit2/FCNBase.h>
#include <Minuit2/FunctionMinimum.h>

namespace py = pybind11;
using namespace pybind11::literals;
using namespace ROOT::Minuit2;

void init_MnMigrad(py::module &m) {
    py::class_<MnApplication>(m, "MnApplication")
        .def("__call__",
             &MnApplication::operator(),
             "Minimize the function, returns a function minimum",
             "maxfcn"_a = 0,
             "tolerance"_a = 0.1);

    py::class_<MnMigrad, MnApplication>(m, "MnMigrad")
        .def(py::init([](const FCNBase &fcn, const MnUserParameters &par, unsigned int stra) {
                 return MnMigrad(fcn, par, MnStrategy(stra));
             }),
             "fcn"_a, "par"_a, "stra"_a = 1)
    ;
}
