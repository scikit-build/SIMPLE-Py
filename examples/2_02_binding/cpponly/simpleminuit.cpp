#include "SimpleFCN.hpp"

int main() {
    SimpleFCN fcn;
    MnUserParameters upar;
    upar.Add("x", 1., 0.1);
    MnMigrad migrad(fcn, upar);
    FunctionMinimum min = migrad();
    std::cout << min << std::endl;
}
