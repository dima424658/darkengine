/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////
// Player Gun Animation
//

#include <dpcpgapi.h>

#include <cfgdbg.h>
#include <config.h>
#include <schema.h>

#include <dpcplayr.h>
#include <dpcplcst.h>
#include <gunflash.h>
#include <gunprop.h>
#include <gunapi.h>
#include <dpcganpr.h>
#include <gunanim.h>
#include <dpcparam.h>
#include <dpctrcst.h>

#include <stdlib.h>

#include <appagg.h>
#include <aggmemb.h>

#include <matrix.h>
#include <matrixs.h>
#include <fix.h>
#include <mprintf.h>
#include <mouse.h>

#include <plyrmode.h>   // for gun wobble

#include <iobjsys.h>
#include <traitman.h>
#include <mnamprop.h>
#include <osysbase.h>
#include <objpos.h>
#include <playrobj.h>
#include <rand.h>
#include <rendprop.h>
#include <portal.h>
#include <physapi.h>
#include <esnd.h>
#include <ctagset.h>
#include <scrnmode.h>

#include <dpcplayr.h>
#include <dpcplayr.h>
#include <dpcctrl.h>
#include <dpcobjst.h>
#include <dpcparam.h>
#include <gunproj.h>
#include <dpcprop.h>
#include <gunvhot.h>

#include <dpcgame.h>
#include <dpcinv.h>
#include <dpccam.h> // for zoom

// delayed burst
#include <autolink.h>
#include <dmgmodel.h>
#include <linkman.h>
#include <relation.h>

// network joy
#include <netman.h>
#include <ghostapi.h>

extern "C"
{
#include <camera.h>
}

// Include these absolutely last
#include <dbmem.h>
#include <initguid.h>
DEFINE_LG_GUID(IID_IPlayerGun, 0x18d);

// where should I really get these from?
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define max(a,b)  (((a) > (b)) ? (a) : (b))

// basic gun states
enum ePlayerGunState
{
    kPlayerGunOff, kPlayerGunLowered, kPlayerGunRaising, kPlayerGunRaised, 
    kPlayerGunLowering, kPlayerGunPoweringUp, kPlayerGunFiring, kPlayerGunPoweringDown,
    kPlayerGunResetting, kPlayerGunStartingLoad, kPlayerGunFinishingLoad, kPlayerGunLoading, 
    kPlayerGunDelaying
};
enum ePlayerGunSwingState
{
    kPlayerGunNotSwing, kPlayerGunSwingStart, kPlayerGunSwingLeft, kPlayerGunSwingRight,
};
enum ePlayerGunFlags
{
    kPlayerGunTriggerPulled = 0x0001, kPlayerGunTriggerReleased = 0x0002, kPlayerGunOverloadStarted = 0x0004
};

// is this right?
const float kRandMax = 32767;

class cPlayerGun:
public cCTDelegating<IPlayerGun>, 
public cCTAggregateMemberControl<kCTU_Default>
{
public:
    cPlayerGun(IUnknown* pOuter):
    m_lastFireTime(0),
    m_flags(0)
    {
        MI_INIT_AGGREGATION_1(pOuter, IPlayerGun, kPriorityNormal, NULL);
    }

    STDMETHOD(Init)(THIS)
    {
        m_handID = NULL;
        m_state = kPlayerGunOff;
        m_gunID = OBJ_NULL;
        return S_OK;
    }

    STDMETHOD(GameInit)(THIS)
    {
        m_pObjSys = AppGetObj(IObjectSystem);
        return S_OK;
    }
    STDMETHOD(GameTerm)(THIS)
    {
        SafeRelease(m_pObjSys);
        m_pObjSys = NULL;
        return S_OK;
    }
    STDMETHOD(Set)(THIS_ ObjID gunID);
    STDMETHOD(SetRaised)(THIS_ ObjID gunID);
    STDMETHOD_(ObjID, Get)(THIS)
    {
        return m_gunID;
    }
    STDMETHOD_(ObjID, GetHand)(THIS)
    {
        return m_handID;
    }
    STDMETHOD(Off)(THIS);
    STDMETHOD(Frame)(tSimTime deltaTime);
    STDMETHOD(Raise)(THIS);
    STDMETHOD(Lower)(THIS);
    STDMETHOD(Unequip) (THIS_ ObjID oldGun, ObjID newGun, BOOL spew, BOOL sednToInv);
    STDMETHOD(ReleaseTrigger)(THIS);
    STDMETHOD(PullTrigger)(THIS);
    STDMETHOD(Load)(void);
    STDMETHOD_(BOOL, IsReloading)(THIS) const
    {
        return((m_state == kPlayerGunStartingLoad) || 
               (m_state == kPlayerGunLoading) || 
               (m_state == kPlayerGunFinishingLoad));
    }
    STDMETHOD_(BOOL, IsTriggerPulled)(THIS) const
    {
        return(m_flags&kPlayerGunTriggerPulled);
    }
    STDMETHOD_ (BOOL, MatchesCurrentAmmo)(THIS_ ObjID clipID)
    {
        if (m_gunID == OBJ_NULL)
            return FALSE;
        return(GetProjectile(m_gunID) == GetProjectileFromClip(clipID, m_gunID));
    }


private:
    ObjID m_handID; // ID of the object we are using to represent the gun in front of the player
    ObjID m_gunID;  // Actual gun object
    ePlayerGunState m_state;
    int m_flags;
    ePlayerGunSwingState m_swingState;
    tSimTime m_swingStartTime;
    tSimTime m_lastFireTime;
    tSimTime m_stateChangeTime;   // time at which we last changed state
    tSimTime m_burstNum; // how many shots we've fired in current burst
    mxs_ang m_heading;
    mxs_ang m_pitch;
    mxs_ang m_pitchTarget;  // where we want pitch to be
    mxs_ang m_pitchRate;    // rate at which we're going there
    float m_bob;
    float m_bobTarget;      // where we want bob to be
    float m_kickBack;       // current kick pack offset
    IObjectSystem *m_pObjSys;
    sGunAnimsState m_animState; // anim state

    // cached property data
    sGunAnim *m_pGunAnim;   // anim controls from property
    sPlayerGunDesc *m_pPlayerGunDesc;
    eWeaponSkills m_weaponType;   
    ObjID m_unequippingGun;
    ObjID m_nextGun;
    BOOL m_unequippingSpew;
    BOOL m_unequippingSendToInv;

    // internal functions
    void Create(void);
    void SetState(ePlayerGunState state);
    void UpdateState(void);
    void SetControls(void);
    void SetSwingState(ePlayerGunSwingState state);
    void UpdateObjPos(void);
    void IntegrateControls(tSimTime delta);
    void Fire(void);

    void Animate(void);
    BOOL Animating(void);
    void SetJoint(int jointNum, float jointPos);
    void StartJointAnimation(const sGunJointAnim &jointAnim);
    void StartAnimation(void);
    void EndAnimation(void);
    BOOL AnimateJoint(const sGunJointAnim &jointAnim);
    BOOL AnimateParameter(float *pValue, sGunAnimState *pState, const sGunParamAnim &params);

    mxs_ang CalcKickAngle(mxs_ang theta, int flags);
    void BoundAbs(mxs_ang &pTheta, mxs_ang bound);
    void BoundAbs(float &pTheta, float bound);
    void ApplyKick(mxs_ang pitch, mxs_ang heading, sGunKick *pGunKick, float pct);
    void ApplyJolt(mxs_ang pitch, mxs_ang heading, float joltBack, float pct);
    mxs_ang CalcRandAngle(void) const;
    BOOL TestForBreakage(void);
    void StartFiringSequence(void);
    void LaunchProjectile(mxs_ang pitch, mxs_ang heading);

    void LinkProjectile(ObjID projID);
    void ExplodeProjectile(void);

    BOOL ShouldWobble(void);
};

