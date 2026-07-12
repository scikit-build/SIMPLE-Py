#include <Python.h>

#include <random>

// Monte Carlo estimate of pi. The loop touches no Python objects, so nothing is
// shared between threads -- it scales cleanly once the GIL is out of the way.
double pi(int trials) {
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_real_distribution<double> dist(-1, 1);

    int inside = 0;
    for (int i = 0; i < trials; ++i) {
        double x = dist(engine);
        double y = dist(engine);
        if (x * x + y * y <= 1.0) {
            ++inside;
        }
    }
    return 4.0 * inside / trials;
}

// Everything below is the binding boilerplate pybind11/nanobind write for us:
// argument conversion, the method table, and the module export.
static PyObject *pi_py(PyObject *, PyObject *arg) {
    long trials = PyLong_AsLong(arg);
    if (trials == -1 && PyErr_Occurred()) {
        return nullptr;
    }
    return PyFloat_FromDouble(pi(static_cast<int>(trials)));
}

static PyMethodDef methods[] = {
    {"pi", pi_py, METH_O, "Estimate pi with a Monte Carlo dart throw"},
    {},
};

// The 3.15+ stable ABI covers free-threaded builds (abi3t, PEP 803), but
// hides PyModuleDef: instead, a PyModExport_<name> hook returns a slot array
// (PEP 793). Py_mod_gil is the free-threading opt-in.
PyABIInfo_VAR(abi_info);

static PySlot slots[] = {
    PySlot_STATIC_DATA(Py_mod_name, (void *)"_core"),
    PySlot_STATIC_DATA(Py_mod_methods, methods),
    PySlot_DATA(Py_mod_abi, &abi_info),
    PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
    PySlot_END,
};

PyMODEXPORT_FUNC PyModExport__core(void) { return slots; }
