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
	BOOL GetMessage(sLoopQueueMessage* pMessage);
	void PackAppend(const sLoopQueueMessage& message);

private:
	ulong m_nRemovePoint;
	ulong m_nInsertPoint;
	sLoopQueueMessage m_Messages[kQueueSize];
};