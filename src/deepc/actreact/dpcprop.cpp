/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcprop.h>

#include <propert_.h>
#include <dataops_.h>
#include <gamestr.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <combprop.h>
#include <esnd.h>

#include <dpcobjst.h>
#include <dpcwsprp.h>
#include <dpcspawn.h>
#include <dpchazpr.h>
#include <dpccurpr.h>
#include <dpcemail.h>
#include <dpcutils.h>
#include <gunapi.h>
#include <dpcovrly.h>
#include <dpcovcst.h>

/////////////////////////////////////////////////////////////
// Object Name Type Property
/////////////////////////////////////////////////////////////
IIntProperty *gPropObjNameType = NULL;

static char *NameTypeNames[] = { "Normal", "Stack Count", "Log Title", "Weapon"};
#define PROP_NAMETYPE_DESC "NameType"

static sPropertyDesc NameTypeDesc =
{
    PROP_NAMETYPE_DESC, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Object Name Type"},
};

static sFieldDesc NameTypeFields[] = 
{
    { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 4, 4, NameTypeNames},
};

static sStructDesc NameTypeStructDesc = 
{
    PROP_NAMETYPE_DESC, 
    sizeof(int),
    kStructFlagNone,
    sizeof(NameTypeFields)/sizeof(NameTypeFields[0]),
    NameTypeFields,
}; 

static sPropertyTypeDesc NameTypeTypeDesc = {PROP_NAMETYPE_DESC, sizeof(int)}; 

/////////////////////////////////////////////////////////////
// Object Name Property
/////////////////////////////////////////////////////////////

IStringProperty* gPropObjName = NULL;

static sPropertyDesc ObjNameDesc =
{
    PROP_OBJNAME_NAME, 0, 
    NULL, 2, 1, // constraints, versions
    { "Obj", "Object name"},
};

#define OBJ_NAME_IMPL kPropertyImplHash

// careful about overflow!!
void ObjGetObjNameSubst(ObjID obj, char *text, int buflen)
{
    cStr str;

    eObjState objstate = ObjGetObjState(obj);

    AutoAppIPtr(GameStrings);
    str = pGameStrings->FetchObjString(obj, PROP_OBJNAME_NAME);

    if (buflen < str.GetLength())
    {
        Warning(("ObjGetObjNameSubst: buffer len %d is too small!\n",buflen));
    }

    strcpy(text,"");

    if (gPropMapText->IsRelevant(obj))
    {
        const char *pStr;
        gPropMapText->Get(obj,&pStr);
        strcpy(text,pStr); 
    }
    else
    {
        if (IProperty_IsRelevant(gPropObjNameType, obj))
        {
            int val;
            gPropObjNameType->Get(obj, &val);
            switch (val)
            {
                case 1: // %d for stack count
                    if (IProperty_IsRelevant(gStackCountProp, obj) && IProperty_IsRelevant(gCombineTypeProp, obj))
                    {
                        int val;
                        gStackCountProp->Get(obj,&val);
                        sprintf(text, str, val);
                    }
                    break;
                case 2: // %d for log title
                    {
                        int usetype = 1; // logs
                        int level, which;
                        if (DPCFindLogData(obj, usetype, &level, &which) == S_OK)
                        {
                            char temp[255];
                            char levelname[32];
                            sprintf(levelname,"level%02d",level + 1);
                            DPCStringFetch(temp,sizeof(temp),"logname",levelname,which+1);

                            sprintf(text, str, temp);
                        }
                    }
                    break;
                case 3: // %s for weapon condition
                    {
                        char temp[255];
                        GunGetConditionString(obj,temp,sizeof(temp));
                        sprintf(text,str,temp); //(int)GunGetCondition(obj));
                    }
                    break;
            }
        }

        // generic case if no name type, or we fail within our name type
        if (strlen(text) == 0)
        {
            strcpy(text, str);
        }
    }

    if (objstate != kObjStateNormal)
    {
        char temp[255];
        DPCStringFetch(temp,sizeof(temp),"ObjState","misc",objstate);
        strcat(text,temp);
    }
}


/////////////////////////////////////////////////////////////
// Object Name Property
/////////////////////////////////////////////////////////////
IStringProperty* gPropObjShortName = NULL;

static sPropertyDesc ObjShortNameDesc =
{
    PROP_OBJSHORTNAME_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Object short name"},
};

// careful about overflow!!
void ObjGetObjShortNameSubst(ObjID obj, char *text, int buflen)
{
    eObjState objstate = ObjGetObjState(obj);

    AutoAppIPtr(GameStrings);
    cStr str = pGameStrings->FetchObjString(obj, PROP_OBJSHORTNAME_NAME);

    if (buflen < str.GetLength())
    {
        Warning(("ObjGetObjNameSubst: buffer len %d is too small!\n",buflen));
    }

    strcpy(text,"");

    if (IProperty_IsRelevant(gPropObjNameType, obj))
    {
        int val;
        gPropObjNameType->Get(obj, &val);
        switch (val)
        {
            case 1: // %d for stack count
            {
                if (IProperty_IsRelevant(gStackCountProp, obj) && IProperty_IsRelevant(gCombineTypeProp, obj))
                {
                    int val;
                    gStackCountProp->Get(obj,&val);
                    sprintf(text, str, val);
                }
                break;
            }
            case 2: // %d for log title
            {
                int usetype = 1; // logs
                int level, which;
                if (DPCFindLogData(obj, usetype, &level, &which) == S_OK)
                {
                    char temp[255];
                    char levelname[32];
                    sprintf(levelname,"level%02d",level + 1);
                    DPCStringFetch(temp,sizeof(temp),"logname",levelname,which+1);

                    sprintf(text, str, temp);
                }
                break;
            }
            case 3: // %s for weapon condition
            {
                char temp[255];
                GunGetConditionString(obj,temp,sizeof(temp));
                sprintf(text,str,temp); //(int)GunGetCondition(obj));
                break;
            }
        }
    }

    // generic case if no name type, or we fail within our name type
    if (strlen(text) == 0)
    {
        strcpy(text, str);
    }
}

/////////////////////////////////////////////////////////////
// AI Frobbing Property
/////////////////////////////////////////////////////////////
IStringProperty* gPropAIFrob = NULL;

static sPropertyDesc AIFrobDesc =
{
    PROP_AIFROB, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Frob obj"},
};

