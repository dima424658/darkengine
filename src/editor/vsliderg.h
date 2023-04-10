/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vsliderg.h,v 1.5 2000/01/29 13:13:30 adurant Exp $
// virtual slider generation garbage
#pragma once

#ifndef ___VSLIDER_H
#define ___VSLIDER_H

typedef struct {
   bool (*parse)(float inp, void *data, int act);
} anonSlider;

#define GenerateVSliderType(type)                     \
   typedef struct {                                   \
      bool (*parse)(float inp, void *data, int act);  \
      type *val;                                      \
      type lo,hi;                                     \
      type old_val;                                   \
      float scale;                                    \
      float extra;                                    \
   } ##type##Slider;                                  \
   EXTERN void *VSlider##type##Build(##type##Slider *s)

// these really are only for use by the actual vslider thing itself
#ifdef __VSLIDER_C
#define GenerateVSliderBuild(type)                    \
   void *VSlider##type##Build(##type##Slider *s)      \
   {                                                  \
      s->parse=VSlider##type##Parse;                  \
      s->scale=1.0;                                   \
      s->extra=0.0;                                   \
      return s;                                       \
   }


#define GenerateVSliderParse(type)                    \
   bool VSlider##type##Parse(float delta, ##type##Slider *s, int action) \
   {                                                  \
      switch (action)                                 \
      {                                               \
      case SLIDER_START:                              \
         s->old_val=*s->val;                          \
         return TRUE;  /* in case we have to abort */ \
      case SLIDER_SLIDE:                              \
         {                                            \
            type old=*s->val;                         \
            float coeff;                              \
            if (s->lo==s->hi)    /* infinite range */ \
            {                                         \
               coeff = s->scale;                      \
               *s->val+=delta*coeff + s->extra;       \
            }                                         \
            else                /* standard slider */ \
            {      /* 1/20 of range is sens 1 full */ \
               coeff = 0.1*(s->hi-s->lo)*s->scale;    \
               *s->val+=delta*coeff + s->extra;       \
               if (*s->val<s->lo) *s->val=s->lo, coeff = 0.0;      \
               else if (*s->val>s->hi) *s->val=s->hi, coeff = 0.0; \
            }                                         \
            if (coeff == 0.0) s->extra = 0.0;          \
            else s->extra = delta*coeff + s->extra - (float)(*s->val - old); \
            return *s->val!=old;                      \
         }                                            \
      case SLIDER_ABORT:                              \
         *s->val=s->old_val;                          \
         return TRUE;      /* auto restore old val */ \
      case SLIDER_END:                                \
            { bool retval = s->old_val!=*s->val;      \
              s->old_val = *s->val;                   \
              return retval;}                         \
      case SLIDER_UPDATE:                             \
         s->old_val = *s->val;                        \
         return FALSE;                                \
      }                                               \
      return FALSE;       /* should never get here */ \
   }
#else    // this is the ! part of __VSLIDER_C
#define GenerateVSliderBuild(type)
#define GenerateVSliderParse(type)
#endif   // __VSLIDER_C

#endif