//////////////////////////////////////////
// Decide if gun should wobble
//
BOOL cPlayerGun::ShouldWobble(void)
{
    if (((g_pPlayerMode->GetMode() == kPM_Stand) || (g_pPlayerMode->GetMode() == kPM_Crouch)) && PhysObjOnGround(PlayerObject()))
    {
        mxs_vector velocity;
        PhysGetVelocity(PlayerObject(), &velocity);
        mxs_real velocity_mag = mx_mag_vec(&velocity);

        return(velocity_mag>GetGunAnimParams()->m_wobbleSpeed);
    }
    return FALSE;
};

////////////////////////////////////////
// Set the current gun
//
STDMETHODIMP cPlayerGun::Set(ObjID gunID)
{
    Off();

    // interpret Set to OBJ_NULL as Off
    if (gunID == OBJ_NULL)
        return S_OK;

    m_gunID = gunID;
    m_pGunAnim = NULL;
    if (!PlayerGunDescGet(gunID, &m_pPlayerGunDesc))
    {
        Warning(("No player gun description for obj %d\n", gunID));
        Off();
        return E_FAIL;
    }
#if 0
    if (!g_pWeaponTypeProperty->Get(gunID, (int*)&m_weaponType))
    {
        Warning(("No weapon type for obj %d\n", gunID));
        // default to conventional
        m_weaponType = kWeaponConventional;
    }
#endif
    // For DC, all weapons "conventional"
    m_weaponType = kWeaponConventional;
    Create();
    Raise();
    UpdateObjPos();
    // play reload sound (== activate sound)
    cTagSet eventTags("Event Reload");
    int schemaHandle = ESndPlay(&eventTags, gunID, OBJ_NULL);

    // tell ghost we are using the gun.... (ack, why do we have to do this twice?)
    GhostSetWeapon(PlayerObject(),gunID,FALSE);

    return S_OK;
}

////////////////////////////////////////
// Set the current gun
//
STDMETHODIMP cPlayerGun::SetRaised(ObjID gunID)
{
    Off();

    // interpret Set to OBJ_NULL as Off
    if (gunID == OBJ_NULL)
        return S_OK;

    m_gunID = gunID;
    m_pGunAnim = NULL;
    if (!PlayerGunDescGet(gunID, &m_pPlayerGunDesc))
    {
        Warning(("No player gun description for obj %d\n", gunID));
        Off();
        return E_FAIL;
    }
#if 0
    if (!g_pWeaponTypeProperty->Get(gunID, (int*)&m_weaponType))
    {
        Warning(("No weapon type for obj %d\n", gunID));
        // default to conventional
        m_weaponType = kWeaponConventional;
    }
#endif
    // For DC, all weapons "conventional"
    m_weaponType = kWeaponConventional;
    Create();
    m_pitch = GetGunAnimParams()->m_raisedPitch;
    SetState(kPlayerGunRaised);
    UpdateObjPos();

    // tell ghost we are using the gun.... (ack, why do we have to do this twice?)
    GhostSetWeapon(PlayerObject(),gunID,FALSE);

    return S_OK;
}

