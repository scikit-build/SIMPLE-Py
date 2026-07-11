import random
import statistics
from concurrent.futures import ThreadPoolExecutor


def pi(trials: int) -> float:
    ran = random.Random()
    inside = 0
    for _ in range(trials):
        x = ran.uniform(-1, 1)
        y = ran.uniform(-1, 1)
        if x * x + y * y <= 1:
            inside += 1
    return 4.0 * inside / trials


def pi_in_threads(threads: int, trials: int) -> float:
    if threads == 0:
        return pi(trials)
    chunks = [trials // threads] * threads
    with ThreadPoolExecutor(max_workers=threads) as executor:
        return statistics.mean(executor.map(pi, chunks))
