#define PY_SSIZE_T_CLEAN
#include <Python.h>

static PyObject *square(PyObject *self, PyObject *args) {
  double x;
  if (!PyArg_ParseTuple(args, "d", &x)) {
    return NULL;
  }
  return PyFloat_FromDouble(x * x);
}

static PyMethodDef methods[] = {
    {"square", square, METH_VARARGS, "Square a number"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef module = {PyModuleDef_HEAD_INIT, "_core", NULL, -1,
                                    methods};

PyMODINIT_FUNC PyInit__core(void) { return PyModule_Create(&module); }
