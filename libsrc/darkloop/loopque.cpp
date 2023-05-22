#include "loopque.h"
#include <lgassert.h>

#include <algorithm>


cLoopQueue::cLoopQueue()
	: m_nRemovePoint{}, m_nInsertPoint{}, m_Messages{} { }

void cLoopQueue::Append(const sLoopQueueMessage& message)
{
	if (m_nInsertPoint >= std::size(m_Messages))
	{
		PackAppend(message);
	}
	else
	{
		m_Messages[m_nInsertPoint] = message;
		++m_nInsertPoint;
	}
}

int cLoopQueue::GetMessage(sLoopQueueMessage* pMessage)
{
	if (m_nRemovePoint == m_nInsertPoint)
		return 0;

	*pMessage = m_Messages[m_nRemovePoint];
	++m_nRemovePoint;
}

void cLoopQueue::PackAppend(const sLoopQueueMessage& message)
{
	if (m_nRemovePoint == 0)
	{
		CriticalMsg1("Loop queue overflow (size is %d)", std::size(m_Messages));
		return;
	}
   
	std::memmove(m_Messages, &m_Messages[m_nRemovePoint], sizeof(sLoopQueueMessage) * (m_nInsertPoint - m_nRemovePoint));
	m_nInsertPoint -= m_nRemovePoint;
	m_nRemovePoint = 0;

	m_Messages[m_nInsertPoint] = message;
	++m_nInsertPoint;
}