/////////////////////////////////////////////////////////////
// Look String Property
/////////////////////////////////////////////////////////////
IStringProperty* gPropObjLookString = NULL;

static sPropertyDesc ObjLookStringDesc =
{
    PROP_OBJLOOKSTRING_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Object Look string"},
};

BOOL ObjGetObjLookString(ObjID obj, const char **ppString)
{
    Assert_(gPropObjLookString);
    return gPropObjLookString->Get(obj, ppString);
}

/////////////////////////////////////////////////////////////
// ExP property
/////////////////////////////////////////////////////////////

IIntProperty* gPropExp;

static sPropertyDesc ExpDesc =
{
    PROP_EXP_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "Exp"},
};

#define EXP_IMPL kPropertyImplHash

int ObjGetExp(ObjID obj)
{
    int exp = 0; // default value
    gPropExp->Get(obj,&exp);
    return exp;
}

/////////////////////////////////////////////////////////////
// Object Icon Property
/////////////////////////////////////////////////////////////

ILabelProperty* gPropObjIcon = NULL;

static sPropertyDesc ObjIconDesc =
{
    PROP_OBJICON_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Object icon"},
};

#define OBJ_ICON_IMPL kPropertyImplLlist

BOOL ObjGetObjIcon(ObjID obj, Label **ppName)
{
    Assert_(gPropObjIcon);
    Assert_(gPropObjBrokenIcon);

    eObjState objstate;
    objstate = ObjGetObjState(obj);
    if (objstate == kObjStateBroken)
    {
        if (gPropObjBrokenIcon->Get(obj, ppName))
            return(TRUE);
        else
            return(gPropObjIcon->Get(obj, ppName));
    }
    else
        return gPropObjIcon->Get(obj, ppName);

}

void ObjSetObjIcon(ObjID obj, Label *pName)
{
    Assert_(gPropObjIcon);
    gPropObjIcon->Set(obj, pName);
}

ILabelProperty* gPropObjBrokenIcon = NULL;

static sPropertyDesc ObjBrokenIconDesc =
{
    PROP_OBJBROKENICON_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Broken icon"},
};

/////////////////////////////////////////////////////////////
// Tech hacking / difficulty properties
/////////////////////////////////////////////////////////////

ITechInfoProperty* gPropHackDiff    = NULL;
ITechInfoProperty* gPropRepairDiff  = NULL;
ITechInfoProperty* gPropModifyDiff  = NULL;
ITechInfoProperty* gPropModify2Diff = NULL;
IStringProperty*   gPropModify1Text = NULL;
IStringProperty*   gPropModify2Text = NULL;
IStringProperty*   gPropHackText    = NULL;

static sPropertyDesc HackDiffDesc =
{
    PROP_HACKDIFF_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "HackDiff"},
};

static sPropertyDesc RepairDiffDesc =
{
    PROP_REPAIRDIFF_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "RepairDiff"},
};

static sPropertyDesc ModifyDiffDesc =
{
    PROP_MODIFYDIFF_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "ModifyDiff"},
};

static sPropertyDesc Modify2DiffDesc =
{
    PROP_MODIFY2DIFF_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "Modify 2 Diff"},
};

static sPropertyDesc Modify1TextDesc =
{
    PROP_MODIFY1TEXT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "Modification #1 Text"},
};

static sPropertyDesc Modify2TextDesc =
{
    PROP_MODIFY2TEXT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "Modification #2 Text"},
};

static sPropertyDesc HackTextDesc =
{
    PROP_HACKTEXT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "GameSys", "Hack Text"},
};

#define HACKDIFF_IMPL    kPropertyImplLlist
#define REPAIRDIFF_IMPL  kPropertyImplLlist
#define MODIFYDIFF_IMPL  kPropertyImplLlist
#define MODIFY2DIFF_IMPL kPropertyImplLlist
#define MODIFY1TEXT_IMPL kPropertyImplLlist
#define MODIFY2TEXT_IMPL kPropertyImplLlist
#define HACKTEXT_IMPL    kPropertyImplLlist

/////////////////////////////////////////////////////////////
// Loot property
/////////////////////////////////////////////////////////////

ILootInfoProperty* gPropLoot = NULL;

static sPropertyDesc LootDesc =
{
    PROP_LOOT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "AI", "Loot"},
};

#define LOOT_IMPL kPropertyImplLlist

ILabelProperty* gPropGuaranteedLoot = NULL;

static sPropertyDesc GuaranteedLootDesc =
{
    PROP_GUARANTEEDLOOT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "AI", "Borg Loot"},
};

ILabelProperty* gPropReallyGuaranteedLoot = NULL;

static sPropertyDesc ReallyGuaranteedLootDesc =
{
    PROP_REALLYGUARANTEEDLOOT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "AI", "Guarantee Loot"},
};

/////////////////////////////////////////////////////////////
// Map obj props
/////////////////////////////////////////////////////////////

ILabelProperty* gPropMapObjIcon = NULL;

static sPropertyDesc MapObjIconDesc =
{
    "MapObjIcon", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Map Icon"},
};

IBoolProperty* gPropMapObjRotate = NULL;

static sPropertyDesc MapObjRotateDesc =
{
    "MapObjRotate", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Map Icon Rotated?"},
};

IStringProperty* gPropMapText = NULL;

static sPropertyDesc MapTextDesc =
{
    "MapText", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Map Text"},
};

/////////////////////////////////////////////////////////////
// Use Message Property
/////////////////////////////////////////////////////////////

IStringProperty* gPropUseMessage = NULL;

static sPropertyDesc UseMessageDesc =
{
    PROP_USEMESSAGE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Use Message"},
};

/////////////////////////////////////////////////////////////
// Use Message Property
/////////////////////////////////////////////////////////////

IIntProperty* gPropMessageTime = NULL;

static sPropertyDesc MessageTimeDesc =
{
    "MsgTime", 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Message Time"},
};

/////////////////////////////////////////////////////////////
// Stack Increment Property
/////////////////////////////////////////////////////////////

IIntProperty* gPropStackIncrem = NULL;

static sPropertyDesc StackIncremDesc =
{
    "StackInc", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Stack Increment"},
};

/////////////////////////////////////////////////////////////
// Recycler Value Property
/////////////////////////////////////////////////////////////
IIntProperty* gPropRecycle = NULL;

static sPropertyDesc RecycleDesc =
{
    "Recycle", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Recycle Value"},
};

