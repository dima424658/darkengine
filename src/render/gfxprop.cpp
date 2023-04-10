/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/gfxprop.cpp,v 1.3 2000/02/19 12:35:23 toml Exp $

/*=========================================================

  Created:  3/24/99 3:01:21 PM

  File:  gfxprop.cpp

  Description:  


=========================================================*/



//_____INCLUDES_AND_DEFINITIONS___________________________


#include <propert_.h>
#include <propface.h>
#include <gfxprop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


//______LOCAL_DATA________________________________________


// Z-Bias property

static IIntProperty*    gZBiasProperty = NULL;

static sPropertyDesc    sZBiasPropDesc = 
{
    Z_BIAS_PROPERTY_NAME,
        0,//kPropertyNoInherit|kPropertyInstantiate,
        NULL, 0, 0, 
    { "Renderer", "Z-Bias" }, 
};


// Bump Mapping property

static IBoolProperty*   gBumpProperty = NULL;

static sPropertyDesc    sBumpPropDesc = 
{
    BUMP_PROPERTY_NAME,
        kPropertyNoInherit|kPropertyInstantiate,
        NULL, 0, 0, 
    { "Renderer", "Bump Map" }, 
};



//____LOCAL_PROTOTYPES____________________________________





//__________CODE__________________________________________


// Z-Bias property

void ZBiasPropertyInit(void)
{
   gZBiasProperty = CreateIntProperty(&sZBiasPropDesc, kPropertyImplDense);
}



BOOL ObjGetZBiasProp( ObjID obj, int* psl )
{
    if( gZBiasProperty )
    {
        BOOL bGotIt = gZBiasProperty->Get( obj, psl );
        return bGotIt;
    }
    return FALSE;
}


BOOL ObjSetZBiasProp( ObjID obj, int sl )
{
    if( gZBiasProperty )
    {
        if( sl < 0 ) 
            sl = 0;
        else if (sl > 16 )
            sl = 16;

        BOOL bGotIt = gZBiasProperty->Set( obj, sl );
        return bGotIt;
    }
    return FALSE;
}


// Bump Mapping property

void BumpPropertyInit(void)
{
   gBumpProperty = CreateBoolProperty(&sBumpPropDesc, kPropertyImplDense);
}


BOOL ObjGetBumpProp( ObjID obj )
{
    if( gBumpProperty )
    {
        BOOL bBump = FALSE;
        gBumpProperty->Get( obj, &bBump );
        return bBump;
    }
    return FALSE;
}

BOOL ObjSetBumpProp( ObjID obj, BOOL bBump )
{
    if( gBumpProperty )
    {
        BOOL bGotIt = gBumpProperty->Set( obj, bBump );
        return bGotIt;
    }
    return FALSE;
}


// eof:  gfxprop.cpp