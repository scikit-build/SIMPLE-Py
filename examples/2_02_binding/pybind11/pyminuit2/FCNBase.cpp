#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // std::vector<double> <-> Python list conversion

#include <Minuit2/FCNBase.h>

namespace py = pybind11;
using namespace ROOT::Minuit2;

class PyFCNBase : public FCNBase {
   public:
     using FCNBase::FCNBase;

     double operator()(const std::vector<double> &v) const override {
         PYBIND11_OVERLOAD_PURE_NAME(
             double, FCNBase, "__call__", operator(), v);}

     double Up() const override {
         PYBIND11_OVERLOAD_PURE(double, FCNBase, Up, );}
 };
void init_FCNBase(py::module &m) {
    py::class_<FCNBase, PyFCNBase>(m, "FCNBase")
         .def(py::init<>())
         .def("__call__", &FCNBase::operator())
         .def("Up", &FCNBase::Up);
}