/////////////////////////////////////////////////////////////
// Locked Message Property
/////////////////////////////////////////////////////////////

IStringProperty* gPropLockedMessage = NULL;

static sPropertyDesc LockedMessageDesc =
{
    PROP_LOCKEDMESSAGE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Locked Message"},
};

/////////////////////////////////////////////////////////////
// HUD Duration property
/////////////////////////////////////////////////////////////

IIntProperty* gPropHUDTime = NULL;

static sPropertyDesc HUDTimeDesc =
{
    PROP_HUDTIME_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Renderer", "HUD Duration"},
    kPropertyChangeLocally,
};

/////////////////////////////////////////////////////////////
// Allow HUD property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropAllowHUDSelect = NULL;

static sPropertyDesc HUDSelectDesc =
{
    PROP_HUDSELECT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "HUD Selectable?"},
};

/////////////////////////////////////////////////////////////
// Hack Duration property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropShowHP = NULL;

static sPropertyDesc ShowHPDesc =
{
    "ShowHP", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Show HP?"},
};

/////////////////////////////////////////////////////////////
// Hack Duration property
/////////////////////////////////////////////////////////////

IIntProperty* gPropHackTime = NULL;

static sPropertyDesc HackTimeDesc =
{
    PROP_HACKTIME_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Player", "Hack Duration"},
};

/////////////////////////////////////////////////////////////
// Block frob property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropBlockFrob = NULL;

static sPropertyDesc BlockFrobDesc =
{
    PROP_BLOCKFROB_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Block Frob?"},
};

/////////////////////////////////////////////////////////////
// Alarmed property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropAlarm = NULL;

static sPropertyDesc AlarmDesc =
{
    PROP_ALARM_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Gamesys", "Alarm On?"},
};

/////////////////////////////////////////////////////////////
// Use Message Property
/////////////////////////////////////////////////////////////

IIntProperty* gPropMiniGames = NULL;

#define PROP_MINIGAMES_NAME   "MiniGames"
static sPropertyDesc MiniGamesDesc =
{
    PROP_MINIGAMES_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Player", "Mini Games"},
};

char *minigameflag_names[] = { 
    "Index", "Slots", "Ping", "Swinekeeper", "OverWorld", 
    "KaBacon", "Abyss", "Hogger", "TicTacToe", "Swine Hunter",
    "Pig Stacker", "Burro Hog", "Golf", "13", "14", 
};

static sFieldDesc miniGamesFields[] = 
{
    { "", kFieldTypeBits, sizeof(int), 0, kFieldFlagUnsigned, 0, 15, 15, minigameflag_names},
};

static sStructDesc miniGamesStructDesc = 
{
    PROP_MINIGAMES_NAME, 
    sizeof(int),
    kStructFlagNone,
    sizeof(miniGamesFields)/sizeof(miniGamesFields[0]),
    miniGamesFields,
}; 

static sPropertyTypeDesc miniGamesTypeDesc = {PROP_MINIGAMES_NAME, sizeof(int)}; 

/////////////////////////////////////////////////////////////
// Object State property
/////////////////////////////////////////////////////////////

IIntProperty* gPropObjState = NULL;

static char* objStateNames[] =
{
    "Normal", "Broken", "Destroyed", "Locked"
};

static sPropertyDesc objStateDesc =
{
    PROP_OBJ_STATE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "State"},
    kPropertyProxyChangable,
};

static sFieldDesc objStateFields[] = 
{
    { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 5, 5, objStateNames},
};

static sStructDesc objStateStructDesc = 
{
    PROP_OBJ_STATE_NAME, 
    sizeof(int),
    kStructFlagNone,
    sizeof(objStateFields)/sizeof(objStateFields[0]),
    objStateFields,
}; 

eObjState ObjGetObjState(ObjID objID)
{
    eObjState state = kObjStateNormal;

    gPropObjState->Get(objID, &state);
    return state;
}

void ObjSetObjState(ObjID objID, eObjState state)
{
    char temp[255], text[255];
    cStr str;

    gPropObjState->Set(objID, state);

    if (state == kObjStateBroken)
    {
        // feedback text
        AutoAppIPtr(GameStrings);
        str = pGameStrings->FetchObjString(objID,PROP_OBJSHORTNAME_NAME);
        DPCStringFetch(temp,sizeof(temp),"WeaponBreaks","misc");
        sprintf(text,temp,str);
        DPCOverlayAddText(text, DEFAULT_MSG_TIME);

        // feedback audio
        ESndPlay(&cTagSet("Event Breaking"), objID, OBJ_NULL); 
    }
}

static sPropertyTypeDesc objStateTypeDesc = {PROP_OBJ_STATE_NAME, sizeof(int)}; 

/////////////////////////////////////////////////////////////
// Software property
/////////////////////////////////////////////////////////////

static char *software_names[] =
{ "PDA", "Hack", "Modify", "Repair"};

IIntProperty* gPropSoftwareLevel;
IIntProperty* gPropSoftwareType;

static sPropertyDesc SoftwareLevelDesc =
{
    PROP_SOFTWARELEVEL_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Software Level"},
};

static sPropertyDesc SoftwareTypeDesc =
{
    PROP_SOFTWARETYPE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Software Type"},
};

static sFieldDesc softwareTypeFields[] = 
{
    { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 5, 5, software_names},
};

static sStructDesc softwareTypeStructDesc = 
{
    PROP_SOFTWARETYPE_NAME, 
    sizeof(int),
    kStructFlagNone,
    sizeof(softwareTypeFields)/sizeof(softwareTypeFields[0]),
    softwareTypeFields,
}; 

static sPropertyTypeDesc softwareTypeTypeDesc = {PROP_SOFTWARETYPE_NAME, sizeof(int)}; 

/////////////////////////////////////////////////////////////
IFloatProperty *gPropDelayTime = NULL;

static sPropertyDesc DelayTimeDesc =
{
    PROP_DELAYTIME_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Delay Time"},
};
/////////////////////////////////////////////////////////////

IEcologyInfoProperty *gPropEcology = NULL;

static sPropertyDesc EcologyDesc =
{
    PROP_ECOLOGY_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Ecology"},
};

IIntProperty *gPropEcoType = NULL;

static sPropertyDesc EcoTypeDesc =
{
    PROP_ECOTYPE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "EcoType"},
};

IIntProperty *gPropEcoState = NULL;

