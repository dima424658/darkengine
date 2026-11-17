#!/usr/bin/env python3
"""Check image identities, streaming uploads, and draws after buffer growth."""
import pathlib
import sys
from compare import read_ppm

paths = sorted(pathlib.Path(sys.argv[1]).glob('*.ppm'), key=lambda path: int(path.stem))
assert len(paths) == 13, f'Expected 13 stress captures, found {len(paths)}'
for path in paths:
    frame = int(path.stem)
    width, height, pixels = read_ppm(path)
    assert (width, height) == (1028, 512), (path, width, height)
    maximum = 0
    for y in range(height):
        for x in range(width):
            if frame == 0:
                stripe = min(2, ((x + 1) * 3 - 1) // width)
                expected = tuple(255 if channel == stripe else 0 for channel in range(3))
            else:
                i = frame - 1
                expected = ((i * 37 + 43) & 255, (i * 53 + 71) & 255, (i * 29 + 101) & 255)
            offset = (y * width + x) * 3
            maximum = max(maximum, max(abs(pixels[offset + channel] - expected[channel])
                                       for channel in range(3)))
    assert maximum <= 1, (path, maximum)
    print(f'{path.name}: maximum channel difference {maximum}')
print('Renderer stress captures passed')
