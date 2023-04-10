/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// alarm icon
#include <2d.h>
#include <appagg.h>

#include <resapi.h>
#include <simtime.h>
#include <playrobj.h>
#include <scrnmode.h>
#include <schema.h>
#include <filevar.h>
#include <propbase.h>
#include <scrptapi.h>

#include <mprintf.h>

#include <dpcalarm.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcplayr.h>
#include <dpcutils.h>
#include <dpcprop.h>
#include <dpcspawn.h>

IRes *gAlarmHnd;

//static Rect alarm_rect = {{10, 10},{10 + 80, 10 + 78}};
static Rect alarm_rect = {{10, 278},{10 + 64, 278 + 64}};

// Here's the type of my global 
struct sAlarmCount
{
    int m_count; 
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gAlarmCountDesc = 
{
    kMissionVar,         // Where do I get saved?
    "ALARMCOUNT",          // Tag file tag
    "Alarm Count",     // friendly name
    FILEVAR_TYPE(sAlarmCount),  // Type (for editing)
    { 1, 0},             // version
    { 1, 0},             // last valid version 
    "DPC",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sAlarmCount,&gAlarmCountDesc> gAlarmCount; 
//--------------------------------------------------------------------------------------
void DPCAlarmInit(int which)
{
    Rect use_rect;
    gAlarmHnd= LoadPCX("alarm");
    DPCOverlaySetFlags(which, kOverlayFlagTranslucent);

    sScrnMode smode;
    ScrnModeGet(&smode);

    use_rect.ul.x = alarm_rect.ul.x;
    use_rect.ul.y = smode.h - (480 - alarm_rect.ul.y);
    use_rect.lr.x = use_rect.ul.x + RectWidth(&alarm_rect);
    use_rect.lr.y = use_rect.ul.y + RectHeight(&alarm_rect);

    DPCOverlaySetRect(which, use_rect);
}

//--------------------------------------------------------------------------------------
void DPCAlarmTerm(void)
{
    SafeFreeHnd(&gAlarmHnd);
}

//--------------------------------------------------------------------------------------
void DPCAlarmDraw(unsigned long inDeltaTicks)
{
    Point p;
    Rect r = DPCOverlayGetRect(kOverlayAlarm);
    int t, deltat;
    int w,h;
    int duration;

    char temp[255];
    t = GetSimTime();
    if (!gPropHackTime->Get(PlayerObject(),&duration))
        return;
    deltat = duration - t;
    if (deltat < 0)
    {
        DPCAlarmDisableAll();
    }
    else
    {
        sprintf(temp,"%.1f",(float)(deltat)/1000.0F);
        w = gr_font_string_width(gDPCFont, temp);
        h = gr_font_string_height(gDPCFont, temp);
        p.x = r.ul.x + (RectWidth(&r) - w) / 2;
        p.y = r.ul.y + RectHeight(&r) + 2;
        gr_set_fcolor(gDPCTextColor);
        gr_font_string(gDPCFont, temp, p.x, p.y);
    }
}
//--------------------------------------------------------------------------------------
IRes *DPCAlarmBitmap(void)
{
    return(gAlarmHnd);
}
//--------------------------------------------------------------------------------------
static IRes *gHackIconHnd;
static Rect full_rect = {{10,278},{10 + 64, 278 + 64}};

//--------------------------------------------------------------------------------------
void DPCHackIconInit(int which)
{
    gHackIconHnd= LoadPCX("HackIcon");

    DPCOverlaySetFlags(which, kOverlayFlagTranslucent);

    sScrnMode smode;
    ScrnModeGet(&smode);
    Rect use_rect;

    use_rect.ul.x = full_rect.ul.x;
    use_rect.ul.y = smode.h - (480 - full_rect.ul.y);
    use_rect.lr.x = use_rect.ul.x + RectWidth(&full_rect);
    use_rect.lr.y = use_rect.ul.y + RectHeight(&full_rect);

    DPCOverlaySetRect(which, use_rect);
}

//--------------------------------------------------------------------------------------
void DPCHackIconTerm(void)
{
    SafeFreeHnd(&gHackIconHnd);
}

//--------------------------------------------------------------------------------------
IRes *DPCHackIconBitmap(void)
{
    return(gHackIconHnd);
}
//--------------------------------------------------------------------------------------
void DPCHackIconDraw(unsigned long inDeltaTicks)
{
    Point p;
    ObjID plr;
    int t, deltat;
    int w,h;
    int duration;
    char temp[255];
    Rect r = DPCOverlayGetRect(kOverlayHackIcon);

    AutoAppIPtr(DPCPlayer);

    plr = PlayerObject();
    if (plr == OBJ_NULL)
        return;

    t = GetSimTime();
    if (!gPropHackTime->Get(plr,&duration))
        return;
    deltat = duration - t;

    // okay, this is kind of dumb to do during drawing
    // but life is hard, then you die
    if (deltat <= 0)
    {
        SchemaPlay((Label *)"xer04",NULL);         
        DPCOverlayChange(kOverlayHackIcon, kOverlayModeOff);
        gPropHackVisibility->Set(plr,1.0);
        pDPCPlayer->RecalcData(plr);
        return;
    }

    sprintf(temp,"%.1f",(float)(deltat)/1000.0F);
    w = gr_font_string_width(gDPCFont, temp);
    h = gr_font_string_height(gDPCFont, temp);
    p.x = r.ul.x + (RectWidth(&r) - w) / 2;
    p.y = r.ul.y + RectHeight(&r) + 2;
    gr_set_fcolor(gDPCTextColor);
    gr_font_string(gDPCFont, temp, p.x, p.y);
}
//--------------------------------------------------------------------------------------
void DPCAlarmAdd(int time)
{
    // so that there is never both hacked and alert at same time
    AutoAppIPtr(DPCPlayer);
    DPCOverlayChange(kOverlayHackIcon, kOverlayModeOff);
    gPropHackVisibility->Set(PlayerObject(),1.0);
    pDPCPlayer->RecalcData(PlayerObject());

    if (gAlarmCount.m_count == 0)
        DPCOverlayChange(kOverlayAlarm,kOverlayModeOn);
    //else
    //mprintf("Hey!  AlarmCount is %d!\n",gAlarmCount.m_count);

    gAlarmCount.m_count++;
    gPropHackTime->Set(PlayerObject(),time);
}
//--------------------------------------------------------------------------------------
void DPCAlarmRemove(void)
{
    if (gAlarmCount.m_count > 0)
    {
        gAlarmCount.m_count--;
        if (gAlarmCount.m_count == 0)
        {
            DPCOverlayChange(kOverlayAlarm, kOverlayModeOff);
            gPropHackTime->Set(PlayerObject(),0);
        }
    }
}
//--------------------------------------------------------------------------------------
void DPCAlarmDisableAll(void)
{
    // iterate through all objects, and anything with a population system on it, 
    // and is alarmed, send it a reset, which will in turn reset any associated
    // cameras.
    ObjID obj;
    sPropertyObjIter iter;
    int ecostate;
    BOOL alert = FALSE;
    AutoAppIPtr(ScriptMan); 

    gPropEcology->IterStart(&iter);
    while (gPropEcology->IterNext(&iter, &obj))
    {
        // are we an ecology
        if (gPropEcoState->IsRelevant(obj))
        {
            gPropEcoState->Get(obj, &ecostate);
            if (ecostate == kEcologyAlert)
            {
                // resetting the ecology will in turn reset any associated
                // cameras & alarms
                sScrMsg msg(obj,"Reset"); 
                // This method may be running on any machine, so make sure the
                // message gets through to the owner of the object:
                msg.flags |= kSMF_MsgPostToOwner;
                pScriptMan->SendMessage(&msg); 
            }
        }
    }
    gPropEcology->IterStop(&iter);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayAlarm = 
{                                 
    DPCAlarmDraw,                 // draw
    DPCAlarmInit,                 // init
    DPCAlarmTerm,                 // term
    NULL,                         // mouse
    NULL,                         // dclick (really use)
    NULL,                         // dragdrop
    NULL,                         // key
    DPCAlarmBitmap,               // bitmap
    "",                           // upschema
    "",                           // downschema
    NULL,                         // state
    NULL,                         // transparency
    0,                            // distance
    FALSE,                        // needmouse
    75,                           // alpha
};                                

sOverlayFunc OverlayHackIcon =    
{                                 
    DPCHackIconDraw,              // draw
    DPCHackIconInit,              // init
    DPCHackIconTerm,              // term
    NULL,                         // mouse
    NULL,                         // dclick (really use)
    NULL,                         // dragdrop
    NULL,                         // key
    DPCHackIconBitmap,            // bitmap
    "",                           // upschema
    "",                           // downschema
    NULL,                         // state
    NULL,                         // transparency
    0,                            // distance
    FALSE,                        // needmouse
    128,                          // alpha
};                                