static sPropertyDesc EcoStateDesc =
{
    PROP_ECOSTATE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "EcoState"},
};

/////////////////////////////////////////////////////////////

ISpawnInfoProperty *gPropSpawn = NULL;

static sPropertyDesc SpawnDesc =
{
    PROP_SPAWN_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Spawn"},
};

/////////////////////////////////////////////////////////////

IVectorProperty *gPropShove = NULL;

static sPropertyDesc ShoveDesc =
{
    PROP_SHOVE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Shove"},
};

/////////////////////////////////////////////////////////////

IStringProperty *gPropHUDUse = NULL;

static sPropertyDesc HUDUseDesc =
{
    PROP_HUDUSE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "HUD Use String"},
};

/////////////////////////////////////////////////////////////

IStringProperty *gPropWorldCursor = NULL;

static sPropertyDesc WorldCursorDesc =
{
    PROP_WORLDCURSOR_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Cursor", "World Cursor String"},
};

IStringProperty *gPropInvCursor = NULL;

static sPropertyDesc InvCursorDesc =
{
    PROP_INVCURSOR_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Cursor", "Inv Cursor String"},
};

IStringProperty *gPropUseCursor = NULL;

static sPropertyDesc UseCursorDesc =
{
    PROP_USECURSOR_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Cursor", "Use Cursor String"},
};

static char *use_names[] =
{ "Generic", "Ammo", "Tech"};

IIntProperty *gPropUseType = NULL;

static sPropertyDesc UseTypeDesc =
{
    PROP_USETYPE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Cursor", "Use Type"},
};

static sFieldDesc useTypeFields[] = 
{
    { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 3, 3, use_names},
};

static sStructDesc useTypeStructDesc = 
{
    PROP_USETYPE_NAME, 
    sizeof(int),
    kStructFlagNone,
    sizeof(useTypeFields)/sizeof(useTypeFields[0]),
    useTypeFields,
}; 

static sPropertyTypeDesc useTypeTypeDesc = {PROP_USETYPE_NAME, sizeof(int)}; 

/////////////////////////////////////////////////////////////
IStringProperty *gPropConsumeType = NULL;

static sPropertyDesc ConsumeTypeDesc =
{
    PROP_CONSUMETYPE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Consume Type"},
};

/////////////////////////////////////////////////////////////
IStringProperty *gPropMetapropType = NULL;

static sPropertyDesc MetapropTypeDesc =
{
    PROP_METAPROPTYPE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Metaprop Type"},
};

/////////////////////////////////////////////////////////////
IStringProperty *gPropObjList = NULL;

static sPropertyDesc ObjListDesc =
{
    PROP_OBJLIST_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Objlist Arg"},
};

/////////////////////////////////////////////////////////////
IStringProperty *gPropSignalType = NULL;

static sPropertyDesc SignalTypeDesc =
{
    PROP_SIGNALTYPE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Signal Type"},
};

/////////////////////////////////////////////////////////////

IStringProperty *gPropQBName = NULL;

static sPropertyDesc QBNameDesc =
{
    PROP_QBNAME_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "QB Name"},
};

IIntProperty *gPropQBVal = NULL;

static sPropertyDesc QBValDesc =
{
    PROP_QBVAL_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "QB Val"},
};

/////////////////////////////////////////////////////////////
IIntProperty *gPropShakeAmt = NULL;

static sPropertyDesc ShakeAmtDesc =
{
    "ShakeAmt", 0, 
    NULL, 0, 0, // constraints, versions
    { "Script", "Shake Strength"},
};

/////////////////////////////////////////////////////////////
IMapRefProperty *gPropMapRef = NULL;

static sPropertyDesc MapRefDesc =
{
    "MapRef", 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Map Ref Info"},
};


/////////////////////////////////////////////////////////////
/* ------------------------------------------------------------ */
// Tech Info (H,R,M) property description

// data ops
class cTechInfoDataOps: public cClassDataOps<sTechInfo>
{
};

// storage class
class cTechInfoStore: public cHashPropertyStore<cTechInfoDataOps>
{
};

// property implementation class
class cTechInfoProperty: public cSpecificProperty<ITechInfoProperty, &IID_ITechInfoProperty, sTechInfo*, cTechInfoStore>
{
    typedef cSpecificProperty<ITechInfoProperty, &IID_ITechInfoProperty, sTechInfo*, cTechInfoStore> cParent; 

public:
    cTechInfoProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sTechInfo); 

};

ITechInfoProperty *g_TechInfoProperty = NULL;

static sFieldDesc TechInfoFields[] = 
{
    {"Success %", kFieldTypeInt, FieldLocation(sTechInfo, m_success),},
    {"Critical Fail %", kFieldTypeInt, FieldLocation(sTechInfo, m_critfail),},
    {"Cost", kFieldTypeFloat, FieldLocation(sTechInfo, m_cost),},
};

static sStructDesc TechInfoStructDesc = 
StructDescBuild(sTechInfo, kStructFlagNone, TechInfoFields);

ITechInfoProperty *CreateTechInfoProperty(sPropertyDesc *desc, 
                                          ePropertyImpl impl)
{
    StructDescRegister(&TechInfoStructDesc);
    return new cTechInfoProperty(desc);
}

/* ------------------------------------------------------------ */
// Map Reference marker data

// data ops
class cMapRefDataOps: public cClassDataOps<sMapRef>
{
};

// storage class
class cMapRefStore: public cHashPropertyStore<cMapRefDataOps>
{
};

// property implementation class
class cMapRefProperty: public cSpecificProperty<IMapRefProperty, &IID_IMapRefProperty, sMapRef*, cMapRefStore>
{
    typedef cSpecificProperty<IMapRefProperty, &IID_IMapRefProperty, sMapRef*, cMapRefStore> cParent; 

public:
    cMapRefProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sMapRef); 

};

IMapRefProperty *g_MapRefProperty = NULL;

static sFieldDesc MapRefFields[] = 
{
    {"Map X", kFieldTypeInt, FieldLocation(sMapRef, m_x),},
    {"Map Y", kFieldTypeInt, FieldLocation(sMapRef, m_y),},
    {"Frame", kFieldTypeInt, FieldLocation(sMapRef, m_frame),},
};

static sStructDesc MapRefStructDesc = 
StructDescBuild(sMapRef, kStructFlagNone, MapRefFields);

