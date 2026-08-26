import sys

import numpy as np
from PIL import Image

from tools.generate_graphics import BYTE_DTYPE, BYTE_SIZE


def main():
    with Image.open(sys.argv[1]) as image, open(sys.argv[2], "w") as txt:
        pixels = np.asarray(image.convert("1"), dtype=BYTE_DTYPE)
        height, width = map(int, pixels.shape)

        assert width <= 255 or height <= 255, "image dimensions too large!"

        padded_width = (width + BYTE_SIZE - 1) // BYTE_SIZE * BYTE_SIZE

        if padded_width != width:
            pixels = np.pad(pixels, ((0, 0), (0, padded_width - width)))

        data = np.ravel(np.packbits(pixels, axis=1, bitorder="little"))
        _ = txt.write(f"0x{width:02x}, 0x{height:02x}, " + ", ".join(f"0x{x:02x}" for x in data))

        print(f"outputted txt from '{sys.argv[1]}' to '{sys.argv[2]}'");

if __name__ == "__main__":
    main()
