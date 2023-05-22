#pragma once

#include <looptype.h>

struct sLoopQueueMessage
{
	int message;
	tLoopMessageData hData;
	int flags;
	int alignmentPadTo16Bytes;
};

static_assert(sizeof(sLoopQueueMessage) == 16);

class cLoopQueue
{
public:
	enum { kQueueSize = 16 };

	cLoopQueue();

	void Append(const sLoopQueueMessage& message);
	int GetMessage(sLoopQueueMessage* pMessage);
	void PackAppend(const sLoopQueueMessage& message);

private:
	unsigned int m_nRemovePoint;
	unsigned int m_nInsertPoint;
	sLoopQueueMessage m_Messages[kQueueSize];
};