IMapRefProperty *CreateMapRefProperty(sPropertyDesc *desc, 
                                      ePropertyImpl impl)
{
    StructDescRegister(&MapRefStructDesc);
    return new cMapRefProperty(desc);
}
/* ------------------------------------------------------------ */
// Loot Info property description

// data ops
class cLootInfoDataOps: public cClassDataOps<sLootInfo>
{
};

// storage class
class cLootInfoStore: public cHashPropertyStore<cLootInfoDataOps>
{
};

// property implementation class
class cLootInfoProperty: public cSpecificProperty<ILootInfoProperty, &IID_ILootInfoProperty, sLootInfo*, cLootInfoStore>
{
    typedef cSpecificProperty<ILootInfoProperty, &IID_ILootInfoProperty, sLootInfo*, cLootInfoStore> cParent; 

public:
    cLootInfoProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sLootInfo); 

};

ILootInfoProperty *g_LootInfoProperty = NULL;

static sFieldDesc LootInfoFields[] = 
{
    {"Number of Picks", kFieldTypeInt, FieldLocation(sLootInfo, m_numpicks),},
    {"1 - Item", kFieldTypeString, FieldLocation(sLootInfo, m_items[0]),},
    {"1 - Rarity", kFieldTypeInt, FieldLocation(sLootInfo, m_rarity[0]),},
    //{"1 - Value", kFieldTypeFloat, FieldLocation(sLootInfo, m_value[0]),},
    {"2 - Item", kFieldTypeString, FieldLocation(sLootInfo, m_items[1]),},
    {"2 - Rarity", kFieldTypeInt, FieldLocation(sLootInfo, m_rarity[1]),},
    //{"2 - Value", kFieldTypeFloat, FieldLocation(sLootInfo, m_value[1]),},
    {"3 - Item", kFieldTypeString, FieldLocation(sLootInfo, m_items[2]),},
    {"3 - Rarity", kFieldTypeInt, FieldLocation(sLootInfo, m_rarity[2]),},
    //{"3 - Value", kFieldTypeFloat, FieldLocation(sLootInfo, m_value[2]),},
    {"4 - Item", kFieldTypeString, FieldLocation(sLootInfo, m_items[3]),},
    {"4 - Rarity", kFieldTypeInt, FieldLocation(sLootInfo, m_rarity[3]),},
    //{"4 - Value", kFieldTypeFloat, FieldLocation(sLootInfo, m_value[3]),},
    {"5 - Item", kFieldTypeString, FieldLocation(sLootInfo, m_items[4]),},
    {"5 - Rarity", kFieldTypeInt, FieldLocation(sLootInfo, m_rarity[4]),},
    //{"5 - Value", kFieldTypeFloat, FieldLocation(sLootInfo, m_value[4]),},
    {"6 - Item", kFieldTypeString, FieldLocation(sLootInfo, m_items[5]),},
    {"6 - Rarity", kFieldTypeInt, FieldLocation(sLootInfo, m_rarity[5]),},
    //{"6 - Value", kFieldTypeFloat, FieldLocation(sLootInfo, m_value[5]),},
};

static sStructDesc LootInfoStructDesc = 
StructDescBuild(sLootInfo, kStructFlagNone, LootInfoFields);

ILootInfoProperty *CreateLootInfoProperty(sPropertyDesc *desc, 
                                          ePropertyImpl impl)
{
    StructDescRegister(&LootInfoStructDesc);
    return new cLootInfoProperty(desc);
}

/////////////////////////////////////////////////////////////

IStringProperty* gPropSettingText1 = NULL;
IStringProperty* gPropSettingText2 = NULL;
IStringProperty* gPropSettingHead1 = NULL;
IStringProperty* gPropSettingHead2 = NULL;

static sPropertyDesc SettingText1Desc =
{
    PROP_SETTINGTEXT1_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Gun", "Setting Text 1"},
};

static sPropertyDesc SettingText2Desc =
{
    PROP_SETTINGTEXT2_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Gun", "Setting Text 2"},
};

static sPropertyDesc SettingHead1Desc =
{
    PROP_SETTINGHEAD1_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Gun", "Setting Header 1"},
};

static sPropertyDesc SettingHead2Desc =
{
    PROP_SETTINGHEAD2_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Gun", "Setting Header 2"},
};

/////////////////////////////////////////////////////////////
IFloatProperty *gPropHackVisibility = NULL;
static sPropertyDesc HackVisibilityDesc =
{
    PROP_HACKVISIBILITY_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Player", "Hack Visibility"},
};

/////////////////////////////////////////////////////////////
IFloatProperty *gPropEnergy = NULL;
static sPropertyDesc EnergyDesc =
{
    PROP_ENERGY_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Energy", "Stored Energy"},
};

IFloatProperty *gPropDrainRate = NULL;
static sPropertyDesc DrainRateDesc =
{
    PROP_DRAINRATE_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Energy", "Drain Rate"},
};

IFloatProperty *gPropDrainAmt = NULL;
static sPropertyDesc DrainAmtDesc =
{
    PROP_DRAINAMT_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj: Energy", "Drain Amount"},
};

/////////////////////////////////////////////////////////////

IIntProperty *gPropTripFlags = NULL;
static sPropertyDesc tripFlagsDesc = 
{
    PROP_TRIPFLAGS_NAME, 0,
    NULL, 0, 0, 
    { "Script", "Trap Control Flags"},
};

char *tripflag_names[] =
{ "Enter", "Exit", "Mono", "Once", "Invert", "Player", "Alarm", "Shove", "ZapInside", "EasterEgg 1"};

static sFieldDesc tripFlagsFields[] = 
{
    { "", kFieldTypeBits, sizeof(int), 0, kFieldFlagUnsigned, 0, 10, 10, tripflag_names},
};

static sStructDesc tripFlagsStructDesc = 
{
    PROP_TRIPFLAGS_NAME, 
    sizeof(int),
    kStructFlagNone,
    sizeof(tripFlagsFields)/sizeof(tripFlagsFields[0]),
    tripFlagsFields,
}; 

static sPropertyTypeDesc tripFlagsTypeDesc = {PROP_TRIPFLAGS_NAME, sizeof(int)}; 

/* ------------------------------------------------------------ */
// Ecology Info property description

// data ops
class cEcologyInfoDataOps: public cClassDataOps<sEcologyInfo>
{
};

// storage class
class cEcologyInfoStore: public cHashPropertyStore<cEcologyInfoDataOps>
{
};

