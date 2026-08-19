from PIL import Image
import numpy as np
import sys

with Image.open(sys.argv[1]) as image, open(sys.argv[2], "w") as file:
    data = np.frombuffer(image.convert("1").tobytes(), dtype=np.uint8);
    data = np.unpackbits(data).reshape(1024, 8)[:, ::-1];
    data = np.packbits(data);
    file.write(", ".join(f"0x{x:02x}" for x in data));
