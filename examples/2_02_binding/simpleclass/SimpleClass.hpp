#pragma once

class Simple {
    int x;

  public:
    Simple(int x) : x(x) {}

    int get() const { return x; }
};
