#pragma once

class Vector2D {
    double x;
    double y;

  public:
    Vector2D(double x, double y) : x(x), y(y) {}

    float get_x() const { return x; }
    float get_y() const { return y; }

    void set_x(float val) { x = val; }
    void set_y(float val) { y = val; }

    Vector2D &operator+=(const Vector2D &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2D operator+(const Vector2D &other) const {
        return Vector2D(x + other.x, y + other.y);
    }
};
