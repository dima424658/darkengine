/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: r:/t2repos/thief2/src/sim/avatar.cpp,v 1.21 1999/08/06 12:25:06 Justin Exp $
//
// Simple avatar implementation
//

#include <lg.h>
#include <comtools.h>

#include <objtype.h>

#include <tagfile.h>
#include <objremap.h> // for ObjRemapOnLoad()

#include <iavatar.h>
#include <avatar.h>

// This should be last:
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cAvatar, IAvatar);

cAvatar::cAvatar(ObjID player, 
                 ulong playerNum)
   : m_player(player), m_playerNum(playerNum)
{
}

cAvatar::cAvatar(ITagFile *file)
{
   file->Read((char *) &m_playerNum, sizeof(m_playerNum));
   file->Read((char *) &m_player, sizeof(m_player));

   // Make sure we cope with any remappings...
   m_player = ObjRemapOnLoad(m_player);
}

// as a note, may systems make sure that the write operation works
// when trying to write sensitive data to disk
STDMETHODIMP_(void) cAvatar::WriteTagInfo(ITagFile *file)
{
   file->Write((char *) &m_playerNum, sizeof(m_playerNum));
   file->Write((char *) &m_player, sizeof(m_player));
}

STDMETHODIMP_(ulong) cAvatar::PlayerNum()
{
   return m_playerNum;
}

STDMETHODIMP_(ObjID) cAvatar::PlayerObjID()
{
   return m_player;
}
