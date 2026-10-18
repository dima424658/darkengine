#include <portableinflate.h>

#include <stdint.h>
#include <string.h>

namespace
{
class cBitReader
{
public:
	cBitReader(const void* data, size_t size)
		: m_next(static_cast<const uint8_t*>(data)), m_end(m_next + size),
		m_bits(0), m_bitCount(0), m_valid(true)
	{
	}

	unsigned Read(unsigned count)
	{
		while (m_bitCount < count)
		{
			if (m_next == m_end)
			{
				m_valid = false;
				return 0;
			}
			m_bits |= static_cast<uint64_t>(*m_next++) << m_bitCount;
			m_bitCount += 8;
		}

		const auto mask = count == 32 ? UINT32_MAX : ((uint32_t{ 1 } << count) - 1);
		const auto value = static_cast<unsigned>(m_bits) & mask;
		m_bits >>= count;
		m_bitCount -= count;
		return value;
	}

	void AlignToByte()
	{
		m_bits = 0;
		m_bitCount = 0;
	}

	bool IsValid() const { return m_valid; }

private:
	const uint8_t* m_next;
	const uint8_t* m_end;
	uint64_t m_bits;
	unsigned m_bitCount;
	bool m_valid;
};

struct sHuffman
{
	uint16_t count[16];
	uint16_t symbol[288];

	bool Build(const uint8_t* lengths, unsigned symbolCount)
	{
		memset(count, 0, sizeof(count));
		for (unsigned i = 0; i < symbolCount; ++i)
		{
			if (lengths[i] > 15)
				return false;
			++count[lengths[i]];
		}

		int codesLeft = 1;
		for (unsigned length = 1; length <= 15; ++length)
		{
			codesLeft = (codesLeft << 1) - count[length];
			if (codesLeft < 0)
				return false;
		}

		uint16_t offsets[16]{};
		for (unsigned length = 1; length < 15; ++length)
			offsets[length + 1] = offsets[length] + count[length];

		for (unsigned value = 0; value < symbolCount; ++value)
		{
			const auto length = lengths[value];
			if (length)
				symbol[offsets[length]++] = static_cast<uint16_t>(value);
		}
		return true;
	}

