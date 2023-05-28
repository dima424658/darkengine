#include <growblk.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

// Tunable parameter: this is the increment we use for allocating the
// string block. Ideally, it should be just a bit larger than we actually
// want the final table to be. But it's only semi-relevant, since this is
// only being used for temporary memory anyway...
#define TAG_STRING_BLOCK_INCREMENT (1024)

cGrowingBlock::cGrowingBlock()
	: m_numIncs{ 0 },
	m_pTempBlock{ nullptr },
	m_curp{ 0 },
	m_blocksize{ 0 }
{
	m_pTempBlock = static_cast<char*>(malloc(TAG_STRING_BLOCK_INCREMENT));
	m_blocksize = TAG_STRING_BLOCK_INCREMENT;
	m_numIncs = 1;
}

cGrowingBlock::~cGrowingBlock()
{
	free(m_pTempBlock);
}

void cGrowingBlock::Append(char c)
{
	if (m_curp >= m_blocksize)
	{
		// Grow the block
		m_blocksize += TAG_STRING_BLOCK_INCREMENT;
		char* pNewBlock = static_cast<char*>(malloc(m_blocksize));
		memcpy(pNewBlock, m_pTempBlock, m_curp);
		free(m_pTempBlock);
		m_pTempBlock = pNewBlock;
	}

	m_pTempBlock[m_curp++] = c;
}

// @TBD: this could be more efficient...
void cGrowingBlock::Append(const char* s)
{
	while (*s != '\0')
		Append(*s++);
}

int cGrowingBlock::GetSize()
{
	return m_curp;
}

char* cGrowingBlock::GetBlock()
{
	return m_pTempBlock;
}