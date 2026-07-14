import numpy as np


def rescale(input_array):
    """Rescale an array so its values span [0, 1]."""
    low = np.min(input_array)
    high = np.max(input_array)
    return (input_array - low) / (high - low)
