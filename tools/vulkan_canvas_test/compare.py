#!/usr/bin/env python3
"""Compare the CPU-reference and GPU-decoded halves of the 12 captured fixtures."""
import pathlib
import sys


def read_ppm(path):
    with path.open('rb') as stream:
        def token():
            value = bytearray()
            while True:
                byte = stream.read(1)
                if not byte:
                    raise ValueError('Incomplete PPM header')
                if byte == b'#' and not value:
                    stream.readline()
                elif byte.isspace():
                    if value:
                        return bytes(value)
                else:
                    value.extend(byte)
        assert token() == b'P6', path
        width, height = int(token()), int(token())
        assert token() == b'255', path
        pixels = stream.read()
    assert len(pixels) == width * height * 3, path
    return width, height, pixels


def main():
    paths = sorted(pathlib.Path(sys.argv[1]).glob('*.ppm'), key=lambda path: int(path.stem))
    assert len(paths) == 12, f'Expected 12 fixture captures, found {len(paths)}'
    for path in paths:
        width, height, pixels = read_ppm(path)
        assert (width, height) == (1028, 512), (path, width, height)
        half_row = width // 2 * 3
        maximum = 0
        for row in range(height):
            offset = row * width * 3
            left = pixels[offset:offset + half_row]
            right = pixels[offset + half_row:offset + half_row * 2]
            maximum = max(maximum, max(abs(a - b) for a, b in zip(left, right)))
        # Native linear filtering and explicit shader interpolation can round one
        # color unit differently. Nearest-neighbor fixtures must match exactly.
        linear = (int(path.stem) // 2) % 2 == 1
        assert maximum <= (1 if linear else 0), (path, maximum)
        print(f'{path.name}: maximum channel difference {maximum}')
    print('All GPU canvas comparisons passed')


if __name__ == "__main__":
    main()
