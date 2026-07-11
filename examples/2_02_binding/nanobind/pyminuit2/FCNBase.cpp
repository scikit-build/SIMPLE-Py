#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>  // std::vector<double> <-> Python list conversion
#include <nanobind/trampoline.h>

#include <Minuit2/FCNBase.h>

namespace nb = nanobind;
using namespace ROOT::Minuit2;

class PyFCNBase : public FCNBase {
   public:
     NB_TRAMPOLINE(FCNBase, 2);

     double operator()(const std::vector<double> &v) const override {
         NB_OVERRIDE_PURE_NAME("__call__", operator(), v);}

     double Up() const override {
         NB_OVERRIDE_PURE(Up);}
 };
void init_FCNBase(nb::module_ &m) {
    nb::class_<FCNBase, PyFCNBase>(m, "FCNBase")
         .def(nb::init<>())
         .def("__call__", &FCNBase::operator())
         .def("Up", &FCNBase::Up);
}
