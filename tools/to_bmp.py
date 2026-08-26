import sys

import numpy as np
from PIL import Image

from tools.generate_graphics import BYTE_DTYPE, BYTE_SIZE


def main():
    data = np.loadtxt(sys.argv[1], delimiter=",", dtype=BYTE_DTYPE, converters=lambda x: int(x, 16))
    width, height = map(int, data[:2])
    data = data[2:]

    padded_width = (width + BYTE_SIZE - 1) // BYTE_SIZE * BYTE_SIZE
    expected_bytes = height * padded_width // BYTE_SIZE

    assert len(data) == expected_bytes, f"image length mismatch, expected {expected_bytes} but got {len(data)}!"

    pixels = np.unpackbits(data, bitorder="little").reshape(height, padded_width)
    pixels = pixels[:, :width]

    # https://pillow.readthedocs.io/en/stable/reference/Image.html#PIL.Image.fromarray
    Image.fromarray(pixels * 255).convert("1").save(sys.argv[2]);

    print(f"outputted image from '{sys.argv[1]}' to '{sys.argv[2]}'");

if __name__ == "__main__":
    main()
