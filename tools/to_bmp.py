from PIL import Image
import numpy as np
import sys

data = np.loadtxt(sys.argv[1], delimiter=",", dtype=np.uint8, converters=lambda x: int(x, 16));
data = np.unpackbits(data, bitorder="little").reshape(64, 128);

# https://pillow.readthedocs.io/en/stable/reference/Image.html#PIL.Image.fromarray
Image.fromarray(data * 255).convert("1").save(sys.argv[2]);

print(f"outputted image from '{sys.argv[1]}' to '{sys.argv[2]}'");