// property implementation class
class cEcologyInfoProperty: public cSpecificProperty<IEcologyInfoProperty, &IID_IEcologyInfoProperty, sEcologyInfo*, cEcologyInfoStore>
{
    typedef cSpecificProperty<IEcologyInfoProperty, &IID_IEcologyInfoProperty, sEcologyInfo*, cEcologyInfoStore> cParent; 

public:
    cEcologyInfoProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sEcologyInfo); 

};

IEcologyInfoProperty *g_EcologyInfoProperty = NULL;

static sFieldDesc EcologyInfoFields[] = 
{
    {"Period",      kFieldTypeFloat, FieldLocation(sEcologyInfo, m_period),},
    {"Normal Min",  kFieldTypeInt, FieldLocation(sEcologyInfo, m_mincount[0]),},
    {"Normal Max",  kFieldTypeInt, FieldLocation(sEcologyInfo, m_maxcount[0]),},
    {"Normal Rand", kFieldTypeInt, FieldLocation(sEcologyInfo, m_randval[0]),},
    {"Alert Min",       kFieldTypeInt,   FieldLocation(sEcologyInfo, m_mincount[2]),},
    {"Alert Max",       kFieldTypeInt,   FieldLocation(sEcologyInfo, m_maxcount[2]),},
    {"Alert Recovery",  kFieldTypeFloat, FieldLocation(sEcologyInfo, m_recovery[2]),},
    {"Alert Rand",      kFieldTypeInt,   FieldLocation(sEcologyInfo, m_randval[2]),},
};

static sStructDesc EcologyInfoStructDesc = 
StructDescBuild(sEcologyInfo, kStructFlagNone, EcologyInfoFields);

IEcologyInfoProperty *CreateEcologyInfoProperty(sPropertyDesc *desc, 
                                                ePropertyImpl impl)
{
    StructDescRegister(&EcologyInfoStructDesc);
    return new cEcologyInfoProperty(desc);
}

/////////////////////////////////////////////////////////////
// Spawn Info property description

// data ops
class cSpawnInfoDataOps: public cClassDataOps<sSpawnInfo>
{
};

// storage class
class cSpawnInfoStore: public cHashPropertyStore<cSpawnInfoDataOps>
{
};

// property implementation class
class cSpawnInfoProperty: public cSpecificProperty<ISpawnInfoProperty, &IID_ISpawnInfoProperty, sSpawnInfo*, cSpawnInfoStore>
{
    typedef cSpecificProperty<ISpawnInfoProperty, &IID_ISpawnInfoProperty, sSpawnInfo*, cSpawnInfoStore> cParent; 

public:
    cSpawnInfoProperty(const sPropertyDesc* desc)
    : cParent(desc)
    {
    }

    STANDARD_DESCRIBE_TYPE(sSpawnInfo); 

};

ISpawnInfoProperty *g_SpawnInfoProperty = NULL;

static char *flagBits[] = 
{
    "PopLimit",
    "PlrDist",
    "GotoLoc",
    "SelfMarker",
    "Raycast",
    "Farthest",
};

static sFieldDesc SpawnInfoFields[] = 
{
    {"Type 1",   kFieldTypeString,  FieldLocation(sSpawnInfo, m_objs[0]),},
    {"Rarity 1", kFieldTypeInt,     FieldLocation(sSpawnInfo, m_odds[0]),},
    {"Type 2",   kFieldTypeString,  FieldLocation(sSpawnInfo, m_objs[1]),},
    {"Rarity 2", kFieldTypeInt,     FieldLocation(sSpawnInfo, m_odds[1]),},
    {"Type 3",   kFieldTypeString,  FieldLocation(sSpawnInfo, m_objs[2]),},
    {"Rarity 3", kFieldTypeInt,     FieldLocation(sSpawnInfo, m_odds[2]),},
    {"Type 4",   kFieldTypeString,  FieldLocation(sSpawnInfo, m_objs[3]),},
    {"Rarity 4", kFieldTypeInt,     FieldLocation(sSpawnInfo, m_odds[3]),},
    {"Flags",    kFieldTypeBits,    FieldLocation(sSpawnInfo, m_flags),kFieldFlagNone, 0, 6, 6, flagBits,},
    {"Supply",   kFieldTypeInt,     FieldLocation(sSpawnInfo, m_supply),},
};

static sStructDesc SpawnInfoStructDesc = 
StructDescBuild(sSpawnInfo, kStructFlagNone, SpawnInfoFields);

ISpawnInfoProperty *CreateSpawnInfoProperty(sPropertyDesc *desc, 
                                            ePropertyImpl impl)
{
    StructDescRegister(&SpawnInfoStructDesc);
    return new cSpawnInfoProperty(desc);
}

/////////////////////////////////////////////////////////////
// Automatically Pickupable object property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropAutoPickup;

static sPropertyDesc AutoPickupDesc =
{
    PROP_AUTOPICKUP_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "AutoPickup?"},
};

/////////////////////////////////////////////////////////////
// Is-Binoculars object property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropIsBinoculars;

static sPropertyDesc IsBinocularsDesc =
{
    PROP_IS_BINOCULARS_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Is Binoculars?"},
};

/////////////////////////////////////////////////////////////
// Is-PlayerCamera object property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropIsPlayerCamera;

static sPropertyDesc IsPlayerCameraDesc =
{
    PROP_IS_PLAYERCAMERA_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Is Player Camera?"},
};

/////////////////////////////////////////////////////////////
// Is-CameraTarget object property
/////////////////////////////////////////////////////////////

IBoolProperty* gPropIsCameraTarget;

static sPropertyDesc IsCameraTargetDesc =
{
    PROP_IS_CAMERATARGET_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Is Camera Target?"},
};

/////////////////////////////////////////////////////////////
// Plot critical object (prevent corpse deletion)
/////////////////////////////////////////////////////////////

IBoolProperty* gPropPlotCritical;

static sPropertyDesc plotCriticalDesc =
{
    PROP_PLOTCRITICAL_NAME, 0, 
    NULL, 0, 0, // constraints, versions
    { "Obj", "Plot Critical?"},
};

/////////////////////////////////////////////////////////////
// Transluce Rate property
/////////////////////////////////////////////////////////////

static sPropertyDesc transluceRateDesc =
{
    PROP_TRANSLUCE_RATE, 0, 
    NULL, 0, 0, // constraints, versions
    { "Renderer", "Transluce Rate"},
};

