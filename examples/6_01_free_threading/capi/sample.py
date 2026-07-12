import sys
import time

from freecomputepi.pi import pi_in_threads

TRIALS = 20_000_000

gil = sys._is_gil_enabled()
print(f"Python {sys.version.split()[0]}, GIL {'enabled' if gil else 'disabled'}")

for threads in [1, 2, 4, 8]:
    start = time.monotonic()
    result = pi_in_threads(threads, TRIALS)
    elapsed = time.monotonic() - start
    print(f"{threads:>2} threads: pi = {result:.5f}  ({elapsed:.2f} s)")
