import sys

import numpy as np

from rescale.core import rescale


def main() -> None:
    values = np.array([float(x) for x in sys.argv[1:]])
    print(rescale(values))


if __name__ == "__main__":
    main()
