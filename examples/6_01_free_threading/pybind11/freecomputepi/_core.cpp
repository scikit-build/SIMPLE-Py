#include <pybind11/pybind11.h>

#include <random>

namespace py = pybind11;

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

PYBIND11_MODULE(_core, m, py::mod_gil_not_used()) {
    m.def("pi", &pi, "Estimate pi with a Monte Carlo dart throw");
}
