#pragma once

#include <loopapi.h>
#include <looptype.h>

#ifndef NO_DB_MEM
#include <memall.h>
#endif

#include <dlist.h>

struct sModeData
{
	sModeData() = default;
	sModeData(const sModeData& o) = default;
	sModeData(struct ILoopDispatch* dispatch, sLoopFrameInfo frame, uint flags)
		: dispatch{ dispatch }, frame{ frame }, flags{ flags } { }

	ILoopDispatch* dispatch;
	sLoopFrameInfo frame;
	uint flags;
};

class cLoopModeElem : public cDListNode <cLoopModeElem, 1>
{
public:
	cLoopModeElem(sModeData* data)
		: modedata{ *data } { }

	sModeData modedata;
};

class cLoopModeStack : public cDList<cLoopModeElem, 1>
{
public:
	cLoopModeStack();

	void Push(sModeData data);
	sModeData* Pop(sModeData* result);
	sModeData* Top();
};