#include <pybind11/pybind11.h>

// Even -> n/2, odd -> 3n+1. Conjectured to always terminate.
int collatz_steps(long long n) {
  int steps = 0;
  while (n != 1) {
    n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    ++steps;
  }
  return steps;
}

PYBIND11_MODULE(collatz, m) {
  m.def("collatz_steps", &collatz_steps, "Steps to reach 1 in the Collatz sequence");
}