/////////////////////////////////////////////////////////////
// Setup the Deep Cover Properties.
/////////////////////////////////////////////////////////////
void DPCPropertiesInit(void)
{
    AutoAppIPtr(GameStrings); 

    gPropObjName = CreateStringProperty(&ObjNameDesc,OBJ_NAME_IMPL);
    pGameStrings->RegisterProp(PROP_OBJNAME_NAME,gPropObjName);   

    StructDescRegister(&NameTypeStructDesc);
    gPropObjNameType = CreateIntegralProperty(&NameTypeDesc, &NameTypeTypeDesc, kPropertyImplHash);

    gPropObjShortName = CreateStringProperty(&ObjShortNameDesc,kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_OBJSHORTNAME_NAME,gPropObjShortName);   

    // AI Frob Property creation.
    gPropAIFrob = CreateStringProperty(&AIFrobDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_AIFROB, gPropAIFrob);

    gPropObjLookString = CreateStringProperty(&ObjLookStringDesc,kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_OBJLOOKSTRING_NAME,gPropObjLookString);   

    gPropExp           = CreateIntProperty(&ExpDesc,EXP_IMPL);
    gPropObjIcon       = CreateLabelProperty(&ObjIconDesc,OBJ_ICON_IMPL);
    gPropObjBrokenIcon = CreateLabelProperty(&ObjBrokenIconDesc,kPropertyImplHash);

    gPropMapRef = CreateMapRefProperty(&MapRefDesc, kPropertyImplHash);

    gPropHackDiff    = CreateTechInfoProperty(&HackDiffDesc,    HACKDIFF_IMPL);
    gPropRepairDiff  = CreateTechInfoProperty(&RepairDiffDesc,  REPAIRDIFF_IMPL);
    gPropModifyDiff  = CreateTechInfoProperty(&ModifyDiffDesc,  MODIFYDIFF_IMPL);
    gPropModify2Diff = CreateTechInfoProperty(&Modify2DiffDesc, MODIFY2DIFF_IMPL);

    gPropModify1Text = CreateStringProperty(&Modify1TextDesc,MODIFY1TEXT_IMPL);
    pGameStrings->RegisterProp(PROP_MODIFY1TEXT_NAME, gPropModify1Text);
    gPropModify2Text = CreateStringProperty(&Modify2TextDesc,MODIFY2TEXT_IMPL);
    pGameStrings->RegisterProp(PROP_MODIFY2TEXT_NAME, gPropModify2Text);

    gPropHackText = CreateStringProperty(&HackTextDesc, HACKTEXT_IMPL);
    pGameStrings->RegisterProp(PROP_HACKTEXT_NAME, gPropHackText);

    gPropUseMessage = CreateStringProperty(&UseMessageDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_USEMESSAGE_NAME,gPropUseMessage);   
    gPropMessageTime = CreateIntProperty(&MessageTimeDesc, kPropertyImplHash);

    gPropStackIncrem = CreateIntProperty(&StackIncremDesc, kPropertyImplHash);

    gPropLockedMessage = CreateStringProperty(&LockedMessageDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_LOCKEDMESSAGE_NAME,gPropLockedMessage);   

    gPropHUDTime        = CreateIntProperty(&HUDTimeDesc,    kPropertyImplHash);
    gPropAllowHUDSelect = CreateBoolProperty(&HUDSelectDesc, kPropertyImplHash);
    gPropShowHP         = CreateBoolProperty(&ShowHPDesc,    kPropertyImplHash);
    gPropHackTime       = CreateIntProperty(&HackTimeDesc,   kPropertyImplHash);

    gPropAlarm     = CreateBoolProperty(&AlarmDesc,     kPropertyImplHash);
    gPropBlockFrob = CreateBoolProperty(&BlockFrobDesc, kPropertyImplHash);

    StructDescRegister(&objStateStructDesc);
    gPropObjState = CreateIntegralProperty(&objStateDesc, &objStateTypeDesc, kPropertyImplHash);

    StructDescRegister(&softwareTypeStructDesc);
    gPropSoftwareType  = CreateIntegralProperty(&SoftwareTypeDesc, &softwareTypeTypeDesc, kPropertyImplHash);
    gPropSoftwareLevel = CreateIntProperty(&SoftwareLevelDesc,     kPropertyImplHash);

    gPropDelayTime = CreateFloatProperty(&DelayTimeDesc, kPropertyImplHash);

    gPropLoot                 = CreateLootInfoProperty(&LootDesc,LOOT_IMPL);
    gPropGuaranteedLoot       = CreateLabelProperty(&GuaranteedLootDesc,kPropertyImplLlist);
    gPropReallyGuaranteedLoot = CreateLabelProperty(&ReallyGuaranteedLootDesc,kPropertyImplLlist);

    gPropMapObjIcon   = CreateLabelProperty(&MapObjIconDesc,kPropertyImplHash);
    gPropMapObjRotate = CreateBoolProperty(&MapObjRotateDesc,kPropertyImplHash);
    gPropMapText      = CreateStringProperty(&MapTextDesc,kPropertyImplHash);

    gPropSettingText1 = CreateStringProperty(&SettingText1Desc,kPropertyImplHash);
    gPropSettingText2 = CreateStringProperty(&SettingText2Desc,kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_SETTINGTEXT1_NAME,gPropSettingText1);   
    pGameStrings->RegisterProp(PROP_SETTINGTEXT2_NAME,gPropSettingText2);   

    gPropSettingHead1 = CreateStringProperty(&SettingHead1Desc,kPropertyImplHash);
    gPropSettingHead2 = CreateStringProperty(&SettingHead2Desc,kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_SETTINGHEAD1_NAME,gPropSettingHead1);   
    pGameStrings->RegisterProp(PROP_SETTINGHEAD2_NAME,gPropSettingHead2);   

    gPropEcology  = CreateEcologyInfoProperty(&EcologyDesc, kPropertyImplHash);
    gPropEcoType  = CreateIntProperty(&EcoTypeDesc,         kPropertyImplHash);
    gPropEcoState = CreateIntProperty(&EcoStateDesc,        kPropertyImplHash);

    gPropSpawn = CreateSpawnInfoProperty(&SpawnDesc, kPropertyImplHash);

    gPropHackVisibility = CreateFloatProperty(&HackVisibilityDesc, kPropertyImplHash);
    gPropShove = CreateVectorProperty(&ShoveDesc, kPropertyImplHash);

    gPropHUDUse = CreateStringProperty(&HUDUseDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_HUDUSE_NAME, gPropHUDUse);

    gPropWorldCursor = CreateStringProperty(&WorldCursorDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_WORLDCURSOR_NAME, gPropWorldCursor);
    gPropInvCursor = CreateStringProperty(&InvCursorDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_INVCURSOR_NAME, gPropInvCursor);
    gPropUseCursor = CreateStringProperty(&UseCursorDesc, kPropertyImplHash);
    pGameStrings->RegisterProp(PROP_USECURSOR_NAME, gPropUseCursor);

    StructDescRegister(&useTypeStructDesc);
    gPropUseType = CreateIntegralProperty(&UseTypeDesc, &useTypeTypeDesc, kPropertyImplHash);
    gPropAutoPickup = CreateBoolProperty(&AutoPickupDesc, kPropertyImplHash);
    gPropIsBinoculars = CreateBoolProperty(&IsBinocularsDesc, kPropertyImplHash);
    gPropIsPlayerCamera = CreateBoolProperty(&IsPlayerCameraDesc, kPropertyImplHash);
    gPropIsCameraTarget = CreateBoolProperty(&IsCameraTargetDesc, kPropertyImplHash);

    gPropEnergy    = CreateFloatProperty(&EnergyDesc,    kPropertyImplHash);
    gPropDrainRate = CreateFloatProperty(&DrainRateDesc, kPropertyImplHash);
    gPropDrainAmt  = CreateFloatProperty(&DrainAmtDesc,  kPropertyImplHash);

    gPropConsumeType  = CreateStringProperty(&ConsumeTypeDesc,  kPropertyImplHash);
    gPropSignalType   = CreateStringProperty(&SignalTypeDesc,   kPropertyImplHash);
    gPropMetapropType = CreateStringProperty(&MetapropTypeDesc, kPropertyImplHash);
    gPropObjList      = CreateStringProperty(&ObjListDesc,      kPropertyImplHash);

    gPropQBName = CreateStringProperty(&QBNameDesc, kPropertyImplHash);
    gPropQBVal  = CreateIntProperty(&QBValDesc,     kPropertyImplHash);

    gPropShakeAmt = CreateIntProperty(&ShakeAmtDesc, kPropertyImplSparseHash);

    gPropRecycle = CreateIntProperty(&RecycleDesc, kPropertyImplHash);

    StructDescRegister(&tripFlagsStructDesc);
    gPropTripFlags = CreateIntegralProperty(&tripFlagsDesc, &tripFlagsTypeDesc, kPropertyImplHash);

    StructDescRegister(&miniGamesStructDesc);
    gPropMiniGames = CreateIntegralProperty(&MiniGamesDesc, &miniGamesTypeDesc, kPropertyImplLlist);

    gPropPlotCritical = CreateBoolProperty(&plotCriticalDesc, kPropertyImplHash);

    CreateIntProperty(&transluceRateDesc, kPropertyImplHash);
}

