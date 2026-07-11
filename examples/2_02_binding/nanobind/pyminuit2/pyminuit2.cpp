#include <nanobind/nanobind.h>

namespace nb = nanobind;

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
