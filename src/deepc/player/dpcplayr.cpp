/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <appagg.h>
#include <aggmemb.h>

#include <lgassert.h>
#include <iobjsys.h>
#include <psnd.h>
#include <mnamprop.h>
#include <config.h>
#include <cfgdbg.h>
#include <playrobj.h>
#include <schema.h>
#include <contain.h>
#include <objhp.h>
#include <mprintf.h>
#include <scrptapi.h>
#include <resapi.h>
#include <resapp.h>
#include <gamestr.h>
#include <plyrmode.h>
#include <combprop.h>
#include <traitman.h>

#include <dpcplayr.h>

#include <gunprop.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcpgapi.h>
#include <dpcinv.h>
#include <dpcincst.h>
#include <dpcinvpr.h>
#include <dpcprop.h>
#include <dpcarmpr.h>
#include <dpcutils.h>
#include <dpcammov.h>
#include <dpcparam.h>
#include <dpcobjst.h>

#include <dpctrcst.h>
#include <dpcplcst.h>
#include <dpcstcst.h>

#include <dpcplprp.h>
#include <engfprop.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <questapi.h>
#include <drkdiff.h>

#include <sdesc.h>
#include <sdesbase.h>

// armor
#include <dpcpldmg.h>

// melee weaps
#include <dpcmelee.h>
#include <weapon.h>

// ai mods
#include <dpcai.h>
#include <aiprcore.h>
#include <aivision.h>

#include <dpccam.h> // Binoc and gun zoom

// must be last headers!!!!
#include <dbmem.h>
#include <initguid.h>
#include <dpcpliid.h>

const float kCharExpPerLevel = 100; // @HACK: experience/level

const Label externalModelName = {"playermd"};

//////////////////////////////
// Various properties, etc.
//////////////////////////////


IStatsDescProperty *g_BaseStatsProperty;
IStatsDescProperty *g_DrugStatsProperty;
IStatsDescProperty *g_ImplantStatsProperty;
IStatsDescProperty *g_ArmorStatsProperty;
IStringProperty *g_PlayerNameProperty;
ITraitsDescProperty *g_TraitsProperty;
IWeaponSkillsProperty *g_BaseWeaponProperty;
ITechSkillsProperty *g_BaseTechProperty;
ITechSkillsProperty *g_ImplantTechProperty;

IStatsDescProperty *g_ReqStatsProperty;
ITechSkillsProperty *g_ReqTechProperty;
IIntProperty *g_ImplantProperty;
IIntProperty *g_ServiceProperty;

IIntProperty *g_CGYearProperty;
IIntProperty *g_CGRoomProperty;

IIntProperty *g_ResearchReportProperty;
IIntProperty *g_MapLocProperty;
IIntProperty *g_HelpTextProperty;

ILogDataProperty *g_LogProperties[NUM_PDA_LEVELS];

void BaseStatsPropertyInit();

// should all these faux tables go into a resource or something?
// probably...
#define MAX_LEVEL    9
//static int trait_bonus[] = { 0, 0, 1, 0, 0, 2, 0, 0, 3 }; // at what levels do these trait slots open up

#define GET_STAT(x) (stats.m_stats[x])

/* ------------------------------------------------------------ */
//////////////////////////////
// Our actual COM implementation!

static sRelativeConstraint DPCPlayerConstraints[] =
{
    { kConstrainAfter, &IID_ILinkManager},
    { kConstrainAfter, &IID_IContainSys},
    { kConstrainAfter, &IID_Res},
    { kConstrainAfter, &IID_IPropertyManager},
    { kNullConstraint,}
};