/////////////////////////////////////////////////////////////
// Cleanup our Deep Cover properties.
/////////////////////////////////////////////////////////////
void DPCPropertiesShutdown(void)
{
   SafeRelease(gPropAIFrob);

   // In Shock the following weren't released.  I have no idea why.
   SafeRelease(gPropObjName);
   SafeRelease(gPropObjNameType);
   SafeRelease(gPropObjShortName);
   SafeRelease(gPropAIFrob);
   SafeRelease(gPropObjLookString);
   SafeRelease(gPropExp);
   SafeRelease(gPropObjIcon);
   SafeRelease(gPropObjBrokenIcon);
   SafeRelease(gPropMapRef);
   SafeRelease(gPropHackDiff);
   SafeRelease(gPropRepairDiff);
   SafeRelease(gPropModifyDiff);
   SafeRelease(gPropModify2Diff);
   SafeRelease(gPropModify1Text);
   SafeRelease(gPropModify2Text);
   SafeRelease(gPropHackText);
   SafeRelease(gPropUseMessage);
   SafeRelease(gPropMessageTime);
   SafeRelease(gPropStackIncrem);
   SafeRelease(gPropLockedMessage);
   SafeRelease(gPropHUDTime);
   SafeRelease(gPropAllowHUDSelect);
   SafeRelease(gPropShowHP);
   SafeRelease(gPropHackTime);
   SafeRelease(gPropAlarm);
   SafeRelease(gPropBlockFrob);
   SafeRelease(gPropObjState);
   SafeRelease(gPropSoftwareType);
   SafeRelease(gPropSoftwareLevel);
   SafeRelease(gPropDelayTime);
   SafeRelease(gPropLoot);
   SafeRelease(gPropGuaranteedLoot);
   SafeRelease(gPropReallyGuaranteedLoot);
   SafeRelease(gPropMapObjIcon);
   SafeRelease(gPropMapObjRotate);
   SafeRelease(gPropMapText);
   SafeRelease(gPropSettingText1);
   SafeRelease(gPropSettingText2);
   SafeRelease(gPropSettingHead1);
   SafeRelease(gPropSettingHead2);
   SafeRelease(gPropEcology);
   SafeRelease(gPropEcoType);
   SafeRelease(gPropEcoState);
   SafeRelease(gPropSpawn);
   SafeRelease(gPropHackVisibility);
   SafeRelease(gPropShove);
   SafeRelease(gPropHUDUse);
   SafeRelease(gPropWorldCursor);
   SafeRelease(gPropInvCursor);
   SafeRelease(gPropUseCursor);
   SafeRelease(gPropUseType);
   SafeRelease(gPropAutoPickup);
   SafeRelease(gPropIsBinoculars);
   SafeRelease(gPropIsPlayerCamera);
   SafeRelease(gPropIsCameraTarget);
   SafeRelease(gPropEnergy);
   SafeRelease(gPropDrainRate);
   SafeRelease(gPropDrainAmt);
   SafeRelease(gPropConsumeType);
   SafeRelease(gPropSignalType);
   SafeRelease(gPropMetapropType);
   SafeRelease(gPropObjList);
   SafeRelease(gPropQBName);
   SafeRelease(gPropQBVal);
   SafeRelease(gPropShakeAmt);
   SafeRelease(gPropRecycle);
   SafeRelease(gPropTripFlags);
   SafeRelease(gPropMiniGames);
   SafeRelease(gPropPlotCritical);
}
