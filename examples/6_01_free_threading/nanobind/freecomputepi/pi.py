import statistics
from concurrent.futures import ThreadPoolExecutor

from ._core import pi


def pi_in_threads(threads: int, trials: int) -> float:
    if threads == 0:
        return pi(trials)
    chunks = [trials // threads] * threads
    with ThreadPoolExecutor(max_workers=threads) as executor:
        return statistics.mean(executor.map(pi, chunks))
