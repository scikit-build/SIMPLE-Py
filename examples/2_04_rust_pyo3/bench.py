"""Benchmark pyo3_example.count_primes against the same algorithm in pure Python."""

import timeit

import pyo3_example

LIMIT = 1_000_000


def count_primes(limit):
    """Count primes below `limit` by trial division (same algorithm as src/lib.rs)."""
    count = 0
    for n in range(2, limit):
        is_prime = True
        d = 2
        while d * d <= n:
            if n % d == 0:
                is_prime = False
                break
            d += 1
        if is_prime:
            count += 1
    return count


if __name__ == "__main__":
    assert count_primes(10_000) == pyo3_example.count_primes(10_000)

    print(f"count_primes({LIMIT:_}), best of 3 runs:")
    python_seconds = min(timeit.repeat(lambda: count_primes(LIMIT), number=1, repeat=3))
    rust_seconds = min(
        timeit.repeat(lambda: pyo3_example.count_primes(LIMIT), number=1, repeat=3)
    )
    print(f"  pure Python: {python_seconds:.3f} s")
    print(f"  Rust (PyO3): {rust_seconds:.3f} s")
    print(f"  speedup:     {python_seconds / rust_seconds:.0f}x")
