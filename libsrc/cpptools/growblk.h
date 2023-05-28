#pragma once

//////////
//
// Utility: growing block
//

class cGrowingBlock {
public:
    cGrowingBlock();
    ~cGrowingBlock();

    void Append(char c);
    void Append(const char* s);

    int GetSize();

    char* GetBlock();

private:
    // The current size of the block, in increments:
    int m_numIncs;
    // The temporary holding cell for the strings:
    char* m_pTempBlock;
    // The next index to put a character into (aka the current length):
    int m_curp;
    // The current block size:
    int m_blocksize;
};