////////////////////////////////////////
// Raise the gun
//
void cPlayerGun::Create(void)
{
    Label *pHandModelName;
    float zeroJoints[MAX_REND_JOINTS] = {0, 0, 0, 0, 0, 0};
    sGunAnimParams *pAnimParams = GetGunAnimParams();

    Assert_(m_handID==OBJ_NULL);
    Assert_(m_gunID!=OBJ_NULL);

    // create the gun model object
    m_handID = m_pObjSys->BeginCreate(ROOT_ARCHETYPE, kObjectConcrete);
    m_pObjSys->NameObject(m_handID, "Player Gun");
    if (PlayerGunDescGetHandModel(m_gunID, &pHandModelName))
        ObjSetModelName(m_handID, (char*)pHandModelName);
    m_pObjSys->SetObjTransience(m_handID, TRUE);
    ObjSetHasRefs(m_handID, FALSE);
    ObjSetJointPos(m_handID, zeroJoints);
    m_pObjSys->EndCreate(m_handID);

    m_flags = 0;
    m_animState.m_flags = 0;
    m_bobTarget = 0;
    m_bob = 0;
    m_heading = 0;
    m_pitch = pAnimParams->m_loweredPitch;
    m_pitchTarget = pAnimParams->m_raisedPitch;
    m_pitchRate = pAnimParams->m_raisePitchRate;
    m_kickBack = 0;
}

