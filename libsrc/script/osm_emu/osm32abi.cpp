// Guest ABI descriptions for legacy 32-bit Dark Engine OSM modules.

#include <osm_emu/osm32abi.h>

#include <stddef.h>

namespace
{

bool EqualAsciiNoCase(const char *left, const char *right)
{
   if (!left || !right) return false;
   while (*left && *right)
   {
      char a = *left++;
      char b = *right++;
      if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
      if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
      if (a != b) return false;
   }
   return *left == *right;
}

// Only ABI differences are listed. Unlisted slots retain their native slot.
const osm32::sServiceMethod kDark1Methods[] = {
   { osm32::kAbiServiceObject,   22, 24 },
   { osm32::kAbiServiceObject,   23, 25 },
   { osm32::kAbiServiceObject,   24, 26 },
   { osm32::kAbiServiceProperty,  8,  9 },
   { osm32::kAbiServiceProperty,  9, 10 },
   { osm32::kAbiServiceProperty, 10, 11 },
   { osm32::kAbiServiceProperty, 11, 12 },
   { osm32::kAbiServicePhysics,  10, 12 },
   { osm32::kAbiServicePhysics,  11, 13 },
   { osm32::kAbiServiceSound,     5,  7 },
   { osm32::kAbiServiceSound,     6,  6 },
   { osm32::kAbiServiceSound,     7,  5 },
   { osm32::kAbiServiceSound,     8,  8 },
   { osm32::kAbiServiceSound,     9, 11 },
   { osm32::kAbiServiceSound,    10, 10 },
   { osm32::kAbiServiceSound,    11,  9 },
   { osm32::kAbiServiceSound,    12, 12 },
   { osm32::kAbiServiceAI,        6,  7 },
   { osm32::kAbiServiceAI,        7,  6 }
};

struct sDark1ServiceLayout
{
   uint16_t id;
   uint8_t lastSlot;
};

// Dark 1 service layouts, recovered from the CodeView type records in the
// retail Thief Gold OSMs. Slots 0-2 are IUnknown and slots 3-4 are the
// inherited IAutoScriptService lifecycle methods. Services absent from the
// recovered type records are deliberately left unconstrained: absence is not
// evidence that a separately supplied TDP or Gold OSM could not request an
// extension service.
const sDark1ServiceLayout kDark1Services[] = {
   { 0x00d7,  7 }, // IDebugScriptService
   { 0x00da, 11 }, // IPropertyScriptService
   { 0x00df, 24 }, // IObjectScriptService
   { 0x00e5, 14 }, // IAIScriptService
   { 0x00ee, 15 }, // ILinkScriptService
   { 0x00ef,  9 }, // ILinkToolsScriptService
   { 0x00f1, 18 }, // ISoundScriptService
   { 0x00f4, 13 }, // IActReactScriptService
   { 0x00f6,  8 }, // IDoorScriptService
   { 0x00fb,  5 }, // ILockedScriptService
   { 0x00fd,  5 }, // IPuppetScriptService
   { 0x00fe,  7 }, // IDamageScriptService
   { 0x010d,  5 }, // IKeyScriptService
   { 0x010e,  9 }, // IWeaponScriptService
   { 0x0111, 10 }, // IPickLockScriptService
   { 0x0115, 11 }, // IBowScriptService
   { 0x0141, 11 }, // IPhysicsScriptService
   { 0x0150,  7 }, // IDrkInvScriptService
   { 0x0152,  9 }, // IQuestScriptService
   { 0x0153,  7 }, // IDrkPowerupsScriptService
   { 0x015d,  8 }, // IPlayerLimbsScriptService
   { 0x016a,  5 }, // IAnimTextureScriptService
   { 0x016c, 10 }, // ILightScriptService
   { 0x017d,  7 }, // IContainerScriptService
   { 0x019f, 11 }, // IDarkUIScriptService
   { 0x01a0, 10 }, // IDataScriptService
   { 0x01b4,  7 }, // IDarkGameScriptService
   { 0x01f8,  5 }  // IPGroupScriptService
};

const osm32::sServiceMethod kDark2Methods[] = {
   // The OSM SDK overload order differs from the native implementation.
   { osm32::kAbiServiceSound,  5,  7 },
   { osm32::kAbiServiceSound,  6,  6 },
   { osm32::kAbiServiceSound,  7,  5 },
   { osm32::kAbiServiceSound,  8,  8 },
   { osm32::kAbiServiceSound,  9, 11 },
   { osm32::kAbiServiceSound, 10, 10 },
   { osm32::kAbiServiceSound, 11,  9 },
   { osm32::kAbiServiceSound, 12, 12 },
   { osm32::kAbiServiceAI,     6,  7 },
   { osm32::kAbiServiceAI,     7,  6 }
};

} // namespace

namespace osm32
{

const char *GuestAbiName(eGuestAbi abi)
{
   return abi == kGuestAbiDark1 ? "Dark 1 (TDP/Gold)" : "Dark 2";
}

eGuestAbi GuestAbiFromProfile(const char *profile)
{
   return EqualAsciiNoCase(profile, "gold") ||
          EqualAsciiNoCase(profile, "tdp") ?
             kGuestAbiDark1 : kGuestAbiDark2;
}

bool ResolveServiceMethod(eGuestAbi abi, eAbiService service,
                          unsigned guestSlot, unsigned *nativeSlot)
{
   if (!nativeSlot) return false;
   if (abi == kGuestAbiDark1 &&
       ((service == kAbiServiceObject && guestSlot > 24) ||
         (service == kAbiServiceProperty && guestSlot > 11) ||
         (service == kAbiServicePhysics && guestSlot > 11) ||
         (service == kAbiServiceSound && guestSlot > 18) ||
         (service == kAbiServiceAI && guestSlot > 14)))
      return false;

   const sServiceMethod *methods = abi == kGuestAbiDark1 ?
      kDark1Methods : kDark2Methods;
   const size_t count = abi == kGuestAbiDark1 ?
      sizeof(kDark1Methods) / sizeof(kDark1Methods[0]) :
      sizeof(kDark2Methods) / sizeof(kDark2Methods[0]);
   for (size_t i = 0; i < count; ++i)
      if (methods[i].service == service && methods[i].guestSlot == guestSlot)
      {
         *nativeSlot = methods[i].nativeSlot;
         return true;
      }
   *nativeSlot = guestSlot;
   return true;
}

bool GuestServiceMethodExists(eGuestAbi abi, uint16_t serviceId,
                              unsigned guestSlot)
{
   if (guestSlot <= 2 || abi != kGuestAbiDark1)
      return true;

   for (size_t i = 0;
        i < sizeof(kDark1Services) / sizeof(kDark1Services[0]); ++i)
      if (kDark1Services[i].id == serviceId)
         return guestSlot >= 3 && guestSlot <= kDark1Services[i].lastSlot;

   // Do not impose the stock ABI on a separately supplied custom service.
   return true;
}

bool GuestPostMessageHasFlags(eGuestAbi abi) { return abi == kGuestAbiDark2; }
bool GuestSoundHasNetworkArgument(eGuestAbi abi) { return abi == kGuestAbiDark2; }
bool GuestStimulateHasSourceArgument(eGuestAbi abi) { return abi == kGuestAbiDark2; }

} // namespace osm32