	int Decode(cBitReader& input) const
	{
		unsigned code = 0;
		unsigned first = 0;
		unsigned index = 0;
		for (unsigned length = 1; length <= 15; ++length)
		{
			code |= input.Read(1);
			if (!input.IsValid())
				return -1;

			const auto lengthCount = count[length];
			if (code >= first && code - first < lengthCount)
				return symbol[index + code - first];

			index += lengthCount;
			first = (first + lengthCount) << 1;
			code <<= 1;
		}
		return -1;
	}
};

bool BuildFixedTrees(sHuffman& literals, sHuffman& distances)
{
	uint8_t lengths[288];
	for (unsigned i = 0; i <= 143; ++i) lengths[i] = 8;
	for (unsigned i = 144; i <= 255; ++i) lengths[i] = 9;
	for (unsigned i = 256; i <= 279; ++i) lengths[i] = 7;
	for (unsigned i = 280; i <= 287; ++i) lengths[i] = 8;
	if (!literals.Build(lengths, 288))
		return false;

	memset(lengths, 5, 32);
	return distances.Build(lengths, 32);
}

bool BuildDynamicTrees(cBitReader& input, sHuffman& literals, sHuffman& distances)
{
	static constexpr uint8_t CodeLengthOrder[19] =
	{
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	const auto literalCount = input.Read(5) + 257;
	const auto distanceCount = input.Read(5) + 1;
	const auto codeLengthCount = input.Read(4) + 4;
	if (!input.IsValid() || literalCount > 286 || distanceCount > 32)
		return false;

	uint8_t codeLengths[19]{};
	for (unsigned i = 0; i < codeLengthCount; ++i)
		codeLengths[CodeLengthOrder[i]] = static_cast<uint8_t>(input.Read(3));
	if (!input.IsValid())
		return false;

	sHuffman codeLengthTree{};
	if (!codeLengthTree.Build(codeLengths, 19))
		return false;

	uint8_t lengths[288 + 32]{};
	const auto totalCount = literalCount + distanceCount;
	unsigned index = 0;
	while (index < totalCount)
	{
		const auto value = codeLengthTree.Decode(input);
		if (value < 0)
			return false;

		if (value <= 15)
		{
			lengths[index++] = static_cast<uint8_t>(value);
			continue;
		}

		unsigned repeat = 0;
		uint8_t repeatedLength = 0;
		if (value == 16)
		{
			if (!index)
				return false;
			repeat = input.Read(2) + 3;
			repeatedLength = lengths[index - 1];
		}
		else if (value == 17)
		{
			repeat = input.Read(3) + 3;
		}
		else if (value == 18)
		{
			repeat = input.Read(7) + 11;
		}
		else
		{
			return false;
		}

		if (!input.IsValid() || repeat > totalCount - index)
			return false;
		while (repeat--)
			lengths[index++] = repeatedLength;
	}

	if (!lengths[256])
		return false;
	return literals.Build(lengths, literalCount) &&
		distances.Build(lengths + literalCount, distanceCount);
}

bool ExpandCompressedBlock(cBitReader& input, const sHuffman& literals,
	const sHuffman& distances, uint8_t*& output, const uint8_t* outputStart,
	const uint8_t* outputEnd)
{
	static constexpr uint16_t LengthBase[29] =
	{
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
		31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
	};
	static constexpr uint8_t LengthExtra[29] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
	};
	static constexpr uint16_t DistanceBase[30] =
	{
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
		193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
		6145, 8193, 12289, 16385, 24577
	};
	static constexpr uint8_t DistanceExtra[30] =
	{
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
		6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
	};

	for (;;)
	{
		const auto symbol = literals.Decode(input);
		if (symbol < 0)
			return false;
		if (symbol < 256)
		{
			if (output == outputEnd)
				return false;
			*output++ = static_cast<uint8_t>(symbol);
			continue;
		}
		if (symbol == 256)
			return true;
		if (symbol > 285)
			return false;

		const auto lengthIndex = static_cast<unsigned>(symbol - 257);
		auto length = static_cast<size_t>(LengthBase[lengthIndex] +
			input.Read(LengthExtra[lengthIndex]));
		const auto distanceSymbol = distances.Decode(input);
		if (!input.IsValid() || distanceSymbol < 0 || distanceSymbol > 29)
			return false;
		auto distance = static_cast<size_t>(DistanceBase[distanceSymbol] +
			input.Read(DistanceExtra[distanceSymbol]));
		if (!input.IsValid() || distance > static_cast<size_t>(output - outputStart) ||
			length > static_cast<size_t>(outputEnd - output))
			return false;

		while (length--)
		{
			*output = output[-static_cast<ptrdiff_t>(distance)];
			++output;
		}
	}
}
}

ptrdiff_t RawDeflateExpand(const void* inputData, size_t inputSize,
	void* outputData, size_t outputSize)
{
	if ((!inputData && inputSize) || (!outputData && outputSize))
		return -1;

	cBitReader input(inputData, inputSize);
	auto* const outputStart = static_cast<uint8_t*>(outputData);
	auto* output = outputStart;
	auto* const outputEnd = outputStart + outputSize;
	bool finalBlock = false;

	while (!finalBlock)
	{
		finalBlock = input.Read(1) != 0;
		const auto blockType = input.Read(2);
		if (!input.IsValid() || blockType == 3)
			return -1;

		if (blockType == 0)
		{
			input.AlignToByte();
			const auto length = input.Read(16);
			const auto inverseLength = input.Read(16);
			if (!input.IsValid() || (length ^ 0xffffu) != inverseLength ||
				length > static_cast<size_t>(outputEnd - output))
				return -1;
			for (unsigned i = 0; i < length; ++i)
				*output++ = static_cast<uint8_t>(input.Read(8));
			if (!input.IsValid())
				return -1;
			continue;
		}

		sHuffman literals{};
		sHuffman distances{};
		const auto treesValid = blockType == 1
			? BuildFixedTrees(literals, distances)
			: BuildDynamicTrees(input, literals, distances);
		if (!treesValid || !ExpandCompressedBlock(input, literals, distances,
			output, outputStart, outputEnd))
			return -1;
	}

	return output - outputStart;
}
