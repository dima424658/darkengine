#pragma once

#include <stddef.h>

// Inflate a raw RFC 1951 stream into a caller-owned buffer. Returns the
// number of bytes produced, or -1 for malformed input or insufficient output.
ptrdiff_t RawDeflateExpand(const void* input, size_t inputSize,
	void* output, size_t outputSize);