class cDPCPlayer : public cCTDelegating<IDPCPlayer>,
public cCTAggregateMemberControl<kCTU_Default>
{

protected:

    Label m_internalModelName;     // what model we use when in 1st person

private:
    ObjID m_nextWeapon;
    BOOL m_nextSfx;
    BOOL m_wieldNextFrame;

    STDMETHOD_(void,DeferNextWeapon)(ObjID o, BOOL sfx = TRUE)
    {
        m_nextWeapon = o;
        m_nextSfx = sfx;
    }


public:

    cDPCPlayer(IUnknown* pOuter)
       :m_nextWeapon(0), m_nextSfx(FALSE), m_wieldNextFrame(0)
    {
        MI_INIT_AGGREGATION_1(pOuter, IDPCPlayer, kPriorityNormal, DPCPlayerConstraints);
    }

/* ------------------------------------------------------------ */
    STDMETHOD(Init)()
    {
        BaseStatsPropertyInit();

        //ClearEquip();

        if (config_get_raw("player_model", (char*)(m_internalModelName.text), sizeof(Label)))
            m_internalModelName.text[sizeof(Label)-1] = '\0';


        return(S_OK);
    }

/* ------------------------------------------------------------ */
    STDMETHOD(UseInternalModel)(void)
    {
        ObjSetModelName(PlayerObject(), (char*)(m_internalModelName.text));
        return(S_OK);
    }

/* ------------------------------------------------------------ */

    STDMETHOD(UseExternalModel)(void)
    {
        ObjGetModelName(PlayerObject(), (char*)(m_internalModelName.text));
        ObjSetModelName(PlayerObject(), (char*)(externalModelName.text));
        return(S_OK);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(char *,GetExternalModel)(void)
    {
        return(char*)(externalModelName.text);
    }

/* ------------------------------------------------------------ */
    STDMETHOD_(int,StatCost)(int which)
    {
        sStatsDesc *stats;
        sStatCost *scp;
        int retval;

        sDiffParams *diff;
        AutoAppIPtr(QuestData);
        int g_diff = pQuestData->Get(DIFF_QVAR); 
        diff = GetDiffParams();

        int val = 0;

        if (g_BaseStatsProperty->Get(PlayerObject(), &stats))
            val = stats->m_stats[which];

        scp = GetStatCosts();
        retval = (scp->costs[which][val-1]); 
        if (diff->m_traincost[g_diff] != 0)
            retval = retval * diff->m_traincost[g_diff];
        return(retval);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(int,WeaponSkillCost)(int which)
    {
        sWeaponSkills *weapons;
        sWeaponSkillCost *wcp;
        int retval;

        sDiffParams *diff;
        AutoAppIPtr(QuestData);
        int g_diff = pQuestData->Get(DIFF_QVAR); 
        diff = GetDiffParams();

        int val = 0; 
        if (g_BaseWeaponProperty->Get(PlayerObject(), &weapons))
            val = weapons->m_wpn[which];

        wcp = GetWeaponSkillCosts();
        retval = (wcp->costs[which][val]); 

        if (diff->m_traincost[g_diff] != 0)
            retval = retval * diff->m_traincost[g_diff];
        return(retval);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(int,TechSkillCost)(int which)
    {
        sTechSkills *techs;
        sTechSkillCost *wcp;
        int val = 0;
        int retval;

        sDiffParams *diff;
        AutoAppIPtr(QuestData);
        int g_diff = pQuestData->Get(DIFF_QVAR); 
        diff = GetDiffParams();

        if (g_BaseTechProperty->Get(PlayerObject(), &techs))
            val = techs->m_tech[which];

        wcp = GetTechSkillCosts();
        retval = (wcp->costs[which][val]); 

        if (diff->m_traincost[g_diff] != 0)
            retval = retval * diff->m_traincost[g_diff];

        return(retval);
    }
/* ------------------------------------------------------------ */
/*
      // give HP
      ObjGetMaxHitPoints(obj, &maxhp);
      maxhp = maxhp + 10;
      maxhp = maxhp + hp_bonus[statp->m_stats[kStatEndurance]];
      if (HasTrait(obj,kTraitTank))
         maxhp = maxhp + 1;
      ObjSetMaxHitPoints(obj, maxhp);
*/   

/* ------------------------------------------------------------ */
// goes and actually set up the links and whatnot, after
// object has been approved by the higher level Equip() func
// OBJ_NULL means to clear out the slot
    STDMETHOD(SetEquip)(ObjID playerobj, ePlayerEquip slot, ObjID o)
    {
        AutoAppIPtr(ContainSys);
        if (o == OBJ_NULL)
        {
            ObjID prevobj = GetEquip(playerobj, slot);
            if (prevobj != OBJ_NULL) {
                pContainSys->Remove(playerobj,prevobj);
                if (gPropIsBinoculars->IsRelevant(prevobj)) {
                  // Do binoc unzoom
                  ZoomTarget(1.0, 0.0);
                }
            }
        }
        else
        {
            // clear out the old contents
            SetEquip(playerobj,slot,OBJ_NULL);
            pContainSys->Add(playerobj,o,DPCCONTAIN_PDOLLBASE + slot, CTF_NONE);
            if (slot == kEquipWeaponAlt) {
               if (gPropIsBinoculars->IsRelevant(o)) {
                 // Do binoc zoom
                 ZoomTarget(16.0, 0.0);
               }
            }
        }
        DPCAmmoRefreshButtons();
        return(S_OK);
    }


    STDMETHOD_(void,NowNextWeapon)(BOOL WaitAFrame)
    {
        if (!m_nextWeapon) return;
        // equip new weap
        if (IsPlayerGun(m_nextWeapon))
        {
            AutoAppIPtr(PlayerGun);
            if (m_nextSfx)
                pPlayerGun->Set(m_nextWeapon);
            else
                pPlayerGun->SetRaised(m_nextWeapon);
        }
        else if (IsMelee(m_nextWeapon))
        {
            if (WaitAFrame) {
               // Necessary to avoid trying to change player modes from
               // this exact point, which is called from inside the 
               // cerebellum code and thus doesn't quite work.
               m_wieldNextFrame = TRUE;
            } else {
               EquipMelee(PlayerObject(), m_nextWeapon, kSWT_Wrench);
               m_nextWeapon = m_nextSfx = 0;
            }
        } 
        else 
        {
           m_nextWeapon = m_nextSfx = 0;
        }
    }

/* ------------------------------------------------------------ */

    STDMETHOD_(void,SetWeaponModel)(ObjID o, BOOL sfx = TRUE)
    {
        AutoAppIPtr(PlayerGun);
        ObjID meleeID;

        // turn off gun
        pPlayerGun->Off();
        // turn off melee weapon, if any
        if ((meleeID = GetWeaponObjID(PlayerObject())) != OBJ_NULL)
        {
            UnEquipMelee(PlayerObject(), meleeID);
            DeferNextWeapon(o, sfx); 
        }
        else 
        {
            DeferNextWeapon(o, sfx); 
            NowNextWeapon(FALSE);
        }
    }

/* ------------------------------------------------------------ */
    STDMETHOD_(BOOL,CheckRequirements)(ObjID o, BOOL spew)
    {
        BOOL allow = TRUE;
        if ((o != OBJ_NULL) && ((g_ReqStatsProperty->IsRelevant(o) || (g_BaseWeaponProperty->IsRelevant(o)))))
        {
            sStatsDesc stats, *req;
            sWeaponSkills skills, *reqskill;

            if (!GetStats(PlayerObject(), &stats))
                allow = FALSE;

            if (!GetWeaponSkills(PlayerObject(), &skills))
                allow = FALSE;

            if (g_ReqStatsProperty->Get(o,&req))
            {
                if (!CheckStats(&stats,req,spew))
                {
                    ConfigSpew("equip",("insufficient stats to use item %d\n",o));
                    allow = FALSE;
                }
            }

            if (g_BaseWeaponProperty->Get(o,&reqskill))
            {
                if (!CheckWeaponSkills(&skills, reqskill, spew))
                {
                    ConfigSpew("equip",("insufficient skill to use item %d\n",o));
                    allow = FALSE;
                }
            }
        }
        return(allow);
    }

/* ------------------------------------------------------------ */
    // hm, this is getting awfully large.  split up?
    STDMETHOD_(BOOL,Equip)(ObjID who, ePlayerEquip slot, ObjID o, BOOL spew)
    {
        BOOL retval = FALSE;

        // no matter which slot, check that the player meets the required stats
        if (!CheckRequirements(o, spew))
            return(FALSE);

#if 0
        eObjState st;
        st = ObjGetObjState(o);
        if (st != kObjStateNormal)
        {
            if (st == kObjStateUnresearched)
            {
                char temp[255];
                DPCStringFetch(temp, sizeof(temp), "EquipUnresearched", "misc");
                DPCOverlayAddText(temp, DEFAULT_MSG_TIME);
            }
            return(FALSE);
        }
#endif

        switch (slot)
        {
            case kEquipWeapon:
                if ((o == OBJ_NULL) || IsPlayerGun(o) || IsMelee(o))
                {
                    if (slot == kEquipWeapon)
                    {
                        SetWeaponModel(o);
                    }

                    SetEquip(who, slot,o);
                    retval = TRUE;
                }
                break;
            case kEquipArmor:
            case kEquipSpecial:
            case kEquipSpecial2:
                {
                    BOOL okay = FALSE;
                    ObjID imp1, imp2;
                    int impval1, impval2, val;
                    impval1 = -1;
                    impval2 = -1;
                    val = 0;

                    imp1 = GetEquip(who, kEquipSpecial);
                    g_ImplantProperty->Get(imp1,&impval1);
                    imp2 = GetEquip(who, kEquipSpecial2);
                    g_ImplantProperty->Get(imp2,&impval2);

                    if (o == OBJ_NULL)
                        okay = TRUE;
                    else if ((slot == kEquipSpecial) && g_ImplantProperty->IsRelevant(o))
                    {
                        // what is in the other slot, and is the same?
                        g_ImplantProperty->Get(o,&val);
                        if (impval2 == val)
                        {
                            char temp[255];
                            DPCStringFetch(temp,sizeof(temp),"DoubleImplant","misc");
                            DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
                        }
                        else
                            okay = TRUE;
                    }
                    else if ((slot == kEquipSpecial2) && g_ImplantProperty->IsRelevant(o) && HasTrait(who,kTraitCybernetic))
                    {
                        // what is in the other slot, and is the same?
                        g_ImplantProperty->Get(o,&val);
                        if (impval1 == val)
                        {
                            char temp[255];
                            DPCStringFetch(temp,sizeof(temp),"DoubleImplant","misc");
                            DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
                        }
                        else
                            okay = TRUE;
                    }
                    else if ((slot == kEquipArmor) && ObjHasArmor(o))
                        okay = TRUE;
                    if (okay)
                    {
                        AutoAppIPtr(ScriptMan); 
                        ObjID old = GetEquip(who,slot);

                        retval = TRUE;
                        //SchemaPlay((Label*)"rustle", NULL);  // should change
                        // send out script messages
                        // okay, we are clearing this out, so send a turnoff to the old object
                        if (old != OBJ_NULL)
                        {
                            sScrMsg msg(old,"TurnOff"); 
                            pScriptMan->SendMessage(&msg); 
                        }
                        // activate the new thing
                        if (o != OBJ_NULL)
                        {
                            sScrMsg msg(o,"TurnOn"); 
                            pScriptMan->SendMessage(&msg); 
                        }
                        SetEquip(who,slot,o);
                    }
                }
                break;
            case kEquipPDA:
            case kEquipHack:
            case kEquipRepair:
            case kEquipModify:
            case kEquipResearch:
                if ((o== OBJ_NULL) || (gPropSoftwareType->IsRelevant(o) && gPropSoftwareLevel->IsRelevant(o)))
                {
                    int softtype;
                    gPropSoftwareType->Get(o,&softtype);
                    // make sure this is a matching type
                    if ((o != OBJ_NULL) && (softtype != slot - kEquipPDA))
                        break;

                    if (o != OBJ_NULL)
                        SchemaPlay((Label *)"boot_sw",NULL);
                    SetEquip(who,slot,o);
                    retval = TRUE;
                }
                break;
            default:
                SetEquip(who, slot,o);
                retval = TRUE;
                break;
        }

        if ((o != OBJ_NULL) && retval && spew)
        {
            char t1[255],t2[255];
            cStr str;
            AutoAppIPtr(GameStrings);
            DPCStringFetch(t1,sizeof(t1),"equip","misc");
            str = pGameStrings->FetchObjString(o, PROP_OBJSHORTNAME_NAME);
            sprintf(t2,t1,str);

            DPCOverlayAddText(t2,DEFAULT_MSG_TIME);
        }
        return(retval);
    }
/* ------------------------------------------------------------ */
    STDMETHOD(ClearEquip)(void)
    {
        int i;
        for (i=0; i < kEquipMax; i = i + 1)
            SetEquip(PlayerObject(),(ePlayerEquip)i,OBJ_NULL);
        return(S_OK);
    }
/* ------------------------------------------------------------ */
    STDMETHOD(AlternateWeapons)(ObjID player)
    {
        // make sure we don't have a 2-handed weapon up
        sPlayerGunDesc *ppgd;
        ObjID o1,o2;
        o1 = GetEquip(player,kEquipWeapon);
        o2 = GetEquip(player,kEquipWeaponAlt);
        if (PlayerGunDescGet(o1, &ppgd))
        {
            if (ppgd->m_handedness == kPlayerGunTwoHanded)
                return(S_FALSE);
        }
        // take 'em away
        Equip(player, kEquipWeapon,OBJ_NULL,FALSE);
        Equip(player, kEquipWeaponAlt, OBJ_NULL,FALSE);
        // put 'em back in, reversed
        Equip(player, kEquipWeapon, o2, FALSE);
        Equip(player, kEquipWeaponAlt, o1, FALSE);

        return(S_OK);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(ObjID,GetCurrentGun)(void) 
    {
        return GetEquip(PlayerObject(),kEquipWeapon);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(ObjID,GetEquip)(ObjID obj, ePlayerEquip slot) 
    { 
        sContainIter *piter;
        ObjID retval;
        AutoAppIPtr(ContainSys);
        eContainType cslot = DPCCONTAIN_PDOLLBASE + slot;
        piter = pContainSys->IterStartType(obj,cslot,cslot);
        if (piter->finished)
            retval = OBJ_NULL;
        else
            retval = piter->containee;
        pContainSys->IterEnd(piter);
        return(retval);
    }
/* ------------------------------------------------------------ */
// hmm, is the cost of adding all those irrelevant stats going to 
// bug us too much using this?
    STDMETHOD_(int,GetStat)(eStats which)
    {
        sStatsDesc desc;
        if (GetStats(PlayerObject(), &desc))
        {
            int retval = desc.m_stats[which - kStatStrength];
            if (retval < 1)
                retval = 1;
            if (retval > MAX_STAT_VAL)
                retval = MAX_STAT_VAL;
            return(retval);
        }
        else
            return(1);
    }
/* ------------------------------------------------------------ */
    STDMETHOD(AddStats)(sStatsDesc *p1, sStatsDesc *p2)
    {
        int i;
        if ((p1 == NULL) || (p2 == NULL))
        {
            Warning(("AddStats: p1 is %x, p2 is %x!\n",p1,p2));
            return(E_FAIL);
        }
        for (i=kStatStrength; i <= kStatCyber; i++)
        {
            p1->m_stats[i] += p2->m_stats[i];
            if (p1->m_stats[i]<1)
                p1->m_stats[i] = 1;
            else if (p1->m_stats[i]>MAX_STAT_VAL)
                p1->m_stats[i] = MAX_STAT_VAL;
        }
        return(S_OK);
    }
/* ------------------------------------------------------------ */
// returns whether or not every stat in stat set A is greater than or equal
// the equivalent stat in stat set B.
    STDMETHOD_(BOOL,CheckStats)(sStatsDesc *a, sStatsDesc *b, BOOL spew)
    {
        int i;
        char temp[255];

        if ((a == NULL) || (b == NULL))
        {
            Warning(("CheckStats: a is %x, b is %x!\n",a,b));
            return(FALSE);
        }
        for (i=kStatStrength; i <= kStatCyber; i++)
        {
            if (a->m_stats[i] < b->m_stats[i])
            {
                if (spew)
                {
                    char statname[32];
                    char formatstr[128];
                    DPCStringFetch(formatstr,sizeof(formatstr),"statreq","misc");
                    DPCStringFetch(statname,sizeof(statname),"statname","misc",i);
                    sprintf(temp,formatstr,statname,b->m_stats[i]);
                    DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
                }
                return(FALSE);
            }
        }
        return(TRUE);
    }
/* ------------------------------------------------------------ */
// returns whether or not every skill in skill set A is greater than or equal
// the equivalent skill in skill set B.
    STDMETHOD_(BOOL,CheckWeaponSkills)(sWeaponSkills *a, sWeaponSkills *b, BOOL spew)
    {
        int i;
        char temp[255];

        if ((a == NULL) || (b == NULL))
        {
            Warning(("CheckWeaponSkills: a is %x, b is %x!\n",a,b));
            return(FALSE);
        }
        for (i=kWeaponConventional; i <= kWeaponAnnelid; i++)
        {
            if (a->m_wpn[i] < b->m_wpn[i])
            {
                if (spew)
                {
                    char statname[32];
                    char formatstr[128];
                    DPCStringFetch(formatstr,sizeof(formatstr),"wpnreq","misc");
                    DPCStringFetch(statname,sizeof(statname),"wpnskill","misc",i);
                    sprintf(temp,formatstr,statname,b->m_wpn[i]);
                    DPCOverlayAddText(temp,DEFAULT_MSG_TIME);
                }
                return(FALSE);
            }
        }
        return(TRUE);
    }
/* ------------------------------------------------------------ */
// the idea here is to have a separate property for each category
// of thing that modifies the stats, so that we (simply) recompute
// it each time requested, rather than tracking deltas, which can
// get out of synch.
    STDMETHOD_(BOOL,GetStats)(ObjID obj, sStatsDesc *desc)
    {
        sStatsDesc *srcdesc;
        BOOL retval = FALSE;

        memset(desc,0,sizeof(sStatsDesc));

        // start with the players base stats
        if (g_BaseStatsProperty->Get(obj, &srcdesc))
        {
            retval = TRUE;
            AddStats(desc, srcdesc);
        }

        // apply any drug modifications
        if (g_DrugStatsProperty->Get(obj, &srcdesc))
        {
            retval = TRUE;
            AddStats(desc, srcdesc);
        }

        // apply implant modifications
        if (g_ImplantStatsProperty->Get(obj, &srcdesc))
        {
            retval = TRUE;
            AddStats(desc, srcdesc);
        }

        // apply armor modifications
        if (g_ArmorStatsProperty->Get(obj, &srcdesc))
        {
            retval = TRUE;
            AddStats(desc, srcdesc);
        }

        return(retval);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(BOOL,GetWeaponSkills)(ObjID obj, sWeaponSkills *wpn)
    {
        BOOL retval = FALSE;
        sWeaponSkills *basewpn;
        int i;

        if (g_BaseWeaponProperty->Get(obj, &basewpn))
        {
            for (i=kWeaponConventional; i<=kWeaponAnnelid; i++)
            {
                wpn->m_wpn[i] = basewpn->m_wpn[i];
            }
            retval = TRUE;
        }

        // return it
        return(retval);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(int,GetWeaponSkill)(eWeaponSkills which)
    {
        sWeaponSkills skill;
        GetWeaponSkills(PlayerObject(),&skill);
        return(skill.m_wpn[(int)which]);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(int,GetTechSkill)(eTechSkills which)
    {
        int retval = 0;
        sTechSkills *tech;
        ObjID obj = PlayerObject();

        // base skill
        if (g_BaseTechProperty->Get(obj, &tech))
            retval += tech->m_tech[which];

        // implant-added skill
        if (g_ImplantTechProperty->Get(obj, &tech))
            retval += tech->m_tech[which];

        // return it
        return(retval);
    }
/* ------------------------------------------------------------ */
    STDMETHOD_(BOOL,HasTrait)(ObjID obj, eTrait which)
    {
        sTraitsDesc *trait;
        int i;

        // punt out if no traits
        if (!g_TraitsProperty->Get(obj, &trait))
            return(FALSE);

        for (i=0; i < NUM_TRAIT_SLOTS; i++)
        {
            if (trait->m_traits[i] == which)
                return(TRUE);
        }

        return(FALSE);
    }

/* ------------------------------------------------------------ */
    STDMETHOD_(BOOL,HasTraitMulti)(eTrait which) 
    {
        AutoAppIPtr(NetManager);
        BOOL networking = pNetManager->Networking();
        if (!networking)
            return(HasTrait(PlayerObject(),which));

        ObjID netobj;
        FOR_ALL_PLAYERS(pNetManager, &netobj)
        {
            if (HasTrait(netobj,which))
                return(TRUE);
        }
        return(FALSE);
    }
/* ------------------------------------------------------------ */
// go through and set any derived properties
// if we got psyched we could check for which of these has actually
// changed...
    STDMETHOD(RecalcData)(ObjID obj)
    {
        sStatsDesc stats;
        sGameParams *gameparams = GetGameParams();
        sStatParams *statparams = GetStatParams();
        sTraitParams *traitparams = GetTraitParams();
        AutoAppIPtr(QuestData);
        int g_diff = pQuestData->Get(DIFF_QVAR); 
        sDiffParams *diff = GetDiffParams();

        if (!PlayerObjectExists())
        {
            return(E_FAIL);
        }
        if (!GetStats(obj, &stats))
        {
            return(E_FAIL);
        }
        if (!g_ContainDimsProperty->IsRelevant(obj))
        {
            return(E_FAIL);
        }
        if (!g_pBashParamsProp->IsRelevant(obj))
        {
            return(E_FAIL);
        }
        if (!g_pAIVisibilityModProperty->IsRelevant(obj))
        {
            return(E_FAIL);
        }

        int agility  = GET_STAT(kStatAgility);
        int strength = GET_STAT(kStatStrength);
        int cyb      = GET_STAT(kStatCyber);
        int endur    = GET_STAT(kStatEndurance);

        // Carrying capacity
        sContainDims dims; 
        sContainDims olddims,*dimptr;
        extern Point gPlayerMaxInv;

        // don't do this resizing or equip checking while in no-interface mode
        BOOL slim_mode = pQuestData->Get("HideInterface");

        if (!slim_mode)
        {
            dims.m_width     = gPlayerMaxInv.x;
            dims.m_height    = gPlayerMaxInv.y;
            g_ContainDimsProperty->Get(obj, &dimptr);
            olddims.m_width  = dimptr->m_width;
            olddims.m_height = dimptr->m_height;

            // note, could be greater or smaller
            if ((dims.m_width != olddims.m_width))
            {
                // set the actual property
                g_ContainDimsProperty->Set(obj, &dims);
                // readjust inventory appropriately
                DPCInvResize(obj, &olddims, &dims);
            }

            // verify that we can still use all of our current equipment
            int slot;
            for (slot = kEquipWeapon; slot < kEquipSpecial2; slot++)
            {
                ObjID equipped;
                equipped = GetEquip(obj,(ePlayerEquip)slot);

                if (!CheckRequirements(equipped,TRUE))
                {
                    // un-equip it
                    Equip(obj,(ePlayerEquip)slot,OBJ_NULL,FALSE);

                    // try to put it back in our inventory
                    // (or throw out into world)
                    DPCInvAddObj(obj,equipped);
                }
            }
        }

        // fall vulnerability
        sBashParamsProp *bpp, newbpp;
        if (g_pBashParamsProp->Get(obj, &bpp))
        {
            memcpy(&newbpp,bpp,sizeof(sBashParamsProp));
            newbpp.coeff = gameparams->bash[agility-1];
            g_pBashParamsProp->Set(obj, &newbpp);
        }

        // running speed
        float speed = gameparams->speed[agility - 1];
        if (speed == 0.0f)
        {
            speed = 1.0f;
        }
        AddSpeedScale("AGI", speed, 1.0);

        // visibility
        sAIVisibilityMods *pVisibility = new sAIVisibilityMods;

        // get default vis from player archetype
        AutoAppIPtr(TraitManager);
        ObjID playerArch = pTraitManager->GetArchetype(obj);
        // there better be a player archetype...
        Assert_(playerArch != OBJ_NULL);
        sAIVisibilityMods* pDefaultVisibility = AIGetVisibilityMod(playerArch);
        for (int i=0; i < kAIMaxVisibilityMods; i++)
        {
            pVisibility->m_mods[i] = pDefaultVisibility->m_mods[i];
        }

        // apply min->max visibility
        float hackvis = 0.0f;
        if (!gPropHackVisibility->Get(obj, &hackvis))
        {
            hackvis = 1.0f;
        }
        pVisibility->m_mods[kAIVisionCamera] = (hackvis) * (float)(statparams->m_camvismin + 
                                                                   (float(MAX_STAT_VAL-cyb)/float(MAX_STAT_VAL-1))*(statparams->m_camvismax-statparams->m_camvismin));
        // worm blend implant
        AutoAppIPtr(DPCPlayer);

        g_pAIVisibilityModProperty->Set(obj, pVisibility);
        // @TODO: is this the right thing?
        delete pVisibility;

        // get extra hit points
        // does this need to take into account any implants?
        int base = diff->m_basehp[g_diff];
        if (base == 0)
        {
            base = statparams->m_hpbase;
        }
        int bonus = diff->m_hpEND[g_diff];
        if (bonus == 0)
        {
            bonus = statparams->m_hpbonus;
        }

        int hp;
        int oldmax;
        ObjGetMaxHitPoints(obj,&oldmax);
        int maxhp = (endur * bonus) + base;
        if (HasTrait(obj,kTraitTank))
        {
            maxhp += traitparams->m_hpbonus;
        }
        ObjSetMaxHitPoints(obj, maxhp);
        ObjGetHitPoints(obj, &hp);
        hp = hp + (maxhp - oldmax);
        if (hp > maxhp)
        {
            hp = maxhp;
        }
        if (hp < 1)
        {
            hp = 1;
        }
        ObjSetHitPoints(obj, hp);

        return(S_OK);
    }

/* ------------------------------------------------------------ */
    STDMETHOD_(int,GetPool)(ObjID obj) 
    {
        ObjID cookies;
        int retval;

        cookies = GetEquip(obj, kEquipFakeCookies);
        if (cookies == OBJ_NULL)
            return(0);

        if (gStackCountProp->Get(cookies, &retval))
            return(retval);
        return(0);
    }

    STDMETHOD(AddPool)(ObjID obj, int quantity) 
    {
        AutoAppIPtr(ContainSys);
        ObjID cookies = GetEquip(obj, kEquipFakeCookies);
        pContainSys->StackAdd(cookies,quantity);
        return(S_OK);
    }

    STDMETHOD(SetPool)(ObjID obj, int value)
    {
        ObjID cookies = GetEquip(obj, kEquipFakeCookies);
        gStackCountProp->Set(cookies,value);
        return(S_OK);
    }

    STDMETHOD_(float, GetStimMultiplier)(ObjID gunID)
    {
        int weaponType = kWeaponConventional;
        // Following not happening for DC, this routine probably going away
#if 0
        if (!g_pWeaponTypeProperty->Get(gunID, &weaponType))
        {
            Warning(("GetStimMultiplier: Gun %d has no weapon type\n", gunID));
            return 1.;
        }
#endif
        sWeaponSkills skills;
        int reqSkill = 0;
        if (GetWeaponSkills(gunID, &skills))
        {
           reqSkill = skills.m_wpn[weaponType];
        }
        return(1.0f+(GetWeaponSkill((eWeaponSkills)weaponType)-reqSkill)*GetSkillParams()->m_damageModifier);
    }

    STDMETHOD_(void, Frame)(void)
    {
       // Exists only to handle deferred-by-a-frame melee weapon wielding.
       if (m_wieldNextFrame) {
          m_wieldNextFrame = FALSE;
          EquipMelee(PlayerObject(), m_nextWeapon, kSWT_Wrench);
          m_nextWeapon = m_nextSfx = 0;
       }
       ZoomIntegrate();
    }

};  // end of COM object

// creation func
void DPCPlayerCreate()
{
    AutoAppIPtr(Unknown); 
    new cDPCPlayer(pUnknown); 
}

/* ------------------------------------------------------------ */
static void LGAPI ListenFunc(sPropertyListenMsg* msg, PropListenerData data)
{
    AutoAppIPtr(DPCPlayer);
    pDPCPlayer->RecalcData(msg->obj);
}
/* ------------------------------------------------------------ */
// Stats property description

// data ops
class cStatsDescDataOps: public cClassDataOps<sStatsDesc>
{
};

// storage class
class cStatsDescStore: public cHashPropertyStore<cStatsDescDataOps>
{
};

// property implementation class
class cStatsDescProperty: public cSpecificProperty<IStatsDescProperty, &IID_IStatsDescProperty, sStatsDesc*, cStatsDescStore>
{
    typedef cSpecificProperty<IStatsDescProperty, &IID_IStatsDescProperty, sStatsDesc*, cStatsDescStore> cParent; 

public:
    cStatsDescProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sStatsDesc); 

};

IStatsDescProperty *g_StatsDescProperty;

static sFieldDesc StatsDescFields[] = 
{
    {"STR", kFieldTypeInt, FieldLocation(sStatsDesc, m_stats[0]),},
    {"END", kFieldTypeInt, FieldLocation(sStatsDesc, m_stats[1]),},
    {"AGI", kFieldTypeInt, FieldLocation(sStatsDesc, m_stats[3]),},
    {"CYB", kFieldTypeInt, FieldLocation(sStatsDesc, m_stats[4]),},
};

static sStructDesc StatsDescStructDesc = 
StructDescBuild(sStatsDesc, kStructFlagNone, StatsDescFields);

IStatsDescProperty *CreateStatsDescProperty(sPropertyDesc *desc, 
                                            ePropertyImpl impl)
{
    IStatsDescProperty *pprop;
    StructDescRegister(&StatsDescStructDesc);
    pprop = new cStatsDescProperty(desc);
    //pprop->Listen(kListenPropModify|kListenPropSet|kListenPropUnset, ListenFunc,NULL);

    return pprop;
}

/* ------------------------------------------------------------ */
// Traits property description

// data ops
class cTraitsDescDataOps: public cClassDataOps<sTraitsDesc>
{
};

// storage class
class cTraitsDescStore: public cHashPropertyStore<cTraitsDescDataOps>
{
};

// property implementation class
class cTraitsDescProperty: public cSpecificProperty<ITraitsDescProperty, &IID_ITraitsDescProperty, sTraitsDesc*, cTraitsDescStore>
{
    typedef cSpecificProperty<ITraitsDescProperty, &IID_ITraitsDescProperty, sTraitsDesc*, cTraitsDescStore> cParent; 

public:
    cTraitsDescProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sTraitsDesc); 

};

ITraitsDescProperty *g_TraitsDescProperty;

static sFieldDesc TraitsDescFields[] = 
{
    {"Trait 1", kFieldTypeInt, FieldLocation(sTraitsDesc, m_traits[0]),},
    {"Trait 2", kFieldTypeInt, FieldLocation(sTraitsDesc, m_traits[1]),},
    {"Trait 3", kFieldTypeInt, FieldLocation(sTraitsDesc, m_traits[2]),},
    {"Trait 4", kFieldTypeInt, FieldLocation(sTraitsDesc, m_traits[3]),},
};

static sStructDesc TraitsDescStructDesc = 
StructDescBuild(sTraitsDesc, kStructFlagNone, TraitsDescFields);

ITraitsDescProperty *CreateTraitsDescProperty(sPropertyDesc *desc, 
                                              ePropertyImpl impl)
{
    StructDescRegister(&TraitsDescStructDesc);
    return new cTraitsDescProperty(desc);
}

/* ------------------------------------------------------------ */
// WeaponSkills property description

// data ops
class cWeaponSkillsDataOps: public cClassDataOps<sWeaponSkills>
{
};

// storage class
class cWeaponSkillsStore: public cHashPropertyStore<cWeaponSkillsDataOps>
{
};

// property implementation class
class cWeaponSkillsProperty: public cSpecificProperty<IWeaponSkillsProperty, &IID_IWeaponSkillsProperty, sWeaponSkills*, cWeaponSkillsStore>
{
    typedef cSpecificProperty<IWeaponSkillsProperty, &IID_IWeaponSkillsProperty, sWeaponSkills*, cWeaponSkillsStore> cParent; 

public:
    cWeaponSkillsProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sWeaponSkills); 

};

IWeaponSkillsProperty *g_WeaponSkillsProperty;

static sFieldDesc WeaponSkillsFields[] = 
{
    {"Conventional", kFieldTypeInt, FieldLocation(sWeaponSkills, m_wpn[0]),},
    {"Energy", kFieldTypeInt, FieldLocation(sWeaponSkills, m_wpn[1]),},
    {"Heavy", kFieldTypeInt, FieldLocation(sWeaponSkills, m_wpn[2]),},
    {"Annelid", kFieldTypeInt, FieldLocation(sWeaponSkills, m_wpn[3]),},
};

static sStructDesc WeaponSkillsStructDesc = 
StructDescBuild(sWeaponSkills, kStructFlagNone, WeaponSkillsFields);

IWeaponSkillsProperty *CreateWeaponSkillsProperty(sPropertyDesc *desc, 
                                                  ePropertyImpl impl)
{
    StructDescRegister(&WeaponSkillsStructDesc);
    return new cWeaponSkillsProperty(desc);
}

/* ------------------------------------------------------------ */
// Tech Skills property description

// data ops
class cTechSkillsDataOps: public cClassDataOps<sTechSkills>
{
};

// storage class
class cTechSkillsStore: public cHashPropertyStore<cTechSkillsDataOps>
{
};

// property implementation class
class cTechSkillsProperty: public cSpecificProperty<ITechSkillsProperty, &IID_ITechSkillsProperty, sTechSkills*, cTechSkillsStore>
{
    typedef cSpecificProperty<ITechSkillsProperty, &IID_ITechSkillsProperty, sTechSkills*, cTechSkillsStore> cParent; 

public:
    cTechSkillsProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sTechSkills); 

};

ITechSkillsProperty *g_TechSkillsProperty;

static sFieldDesc TechSkillsFields[] = 
{
    {"Hack", kFieldTypeInt, FieldLocation(sTechSkills, m_tech[0]),},
    {"Repair", kFieldTypeInt, FieldLocation(sTechSkills, m_tech[1]),},
    {"Modify", kFieldTypeInt, FieldLocation(sTechSkills, m_tech[2]),},
    {"Maintain", kFieldTypeInt, FieldLocation(sTechSkills, m_tech[3]),},
    {"Research", kFieldTypeInt, FieldLocation(sTechSkills, m_tech[4]),},
};

static sStructDesc TechSkillsStructDesc = 
StructDescBuild(sTechSkills, kStructFlagNone, TechSkillsFields);

ITechSkillsProperty *CreateTechSkillsProperty(sPropertyDesc *desc, 
                                              ePropertyImpl impl)
{
    StructDescRegister(&TechSkillsStructDesc);
    return new cTechSkillsProperty(desc);
}

/* ------------------------------------------------------------ */
// LogData property description

// data ops
class cLogDataDataOps: public cClassDataOps<sLogData>
{
};

// storage class
class cLogDataStore: public cHashPropertyStore<cLogDataDataOps>
{
};

// property implementation class
class cLogDataProperty: public cSpecificProperty<ILogDataProperty, &IID_ILogDataProperty, sLogData*, cLogDataStore>
{
    typedef cSpecificProperty<ILogDataProperty, &IID_ILogDataProperty, sLogData*, cLogDataStore> cParent; 

public:
    cLogDataProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sLogData); 

};

ILogDataProperty *g_LogDataProperty;

char *stupid_bits[] = {"1","2","3","4","5","6","7","8","9",
    "10","11","12","13","14","15","16","17","18","19",
    "20","21","22","23","24","25","26","27","28","29",
    "30","31","32",
};

static sFieldDesc LogDataFields[4] = 
{
    {"Emails", kFieldTypeBits, FieldLocation(sLogData, m_data[0]),FullFieldNames(stupid_bits)},
    {"Logs", kFieldTypeBits, FieldLocation(sLogData, m_data[1]),FullFieldNames(stupid_bits)},
    {"Notes", kFieldTypeBits, FieldLocation(sLogData, m_data[2]),FullFieldNames(stupid_bits)},
    {"Videos", kFieldTypeBits, FieldLocation(sLogData, m_data[3]),FullFieldNames(stupid_bits)},
};

static sStructDesc LogDataStructDesc = 
StructDescBuild(sLogData, kStructFlagNone, LogDataFields);

ILogDataProperty *CreateLogDataProperty(sPropertyDesc *desc, 
                                        ePropertyImpl impl)
{
    StructDescRegister(&LogDataStructDesc);
    return new cLogDataProperty(desc);
}

/* ------------------------------------------------------------ */
#define PROP_BASE_STATS_DESC "BaseStatsDesc"
static sPropertyDesc BaseStatsDesc = { PROP_BASE_STATS_DESC, 0, NULL, 1, 0, {"Player", "Base Stats"},};
#define PROP_DRUG_STATS_DESC "DrugStatsDesc"
static sPropertyDesc DrugStatsDesc = { PROP_DRUG_STATS_DESC, 0, NULL, 1, 0, {"Player", "Drug Stats"},};
#define PROP_IMPLANT_STATS_DESC "ImplStatsDesc"
static sPropertyDesc ImplantStatsDesc = { PROP_IMPLANT_STATS_DESC, 0, NULL, 1, 0, {"Player", "Implant Stats"},};
#define PROP_ARMOR_STATS_DESC "ArmrStatsDesc"
static sPropertyDesc ArmorStatsDesc = { PROP_ARMOR_STATS_DESC, 0, NULL, 1, 0, {"Player", "Armor Stats"},};
#define PROP_PLR_NAME_DESC "PlayerNameDesc"
static sPropertyDesc PlayerNameDesc = { PROP_PLR_NAME_DESC, 0, NULL, 1, 0, {"Player", "Name"},};
#define PROP_REQ_STATS_DESC "ReqStatsDesc"
static sPropertyDesc ReqStatsDesc = { PROP_REQ_STATS_DESC, 0, NULL, 1, 0, {"Obj", "Required Stats"},};
#define PROP_REQ_TECH_DESC "ReqTechDesc"
static sPropertyDesc ReqTechDesc = { PROP_REQ_TECH_DESC, 0, NULL, 1, 0, {"Obj", "Required Tech Skill"},};
#define PROP_TRAITS_DESC "TraitsDesc"
static sPropertyDesc TraitsDesc = { PROP_TRAITS_DESC, 0, NULL, 1, 0, {"Player", "Traits"},};
#define PROP_WEAPONS_DESC "BaseWeaponDesc"
static sPropertyDesc BaseWeaponDesc = { PROP_WEAPONS_DESC, 0, NULL, 1, 0, {"Player", "Weapon Skills"},};
#define PROP_TECH_DESC "BaseTechDesc"
static sPropertyDesc BaseTechDesc = { PROP_TECH_DESC, 0, NULL, 1, 0, {"Player", "Tech Skills"},};
#define PROP_IMPLANT_TECH_DESC "ImplTechDesc"
static sPropertyDesc ImplantTechDesc = { PROP_IMPLANT_TECH_DESC, 0, NULL, 1, 0, {"Player", "Implant Skills"},};
#define PROP_IMPLANT_DESC "ImplantDesc"
static sPropertyDesc ImplantDesc = { PROP_IMPLANT_DESC, 0, NULL, 1, 0, {"Obj", "Implant"},};

#define PROP_MAPLOC_DESC "MapLoc"
static sPropertyDesc MapLocDesc = {PROP_MAPLOC_DESC, 0, NULL, 1, 0, {"Gamesys", "Map Loc"},};

#define PROP_CGYEAR_DESC "CharGenYear"
static sPropertyDesc CGYearDesc = {PROP_CGYEAR_DESC, 0, NULL, 1, 0, {"Player", "Char Gen Year"},};
#define PROP_CGROOM_DESC "CharGenRoom"
static sPropertyDesc CGRoomDesc = {PROP_CGROOM_DESC, 0, NULL, 1, 0, {"Player", "Char Gen Room"},};

static char *ServiceNames[] = { "Marines", "Navy", "OSA"};
#define PROP_SERVICE_DESC "Service"

static sPropertyDesc ServiceDesc =
{
    PROP_SERVICE_DESC, 0, 
    NULL, 0, 0, // constraints, versions
    { "Player", "Service"},
};

static sFieldDesc ServiceFields[] = 
{
    { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 3, 3, ServiceNames},
};

static sStructDesc ServiceStructDesc = 
{
    PROP_SERVICE_DESC, 
    sizeof(int),
    kStructFlagNone,
    sizeof(ServiceFields)/sizeof(ServiceFields[0]),
    ServiceFields,
}; 

static sPropertyTypeDesc ServiceTypeDesc = {PROP_SERVICE_DESC, sizeof(int)}; 

// log properties
// okay, this is retarded, I admit.
static sPropertyDesc LogPropertyDescs[NUM_PDA_LEVELS] = {
    { "Logs1", 0, NULL, 1, 0, {"Logs", "Deck 1"},},
    { "Logs2", 0, NULL, 1, 0, {"Logs", "Deck 2"},},
    { "Logs3", 0, NULL, 1, 0, {"Logs", "Deck 3"},},
    { "Logs4", 0, NULL, 1, 0, {"Logs", "Deck 4"},},
    { "Logs5", 0, NULL, 1, 0, {"Logs", "Deck 5"},},
    { "Logs6", 0, NULL, 1, 0, {"Logs", "Deck 6"},},
    { "Logs7", 0, NULL, 1, 0, {"Logs", "Deck 7"},},
    { "Logs8", 0, NULL, 1, 0, {"Logs", "Deck 8"},},
    { "Logs9", 0, NULL, 1, 0, {"Logs", "Deck 9"},},
};

static sFieldDesc LogFieldDescs[NUM_PDA_LEVELS][4];
static sStructDesc LogSDescs[NUM_PDA_LEVELS];
static char LogBitNames[NUM_PDA_LEVELS][4][32][128];

// must be kept in synch with enum!
static char *ImplantNames[] = 
{ 
    "Strength",
    "Endurance",
    "Agility",
    "Intelligence",
    "MaxHP",
    "Run",
    "Aim",
    "Tech",
    "Research",
    "WormMind",
    "WormBlood",
    "WormBlend",
    "WormHeart",
};

#define NUM_IMPLANTS (sizeof(ImplantNames)/sizeof(char *))
static sFieldDesc ImplantFieldDesc[] = 
{
    { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, NUM_IMPLANTS, NUM_IMPLANTS, ImplantNames},
}; 

static sStructDesc ImplantSDesc = 
{ PROP_IMPLANT_DESC,sizeof(int),kStructFlagNone,sizeof(ImplantFieldDesc)/sizeof(ImplantFieldDesc[0]),ImplantFieldDesc,}; 

static sPropertyTypeDesc ImplantTypeDesc = {PROP_IMPLANT_DESC, sizeof(int)}; 

static char *ReportNames[] = { 
// 1 - 5   
    "Once Grunt Organ", "Spore Organ", "Midwife Organ", "Grub Organ", "Anti Annelid Toxin", 
// 6 - 10
    "Wurmblood", "Crystal Shard", "Intelligence Hypo", "Swarm Organ", "Rumbler Organ",
// 11 - 15
    "Wurmmind", "Molecular Crazy Glue", "Arachnightmare Organ", "Minor Overlord Organ", "Wurmblend",
// 16 - 20
    "Viral Proliferator", "Worm-skin", "Worm Collector", "Greater Overlord Organ", "Wormheart",
// 21 - 25
    "Annelid Launcher", "Monkey Brain", "Healing Gland", "Psi Organ"
};

#define PROP_RESEARCHREPORT_DESC "RsrchRep"

static sPropertyDesc ResearchReportDesc =
{
    PROP_RESEARCHREPORT_DESC, 0, 
    NULL, 0, 0, // constraints, versions
    { "Player", "Research Report"},
};

static sFieldDesc ResearchReportFields[] = 
{
    { "", kFieldTypeBits, sizeof(int), 0, kFieldFlagNone, 0, 24, 24, ReportNames},
};

static sStructDesc ResearchReportStructDesc = 
{
    PROP_RESEARCHREPORT_DESC, 
    sizeof(int),
    kStructFlagNone,
    sizeof(ResearchReportFields)/sizeof(ResearchReportFields[0]),
    ResearchReportFields,
}; 

static sPropertyTypeDesc ResearchReportTypeDesc = {PROP_RESEARCHREPORT_DESC, sizeof(int)}; 


static char *HelpTextNames[] = { 
    "1", "2", "3", "4", "5",
    "6", "7", "8", "9", "10",   
    "11", "12", "13", "14", "15",
    "16", "17", "18", "19", "20",   
    "21", "22", "23", "24", "25",
    "26", "27", "28", "29", "30",   
};

#define PROP_HELPTEXT_DESC "HelpText"

static sPropertyDesc HelpTextDesc =
{
    PROP_HELPTEXT_DESC, 0, 
    NULL, 0, 0, // constraints, versions
    { "Player", "HelpText"},
};

static sFieldDesc HelpTextFields[] = 
{
    { "", kFieldTypeBits, sizeof(int), 0, kFieldFlagNone, 0, 30, 30, HelpTextNames},
};

static sStructDesc HelpTextStructDesc = 
{
    PROP_HELPTEXT_DESC, 
    sizeof(int),
    kStructFlagNone,
    sizeof(HelpTextFields)/sizeof(HelpTextFields[0]),
    HelpTextFields,
}; 

static sPropertyTypeDesc HelpTextTypeDesc = {PROP_HELPTEXT_DESC, sizeof(int)}; 

void BaseStatsPropertyInit()
{
    g_BaseStatsProperty    = CreateStatsDescProperty(&BaseStatsDesc,     kPropertyImplSparse);
    g_DrugStatsProperty    = CreateStatsDescProperty(&DrugStatsDesc,     kPropertyImplSparse);
    g_ImplantStatsProperty = CreateStatsDescProperty(&ImplantStatsDesc,  kPropertyImplSparse);
    g_ArmorStatsProperty   = CreateStatsDescProperty(&ArmorStatsDesc,    kPropertyImplSparse);
    
    g_PlayerNameProperty   = CreateStringProperty(&PlayerNameDesc,       kPropertyImplSparse);
    g_TraitsProperty       = CreateTraitsDescProperty(&TraitsDesc,       kPropertyImplVerySparse);
    g_BaseWeaponProperty   = CreateWeaponSkillsProperty(&BaseWeaponDesc, kPropertyImplSparse);
    g_BaseTechProperty     = CreateTechSkillsProperty(&BaseTechDesc,     kPropertyImplSparse);
    g_ImplantTechProperty  = CreateTechSkillsProperty(&ImplantTechDesc,  kPropertyImplSparse);
    
    g_ReqStatsProperty     = CreateStatsDescProperty(&ReqStatsDesc,      kPropertyImplSparse);
    g_ReqTechProperty      = CreateTechSkillsProperty(&ReqTechDesc,      kPropertyImplSparse);

    StructDescRegister(&ImplantSDesc);
    g_ImplantProperty    = CreateIntegralProperty(&ImplantDesc, &ImplantTypeDesc, kPropertyImplSparse);

    StructDescRegister(&ServiceStructDesc);
    g_ServiceProperty = CreateIntegralProperty(&ServiceDesc, &ServiceTypeDesc, kPropertyImplVerySparse);

    g_CGYearProperty = CreateIntProperty(&CGYearDesc, kPropertyImplVerySparse);
    g_CGRoomProperty = CreateIntProperty(&CGRoomDesc, kPropertyImplVerySparse);

    StructDescRegister(&ResearchReportStructDesc);
    g_ResearchReportProperty = CreateIntegralProperty(&ResearchReportDesc, &ResearchReportTypeDesc, kPropertyImplVerySparse);

    g_MapLocProperty = CreateIntProperty(&MapLocDesc, kPropertyImplVerySparse);

    StructDescRegister(&HelpTextStructDesc);
    g_HelpTextProperty = CreateIntegralProperty(&HelpTextDesc, &HelpTextTypeDesc, kPropertyImplVerySparse);

    int i;
    for (i=0;i < NUM_PDA_LEVELS; i++)
    {
        g_LogProperties[i] = CreateLogDataProperty(&LogPropertyDescs[i], kPropertyImplDense);
    }
}
/* ------------------------------------------------------------ */

void DPCGainPool(int amt)
{
    AutoAppIPtr(DPCPlayer);
    pDPCPlayer->AddPool(PlayerObject(), amt);
}

void DPCTrainMFD(int which)
{
    int mfd;
    switch (which)
    {
        case 0:
        {
            mfd = kOverlayBuyStats;
            break;
        }
        case 1:
        {
            mfd = kOverlayBuyTech;
            break;
        }
        case 2:
        {
            mfd = kOverlayBuyWeapon;
            break;
        }
    }
    DPCOverlayChange(mfd, kOverlayModeOn);
    DPCOverlaySetObj(mfd, PlayerObject());
}

/* ------------------------------------------------------------ */
void DPCSwapGuns(void)
{
    AutoAppIPtr(DPCPlayer);
    pDPCPlayer->AlternateWeapons(PlayerObject());
}
/* ------------------------------------------------------------ */

