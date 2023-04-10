/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/quesreac.cpp,v 1.2 1998/10/05 17:27:28 mahk Exp $
#include <quesreac.h>
#include <reaction.h>
#include <appagg.h>
#include <reacbase.h>
#include <questapi.h>

#include <sdestool.h>
#include <sdesbase.h>

// Must be last header 
#include <dbmem.h>


//------------------------------------------------------------
// "Set quest bit" reaction
//

//
// The parameters
//

enum eQuestOp 
{
   kSet,
   kAdd,
   kMul,
   kDiv, 
   kOr, 
   kAnd, 
   kXOr, 

   kNumOps
}; 

struct sQVarParam
{
   char var[28]; 
   ushort op; 
   short val; 
};  // EXACTLY 32 bytes 

//
// The actual qvar setter
//

static void set_qvar(const char* var, uint op, int val)
{
   AutoAppIPtr(QuestData); 
   int qval = pQuestData->Get(var); 

   switch (op)
   {
      case kSet: qval = val; break; 
      case kAdd: qval += val; break;  
      case kMul: qval *= val; break; 
      case kDiv: qval /= val; break; 
      case kOr:  qval |= val; break; 
      case kAnd: qval &= val; break; 
      case kXOr: qval ^= val; break; 

      default: 
         Warning(("Unknown qvar op %d\n",op)); 
   }
   pQuestData->Set(var,qval); 
}

//
// The reaction
//

static eReactionResult LGAPI set_var_func(sReactionEvent* , const sReactionParam* param, tReactionFuncData )
{
   sQVarParam* qvp = (sQVarParam*)&param->data; 

   set_qvar(qvp->var,qvp->op,qvp->val); 
   return kReactionNormal; 
}

static sReactionDesc set_var_desc = 
{
   "SetQvar",
   "Set Quest Variable", 
   REACTION_PARAM_TYPE(sQVarParam),
}; 


//
// The sdesc
//

static const char* op_names[kNumOps] = 
{
   "Set To",
   "Add",
   "Multiply By",
   "Divide By",
   "Bitwise Or",
   "Bitwise And", 
   "Bitwise XOR", 
};

static sFieldDesc qvar_fields[] = 
{
   { "Quest Variable", kFieldTypeString, FieldLocation(sQVarParam,var), },
   { "Operation", kFieldTypeEnum, FieldLocation(sQVarParam,op), kFieldFlagNone, 
     0, kNumOps, kNumOps, op_names }, 
   { "Operand", kFieldTypeInt, FieldLocation(sQVarParam,val) }
};

static sStructDesc qvar_sdesc = StructDescBuild(sQVarParam,kStructFlagNone,qvar_fields); 

//
// Init/Term 
//

EXTERN void QuestReactionsInit()
{
   AutoAppIPtr(Reactions); 
   pReactions->Add(&set_var_desc,set_var_func,NULL); 
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&qvar_sdesc); 
}

EXTERN void QuestReactionsTerm()
{
   
}
