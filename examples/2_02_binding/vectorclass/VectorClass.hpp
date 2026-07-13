#pragma once

class Vector2D {
    double x;
    double y;

  public:
    Vector2D(double x, double y) : x(x), y(y) {}

    double get_x() const { return x; }
    double get_y() const { return y; }

    void set_x(double val) { x = val; }
    void set_y(double val) { y = val; }

    Vector2D &operator+=(const Vector2D &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2D operator+(const Vector2D &other) const {
        return Vector2D(x + other.x, y + other.y);
    }
};