////////////////////////////////////////
// Raise the gun
//
STDMETHODIMP cPlayerGun::Raise(void)
{
    SetState(kPlayerGunRaising);
    return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::Lower(void)
{
    SetState(kPlayerGunLowering);
    return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::Unequip(ObjID oldGun, ObjID newGun, BOOL spew, BOOL sendToInv)
{
    // @TODO: make this work
    m_unequippingGun = oldGun;
    m_nextGun = newGun;
    m_unequippingSpew = spew;
    m_unequippingSendToInv = sendToInv; // FALSE if from drag/drop
    Lower();
    return S_OK;
}

////////////////////////////////////////

void cPlayerGun::SetSwingState(ePlayerGunSwingState state)
{
    m_swingState = state;
    m_swingStartTime = GetSimTime();
}

////////////////////////////////////////

void cPlayerGun::SetState(ePlayerGunState state)
{
    switch (m_state)
    {
        case kPlayerGunRaised:
            SetSwingState(kPlayerGunNotSwing);
            break;
    }

    m_state = state;
    m_stateChangeTime = GetSimTime();

    switch (m_state)
    {
        case kPlayerGunLowered:
            Off();
            if (m_unequippingGun) 
            {
               AutoAppIPtr(DPCPlayer);
               pDPCPlayer->Equip(PlayerObject(), kEquipWeapon, 
                                                 m_nextGun,m_unequippingSpew);
               if (m_unequippingSendToInv) {
                  DPCInvAddObj(PlayerObject(), m_unequippingGun, 1);
               }
               m_unequippingGun = m_nextGun = OBJ_NULL;
            }
            break;
        case kPlayerGunRaised:
            m_burstNum = 0;

            {
            float zoom = BaseGunDescGetZoom(m_gunID);
            ZoomTarget(zoom, 2.0);
            }

            break;
        case kPlayerGunPoweringUp:
        case kPlayerGunPoweringDown:
            // Post firing animation
            if (g_gunAnimPostProperty->Get(m_gunID, &m_pGunAnim))
                StartAnimation();
            break;
        case kPlayerGunResetting:
            // Post firing animation
            if (g_gunAnimPostProperty->Get(m_gunID, &m_pGunAnim))
                StartAnimation();
            break;
        case kPlayerGunStartingLoad:
            m_pitchTarget = m_pPlayerGunDesc->m_reloadPitch;
            m_pitchRate = m_pPlayerGunDesc->m_reloadRate;
            break;
        case kPlayerGunFinishingLoad:
            m_pitchTarget = GetGunAnimParams()->m_raisedPitch;
            m_pitchRate = m_pPlayerGunDesc->m_reloadRate;
            break;
    }
}

////////////////////////////////////////

inline void cPlayerGun::SetJoint(int jointNum, float jointPos)
{
    float *pJoints;

    // this is stupid
    pJoints = ObjJointPos(m_handID);
    pJoints[jointNum] = jointPos;
}

////////////////////////////////////////

void cPlayerGun::StartJointAnimation(const sGunJointAnim &jointAnim)
{
    SetJoint(jointAnim.m_num, jointAnim.m_params.m_target2);
}

////////////////////////////////////////

inline BOOL cPlayerGun::Animating(void)
{
    return(m_animState.m_flags)&kGunAnimating;
}

////////////////////////////////////////

void cPlayerGun::StartAnimation(void)
{
    Assert_(m_pGunAnim != NULL);
    m_animState.m_startTime = GetSimTime();
    StartJointAnimation(m_pGunAnim->m_jointAnim);
    m_animState.m_flags = kGunAnimating;
    m_animState.m_jointAnimState.m_flags = 0;
}

void cPlayerGun::EndAnimation(void)
{
    m_animState.m_flags &= ~kGunAnimating;
    m_pGunAnim = NULL;
}

////////////////////////////////////////
// Integrate a given parameter value towards a target, specified in the anim params
// Sets values in the gun joint state structure
BOOL cPlayerGun::AnimateParameter(float *pValue, sGunAnimState *pState, const sGunParamAnim &params)
{
    float elapsedTime;
    float target;
    float rate;
    float value;

    // up/down anim
    if ((pState->m_flags)&kGunAnimReversed)
    {
        rate = params.m_rate2/1000;
        target = params.m_target2;
        elapsedTime = GetSimTime()-pState->m_reverseTime;
        value = params.m_target1+rate*elapsedTime;
    }
    else
    {
        rate = params.m_rate1/1000;
        target = params.m_target1;
        elapsedTime = GetSimTime()-m_animState.m_startTime;
        value = params.m_target2+rate*elapsedTime;
    }
    if (((rate<0) && (value<=target)) || ((rate>0) && (value>=target)))
    {
        *pValue = target;
        // check if we want to go to target 2 or not
        if (!(params.m_flags&kGunAnimTarget2) || ((pState->m_flags)&kGunAnimReversed))
            return TRUE;
        else
        {
            pState->m_flags |= kGunAnimReversed;
            pState->m_reverseTime = GetSimTime();
        }
        return FALSE;
    }
    *pValue = value;
    return FALSE;
}

////////////////////////////////////////
// Returns TRUE when animation completed
//
BOOL cPlayerGun::AnimateJoint(const sGunJointAnim &jointAnim)
{
    float *pJoints;

    pJoints = ObjJointPos(m_handID);
    if (AnimateParameter(&pJoints[0], &m_animState.m_jointAnimState, m_pGunAnim->m_jointAnim.m_params))
        return TRUE;
    return FALSE;
}

////////////////////////////////////////
// Enact animation & detect end
//
void cPlayerGun::Animate(void)
{
    Assert_(m_pGunAnim != NULL);
    // currently we only have one animation to do!
    if (AnimateJoint(m_pGunAnim->m_jointAnim))
        EndAnimation();
}

////////////////////////////////////////

mxs_ang cPlayerGun::CalcKickAngle(mxs_ang theta, int flags)
{
    AutoAppIPtr(DPCPlayer);
    // adjust for agility
    // MAX_STAT_VAL better be more than 1...
    theta = mxs_ang(float(theta)*(float(MAX_STAT_VAL-pDPCPlayer->GetStat(kStatAgility))/float(MAX_STAT_VAL-1)));
    if ((flags&kPlayerGunKickUp) && (flags&kPlayerGunKickDown))
        if (Rand()>(kRandMax/2))
            flags = kPlayerGunKickUp;
        else
            flags = kPlayerGunKickDown;
    if (flags&kPlayerGunKickUp)
        return theta*(0.5+float(Rand())/(2*kRandMax));
    else if (flags&kPlayerGunKickDown)
        return theta*(-0.5-float(Rand())/(2*kRandMax));
    else
        return 0;
}

////////////////////////////////////////

void cPlayerGun::ApplyJolt(mxs_ang pitch, mxs_ang heading, float joltBack, float pct)
{
    mxs_angvec ang;
    mxs_vector vel;
    mxs_vector pos;
    mxs_matrix mat;

    // Apply pitch jolt
    PhysGetSubModRotation(PlayerObject(), PLAYER_HEAD, &ang);
    ang.ty += pitch*pct;
    // Apply heading jolt
    ang.tz += heading*pct;
    PosPropLock++;
    PhysicsListenerLock = TRUE;
    PhysSetSubModRotation(PlayerObject(), PLAYER_HEAD, &ang);
    PhysicsListenerLock = FALSE;
    PosPropLock--;

    // Apply jolt back
    PhysGetVelocity(PlayerObject(), &vel);
    CameraGetLocation(PlayerCamera(), &pos, &ang);
    mx_ang2mat(&mat, &ang);
    mx_scale_addeq_vec(&vel, &mat.vec[0], joltBack);
    PhysSetVelocity(PlayerObject(), &vel);
}

////////////////////////////////////////

void cPlayerGun::BoundAbs(mxs_ang &theta, mxs_ang bound)
{
    if (bound<0)
        theta = max(theta, bound);
    else
        theta = min(theta, bound);
}

////////////////////////////////////////

void cPlayerGun::BoundAbs(float &theta, float bound)
{
    if (bound<0)
        theta = max(theta, bound);
    else
        theta = min(theta, bound);
}

////////////////////////////////////////

void cPlayerGun::ApplyKick(mxs_ang pitch, mxs_ang heading, sGunKick *pGunKick, float pct)
{
    m_pitch += pitch*pct;
    BoundAbs(m_pitch, pGunKick->m_kickPitchMax);
    m_pitchRate = pGunKick->m_kickAngularReturnRate;
    m_heading += heading*pct;
    m_kickBack += pGunKick->m_kickBack*pct;
    BoundAbs(m_kickBack, pGunKick->m_kickBackMax);
    UpdateObjPos();
}

////////////////////////////////////////

inline mxs_ang cPlayerGun::CalcRandAngle(void) const
{
    AutoAppIPtr(DPCPlayer);
    mxs_ang randAngle = GetSkillParams()->m_inaccuracy*(MAX_SKILL_VAL-pDPCPlayer->GetWeaponSkill(m_weaponType));
    // adjust for traits
    /*
    if ((m_pPlayerGunDesc->m_handedness == kPlayerGunTwoHanded) && 
       pDPCPlayer->HasTrait(PlayerObject(), kTraitRifleman))
    {
       ConfigSpew("PlayerShotSpew", ("Rifleman: Adjusting inaccuracy from %x to %x\n", randAngle, mxs_ang(randAngle*kTraitRiflemanKickFrac)));
       randAngle *= kTraitRiflemanKickFrac;
    }
    else if ((m_pPlayerGunDesc->m_handedness == kPlayerGunOneHanded) && 
       pDPCPlayer->HasTrait(PlayerObject(), kTraitOneHand))
    {
       ConfigSpew("PlayerShotSpew", ("OneHanded: Adjusting inaccuracy from %x to %x\n", randAngle, mxs_ang(randAngle*kTraitOneHandKickFrac)));
       randAngle *= kTraitOneHandKickFrac;
    }
    */
    return randAngle;
}

/////////////////////////////////////////////////////////
// Check to see if gun breaks. TRUE iff gun breaks.
// Should only be called with state normal.
// 

BOOL cPlayerGun::TestForBreakage(void)
{
    sGunReliability *pGunReliability;
    BOOL gunBreaks = FALSE;

    if (ObjGetObjState(m_gunID) != kObjStateNormal)
        return FALSE;
    if (g_pGunReliabilityProperty->Get(m_gunID, &pGunReliability))
    {
        AutoAppIPtr(DPCPlayer);
        float condition = GunGetCondition(m_gunID);
        if (condition < pGunReliability->m_threshBreak)
        {
            float minBreak = pGunReliability->m_minBreak/100;
            float maxBreak = pGunReliability->m_maxBreak/100;
            int weapSkill = pDPCPlayer->GetWeaponSkill(m_weaponType);
            float breakChance = (minBreak+(1-(condition-pGunReliability->m_threshBreak)/100)*(maxBreak-minBreak))*(1-(GetSkillParams()->m_breakModifier*weapSkill));

            if ((float(Rand())/kRandMax)<breakChance)
            {
                gunBreaks = TRUE;
                ObjSetObjState(m_gunID, kObjStateBroken);
#if 0
                // Spoken "broken" alert
                SchemaPlay((Label *)"bb04",NULL);         
#endif
                cTagSet tagSet("Event Break");
                ESndPlay(&tagSet, m_gunID, OBJ_NULL); 
            }
        }
        float degradeRate = pGunReliability->m_degradeRate;
        float degradeMod;
        if (config_get_float("gun_degrade_rate", &degradeMod))
        {
           degradeRate *= degradeMod;
        }
        /*
        // maintenance affecting degredation rate removed in favor
        // of the new "wrench" system. -- Xemu, 2/15/99
        int maintSkill = pDPCPlayer->GetTechSkill(kTechMaintenance);
        if (maintSkill>0)
           degradeRate *= (1.0-(1.0/(7.0-float(maintSkill))));
        */
        if (condition>degradeRate)
        {
            ConfigSpew("DegradeSpew", ("Gun %d degrades by %g to %g\n", m_gunID, degradeRate, condition-degradeRate));
            condition -= degradeRate;
        }
        else
        {
            ConfigSpew("DegradeSpew", ("Gun %d degrades to zero\n", m_gunID));
            condition = 0;
        }

        GunSetCondition(m_gunID, condition);

        // Implement degrading of gun's special powers
        float degradeRate2 = pGunReliability->m_silenceDegradeRate;
        if (degradeRate2) 
        {
           float silenceVal = GunGetSilencing(m_gunID);
           if (silenceVal>degradeRate2)
           {
               ConfigSpew("DegradeSpew", ("Gun %d alt-degrades by %g to %g\n", m_gunID, degradeRate2, silenceVal-degradeRate2));
               silenceVal -= degradeRate2;
           }
           else
           {
               ConfigSpew("DegradeSpew", ("Gun %d alt-degrades to zero\n", m_gunID));
               silenceVal = 0;
           }
           GunSetSilencing(m_gunID, silenceVal);
        }

    }
    return gunBreaks;
}

////////////////////////////////////////

void cPlayerGun::LaunchProjectile(mxs_ang pitch, mxs_ang heading)
{
    ObjID projID;
    ObjID launchedID;
    sBaseGunDesc *pBaseGunDesc;

    if (!BaseGunDescGet(m_gunID, &pBaseGunDesc))
    {
        Warning(("No base gun description for obj %d\n", m_gunID));
        return;
    }

    // find projectile
    // @TODO: cache proj ID
    if ((projID = GetProjectile(m_gunID)) != OBJ_NULL)
    {
        // calc modifiers
        mxs_ang randAngle = CalcRandAngle();
        AutoAppIPtr(DPCPlayer);

        mxs_ang cursor_pitch, cursor_heading;
        cursor_pitch = cursor_heading = 0;
        if (DPC_mouse)
        {
            float dx, dy, MCH, MCP;
            short mpx, mpy;
            sScrnMode scm;

            mouse_get_xy(&mpx, &mpy);
            ScrnModeGet(&scm);

            dx = -1.0 * (float)(mpx - (scm.w / 2)) / (float)(scm.w / 2);
            dy = -1.0 * (float)(mpy - (scm.h / 2)) / (float)(scm.h / 2);

            MCH = 10000;
            MCP = 7000;
            config_get_float("cursor_heading",&MCH);
            config_get_float("cursor_pitch",&MCP);

            cursor_heading = (mxs_ang)(dx * MCH); 
            cursor_pitch = (mxs_ang)(dy * MCP); 
            //mprintf("cursor deviation = %x, %x\n",cursor_heading, cursor_pitch);
        }

        // stim intensity
        float stimModifier = pBaseGunDesc->m_stimModifier;
        // weapon skill
        stimModifier *= pDPCPlayer->GetStimMultiplier(m_gunID);
        // Lucky shot trait
        if (pDPCPlayer->HasTrait(PlayerObject(), kTraitSharpshooter))
            stimModifier *= GetTraitParams()->m_lethalMult;

        // Launch projectiles
        if (GunStateGetAmmo(m_gunID)>=pBaseGunDesc->m_ammoUsage)
        {
            sLaunchParams launchParams = g_defaultLaunchParams;

            ConfigSpew("PlayerShotSpew", ("Launching projectiles, objID %d\n", projID));
            // setup valid loc (fall back proj start location) as player head position...
            launchParams.flags = kLaunchTellAI|kLaunchLocOverride|kLaunchPitchOverride;
            // setup remainder of launch parameters
            launchParams.speedMult = pBaseGunDesc->m_speedModifier;
            launchParams.intensityMult = stimModifier;
//         VHotGetOffset(&launchParams.offset, m_handID, 0);
//         mx_addeq_vec(&launchParams.offset, &m_pPlayerGunDesc->m_fireOffset);
            launchParams.loc = PlayerCamera()->pos;
            launchParams.pitch = PlayerCamera()->ang.ty-pitch - cursor_pitch;
            launchParams.heading = heading + cursor_heading;
            launchParams.error = randAngle;
            // actually launch the damn things
            launchedID = GunLaunchProjectile(PlayerObject(), projID, &launchParams);
            // dec ammo
            GunStateSetAmmo(m_gunID, GunStateGetAmmo(m_gunID)-pBaseGunDesc->m_ammoUsage);
#if 0
            // Spoken out-of-ammo alert
            if (GunStateGetAmmo(m_gunID) == 0)
            {
                SchemaPlay((Label *)"bb08",NULL);         
            }
#endif
        }
        // link the projectile if we are delayed burst
        if ((m_pPlayerGunDesc->m_flags)&kPlayerGunDelayedExplosion)
            LinkProjectile(launchedID);

        // play firing sound
        cTagSet tagSet("Event Shoot");
        // @TODO: make this the real weapon mode
        tagSet.Add(cTag("WeaponMode", int(0)));

        float condition = GunGetSilencing(m_gunID);
        tagSet.Add(cTag("DegradeValue", int(condition/10)));
        // Don't network the sound for all players
        sSchemaCallParams params = g_sDefaultSchemaCallParams;
        params.flags |= SCH_NO_NETWORK;
        ObjPos *pHandPos = ObjPosGet(m_handID);
        AutoAppIPtr(NetManager);
#if 0
        if (pNetManager->Networking())
            ESndPlayLoc(&tagSet, m_gunID, projID, &(pHandPos->loc.vec), &params);
        else
            ESndPlay(&tagSet, m_gunID, projID);
#endif
        // DPCHACK Porges -- this sort of abuses the sound card but is
        // here so gun fire (as opposed to bullet impact) will propagate
        // to AIs.  Probably wants another routine to prop the sound w/o
        // playing it out loud or something.
        ESndPlayLoc(&tagSet, m_gunID, projID, &(pHandPos->loc.vec), &params);

    }
    // update state
    ++m_burstNum;
    if ((m_flags&kPlayerGunTriggerPulled) && 
        ((m_burstNum<pBaseGunDesc->m_burst) || (pBaseGunDesc->m_burst<0)) &&
        (GunStateGetAmmo(m_gunID)>=pBaseGunDesc->m_ammoUsage))
        SetState(kPlayerGunResetting);
    else
        SetState(kPlayerGunPoweringDown);
}

///////////////////////////////////////////

void cPlayerGun::Fire(void)
{
    ITraitManager *pTraitMan = AppGetObj(ITraitManager);
    cMxsVector fireOffset;
    sGunKick *pGunKick;
    GunKickGetSafe(m_gunID, &pGunKick);
    
    mxs_ang pitch       = CalcKickAngle(pGunKick->m_kickPitch,    m_pPlayerGunDesc->m_flags);
    mxs_ang heading     = CalcKickAngle(pGunKick->m_kickHeading, (m_pPlayerGunDesc->m_flags) >> 2);
    mxs_ang joltPitch   = CalcKickAngle(pGunKick->m_joltPitch,   (m_pPlayerGunDesc->m_flags) >> 4);
    mxs_ang joltHeading = CalcKickAngle(pGunKick->m_joltHeading, (m_pPlayerGunDesc->m_flags) >> 6);

    // pre-shot kickback
    ApplyKick(pitch, heading, pGunKick, 1);
    // pre-shot jolt
    ApplyJolt(-joltPitch, joltHeading, pGunKick->m_joltBack, 1);

    // shoot
    // note - kick disabled for actual shot effect for the moment (probably for all time)
    // so just pass in zero for pitch & heading adj
    LaunchProjectile(0, 0);

    // create gun flash(es)
    AutoAppIPtr(TraitManager);
    CreateGunFlashes(pTraitManager->GetArchetype(m_gunID), m_handID);

    /*
    post-shot no longer used...
    // post-shot kickback
    ApplyKick(pitch, heading, pGunKick, 1-pGunKick->m_preKickPct);
    // post-shot jolt
    ApplyJolt(-joltPitch, joltHeading, pGunKick->m_joltBack, 1-pGunKick->m_preKickPct);
    */

    m_lastFireTime = GetSimTime();

    SafeRelease(pTraitMan);
    GhostNotify(PlayerObject(),kGhostStFiring);
}

////////////////////////////////////////

void cPlayerGun::UpdateState(void)
{
    sGunAnimParams *pAnimParams = GetGunAnimParams();

    switch (m_state)
    {
        case kPlayerGunPoweringUp:
            SetState(kPlayerGunFiring);
            UpdateState();
            break;
        case kPlayerGunFiring:
            if (ObjGetObjState(m_gunID) == kObjStateNormal)
            {
                if (!TestForBreakage())
                    Fire();
            }
            else
                SetState(kPlayerGunPoweringDown);
            break;
        case kPlayerGunPoweringDown:
            {
                sBaseGunDesc *pBaseGunDesc;
                BaseGunDescGetSafe(m_gunID, &pBaseGunDesc);
                if (GetSimTime()>=(m_lastFireTime+pBaseGunDesc->m_shotInterval))
                {
                    /*
                    cTagSet eventTags("Event Reset");
                    int schemaHandle = ESndPlay(&eventTags, m_gunID, OBJ_NULL); 
                    */
                    SetState(kPlayerGunRaised);
                }
            }
            break;
        case kPlayerGunResetting:
            {
                // @TODO: query anims here?
                sBaseGunDesc *pBaseGunDesc;
                BaseGunDescGetSafe(m_gunID, &pBaseGunDesc);
                if (GetSimTime()>=(m_lastFireTime+pBaseGunDesc->m_burstInterval))
                    SetState(kPlayerGunFiring);
            }
            break;
        case kPlayerGunRaising:
            if (m_pitch == pAnimParams->m_raisedPitch)
                SetState(kPlayerGunRaised);
            break;
        case kPlayerGunLowering:
            if (m_pitch == pAnimParams->m_loweredPitch)
                SetState(kPlayerGunLowered);
            break;
        case kPlayerGunRaised:
            if ((m_swingState == kPlayerGunNotSwing) && ShouldWobble())
                SetSwingState(kPlayerGunSwingStart);
            else if (!ShouldWobble())
                SetSwingState(kPlayerGunNotSwing);
            break;
        case kPlayerGunFinishingLoad:
            if (m_pitch == pAnimParams->m_raisedPitch)
                SetState(kPlayerGunRaised);
            break;
        case kPlayerGunStartingLoad:
            if (m_pitch == m_pPlayerGunDesc->m_reloadPitch)
                SetState(kPlayerGunLoading);
            break;
        case kPlayerGunLoading:
            {
                sBaseGunDesc *pBaseGunDesc;
                BaseGunDescGetSafe(m_gunID, &pBaseGunDesc);
                if (GetSimTime()>(m_stateChangeTime+pBaseGunDesc->m_reloadTime))
                {
                    GunLoadSound(m_gunID);
                    SetState(kPlayerGunFinishingLoad);
                }
            }
            break;
    }
} 

////////////////////////////////////////

float Integrate(float val, float targ, float rate, tSimTime delta)
{
    float diff = val-targ;
    float move = rate*delta/1000;
    if (diff<0)
    {
        if (diff<-1*move)
            return val+move;
    }
    else
    {
        if (diff>move)
            return val-move;
    }
    return targ;     
}

////////////////////////////////////////

mxs_ang IntegrateAng(mxs_ang val, mxs_ang targ, mxs_ang rate, tSimTime delta)
{
    if (val != targ)
    {
        mxs_ang move = fix_mul(rate, fix_from_float(float(delta)/1000.0));
        mxs_ang diff = targ-val;
        if (diff<MX_ANG_PI)
        {
            if (move>diff)
                return targ;
            else
                return val+move;
        }
        else
        {
            if (move>mxs_ang(-diff))
                return targ;
            else
                return val-move;
        }
    }
    return targ;
}

////////////////////////////////////////

void cPlayerGun::SetControls(void)
{
    sGunAnimParams *pAnimParams = GetGunAnimParams();

    switch (m_state)
    {
        case kPlayerGunRaised:
            if (m_swingState != kPlayerGunNotSwing)
            {
                int delta = GetSimTime()-m_swingStartTime;
                switch (m_swingState)
                {
                    case kPlayerGunSwingStart:
                        if (delta>(pAnimParams->m_swingPeriod)/2)
                            SetSwingState(kPlayerGunSwingRight);
                        delta = GetSimTime()-m_swingStartTime;
                        break;
                    case kPlayerGunSwingLeft:
                        if (delta>pAnimParams->m_swingPeriod)
                            SetSwingState(kPlayerGunSwingRight);
                        delta = GetSimTime()-m_swingStartTime;
                        break;
                    case kPlayerGunSwingRight:
                        if (delta>pAnimParams->m_swingPeriod)
                            SetSwingState(kPlayerGunSwingLeft);
                        delta = GetSimTime()-m_swingStartTime;
                        break;
                }
                switch (m_swingState)
                {
                    case kPlayerGunSwingStart:
                        m_heading = fix_mul(pAnimParams->m_swingAmplitude, fix_from_float(float(delta)/(float(pAnimParams->m_swingPeriod))));
                        break;
                    case kPlayerGunSwingLeft:
                        m_heading = fix_mul(pAnimParams->m_swingAmplitude, fix_from_float((float(delta)-float((pAnimParams->m_swingPeriod)/2))/float(pAnimParams->m_swingPeriod)));
                        break;
                    case kPlayerGunSwingRight:
                        m_heading = fix_mul(pAnimParams->m_swingAmplitude, fix_from_float((float((pAnimParams->m_swingPeriod)/2)-float(delta))/float(pAnimParams->m_swingPeriod)));
                        break;
                }
                if (m_bob==m_bobTarget)
                {
                    m_bobTarget = (float(Rand())/float(RAND_MAX))*pAnimParams->m_bobAmplitude;
                    ConfigSpew("BobSpew", ("Setting bob target to %g\n", m_bobTarget));
                }
            }
            break;
        case kPlayerGunRaising:
            m_pitchTarget = pAnimParams->m_raisedPitch;
            m_pitchRate = pAnimParams->m_raisePitchRate;
            break;
        case kPlayerGunLowering:
            m_pitchTarget = pAnimParams->m_loweredPitch;
            m_pitchRate = pAnimParams->m_raisePitchRate;
            break;
    }
}

////////////////////////////////////////

void cPlayerGun::IntegrateControls(tSimTime delta)
{
    sGunKick *pGunKick;
    sGunAnimParams *pAnimParams = GetGunAnimParams();

    // Heading
    m_heading = IntegrateAng(m_heading, 0, pAnimParams->m_swingReturn, delta);

    // Pitch
    m_pitch = IntegrateAng(m_pitch, m_pitchTarget, m_pitchRate, delta);

    // Bob
    ConfigSpew("BobSpew", ("Bob target is %g, delta %d, rate %g\n", m_bob, delta, pAnimParams->m_bobRate));
    ConfigSpew("BobSpew", ("Setting bob from %g", m_bob));
    if (m_bob<m_bobTarget)
        m_bob = min(m_bob+pAnimParams->m_bobRate*delta/1000, m_bobTarget);
    else if (m_bob>m_bobTarget)
        m_bob = max(m_bob-pAnimParams->m_bobRate*delta/1000, m_bobTarget);
    ConfigSpew("BobSpew", (" to %g\n", m_bob));

    // Kickback
    GunKickGetSafe(m_gunID, &pGunKick);
    m_kickBack = Integrate(m_kickBack, 0, pGunKick->m_kickBackReturnRate, delta);
}

////////////////////////////////////////

void cPlayerGun::UpdateObjPos(void)
{
    mxs_vector camPos;
    mxs_angvec camFacing;
    mxs_matrix m1, m2;
    mxs_vector offset, kickBackVec;

    if (m_handID == OBJ_NULL)
        return;

    // Get base location/facing
    CameraGetLocation(PlayerCamera(), &camPos, &camFacing);
    // turn player facing into matrix
    mx_ang2mat(&m1, &camFacing);

    // apply heading adjustment
    mx_rot_z_mat(&m2, &m1, m_heading);
    // apply pitch adjustment
    mx_rot_y_mat(&m1, &m2, -m_pitch);

    // calc facing vector
    mx_mat2ang(&camFacing, &m1);

    // calc offset
    mx_copy_vec(&offset, &(m_pPlayerGunDesc->m_posOffset));
    kickBackVec.x = m_kickBack;
    kickBackVec.y = 0;
    kickBackVec.z = 0;
    mx_addeq_vec(&offset, &kickBackVec);
    mx_mat_muleq_vec(&m1, &offset);
    mx_addeq_vec(&camPos, &offset);

    // bob
    camPos.z += m_bob;

    // update actual obj pos
    ObjPosUpdate(m_handID, &camPos, &camFacing);
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::Off(void)
{
    // destroy old model
    if (m_handID != OBJ_NULL)
    {
        m_pObjSys->Destroy(m_handID);
        m_handID = OBJ_NULL;
        SetState(kPlayerGunOff);
    }
    m_gunID = OBJ_NULL;
    m_flags = 0;
    GhostSetWeapon(PlayerObject(),m_gunID,FALSE);
    return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::Frame(tSimTime timeDelta)
{
    if (m_state != kPlayerGunOff)
    {
        if (Animating())
            Animate();

        if (!Animating())
            UpdateState();

        // UpdateState can kill the model
        if (m_handID == OBJ_NULL)
            return S_OK;

        SetControls();
        IntegrateControls(timeDelta);
        UpdateObjPos();
    }
    return S_OK;
}

////////////////////////////////////////

inline void cPlayerGun::StartFiringSequence(void)
{
    SetState(kPlayerGunPoweringUp);
    UpdateState();
}

////////////////////////////////////////
// 
// LinkProjectile: used by delayed blast weapons.
// We are over-loading the projectile link here, but what the hell.
// No one else should be linking projectiles to the player... or this will cause trouble
//
void cPlayerGun::LinkProjectile(ObjID projID)
{
    AutoAppIPtr(LinkManager);

    pLinkManager->Add(PlayerObject(), projID, g_pProjectileLinks->GetID());
}

////////////////////////////////////////
//
// Explode all linked projectiles.
//
void cPlayerGun::ExplodeProjectile(void)
{
    cAutoLinkQuery query(g_pProjectileLinks, PlayerObject(), LINKOBJ_WILDCARD);  
    AutoAppIPtr(DamageModel);
    AutoAppIPtr(LinkManager);
    ObjID projID;

    while (!query->Done())
    {
        projID = query.GetDest();
        pLinkManager->Remove(query->ID());
        pDamageModel->SlayObject(projID, PlayerObject());
        query->Next();
    }
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::PullTrigger(void)
{
    Assert_(!(m_flags&kPlayerGunTriggerPulled));
    m_flags |= kPlayerGunTriggerPulled;

    switch (m_state)
    {
        case kPlayerGunRaised:
            if (ObjGetObjState(m_gunID) == kObjStateNormal)
            {
                sBaseGunDesc *pBaseGunDesc;
                BaseGunDescGetSafe(m_gunID, &pBaseGunDesc);
                if (GunStateGetAmmo(m_gunID)>=pBaseGunDesc->m_ammoUsage)
                {
                    StartFiringSequence();
                    return S_OK;
                }
                else
                {
                    // play out of ammo sound
                    ESndPlay(&cTagSet("Event OutofAmmo"), m_gunID, OBJ_NULL); 
                }
            }
            else
            // @TODO: play broken noise here
            {
                ESndPlay(&cTagSet("Event Broken"), m_gunID, OBJ_NULL); 
            }
            break;
        default:
            return E_FAIL;
    }
    return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::ReleaseTrigger(void)
{
    m_flags &= ~kPlayerGunTriggerPulled;
    if ((m_pPlayerGunDesc->m_flags)&kPlayerGunDelayedExplosion)
    {
        ExplodeProjectile();
    }
    return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPlayerGun::Load(void)
{
    SetState(kPlayerGunStartingLoad);
    return S_OK;
}

////////////////////////////////////////

void DPCPlayerGunCreate(void)
{
    AutoAppIPtr(Unknown); 
    new cPlayerGun(pUnknown); 
}


