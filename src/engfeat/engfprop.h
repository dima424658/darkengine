/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/engfprop.h,v 1.2 2000/01/29 13:19:31 adurant Exp $
#pragma once

#ifndef __ENGFPROP_H
#define __ENGFPROP_H

#include <matrixs.h>

#include <property.h>
#include <propface.h>
////////////////////////////////////////

struct sBashParamsProp
{
   mxs_real threshold;
   mxs_real coeff;
};

#undef  INTERFACE
#define INTERFACE IBashParamsProp
DECLARE_PROPERTY_INTERFACE(IBashParamsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sBashParamsProp *);
};

#define BASH_FACTOR_PROP_NAME  "BashFactor"
#define BASH_PARAMS_PROP_NAME  "BashParams"

EXTERN IFloatProperty      *g_pBashFactorProp;
EXTERN IBashParamsProperty *g_pBashParamsProp;

#endif