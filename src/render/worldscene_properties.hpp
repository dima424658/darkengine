#ifndef DARK_WORLDSCENE_PROPERTIES_HPP
#define DARK_WORLDSCENE_PROPERTIES_HPP
#include <algorithm>
// Values are valid only for the observed native property/hierarchy revision.
// Defaults are restored before reading, including after a property is removed.
struct WorldSceneMaterialProperties {
   unsigned long long revision=0;
   float alpha=1,selfIllum=0;
   bool castsShadow=true,selfIllumPresent=false;
   // Native MD treats the object value as a multiplier of authored material
   // illumination, not as an emissive level for every material.
   float MdIllumination(float material) const {
      return std::max(0.0f,material)*(selfIllumPresent?std::max(0.0f,selfIllum):1.0f);
   }
   template<class Reader> void Refresh(unsigned long long current,Reader read) {
      if(revision==current)return;
      float nextAlpha=1,nextIllum=0;
      bool nextShadow=true;read(nextAlpha,nextIllum,nextShadow);
      alpha=nextAlpha;selfIllum=nextIllum;castsShadow=nextShadow;revision=current;
   }
};
#endif
