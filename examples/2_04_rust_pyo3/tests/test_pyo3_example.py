"""Tests for the pyo3_example extension module built with PyO3 and maturin."""

import pytest

import pyo3_example


def test_sum_as_string():
    assert pyo3_example.sum_as_string(2, 40) == "42"


def test_count_primes():
    assert pyo3_example.count_primes(10) == 4
    assert pyo3_example.count_primes(100) == 25


def test_count_primes_edge_cases():
    assert pyo3_example.count_primes(0) == 0
    assert pyo3_example.count_primes(2) == 0


def test_point_construction_and_attributes():
    point = pyo3_example.Point(1.0, 2.0)
    assert point.x == 1.0
    assert point.y == 2.0


def test_point_magnitude():
    point = pyo3_example.Point(3.0, 4.0)
    assert point.magnitude() == 5.0


def test_point_repr():
    point = pyo3_example.Point(1.0, 2.0)
    assert repr(point) == "Point(x=1.0, y=2.0)"


def test_checked_div():
    assert pyo3_example.checked_div(1.0, 2.0) == 0.5


def test_checked_div_by_zero():
    with pytest.raises(ZeroDivisionError):
        pyo3_example.checked_div(1.0, 0.0)
