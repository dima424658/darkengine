// Guest ABI descriptions for legacy 32-bit Dark Engine OSM modules.

#ifndef __OSM32ABI_H
#define __OSM32ABI_H

#include <stdint.h>

namespace osm32
{

enum eGuestAbi
{
   kGuestAbiDark1,
   kGuestAbiDark2
};

enum eAbiService
{
   kAbiServiceObject,
   kAbiServiceProperty,
   kAbiServicePhysics,
   kAbiServiceSound,
   kAbiServiceAI
};

struct sServiceMethod
{
   eAbiService service;
   uint8_t guestSlot;
   uint8_t nativeSlot;
};

const char *GuestAbiName(eGuestAbi abi);
eGuestAbi GuestAbiFromProfile(const char *profile);
bool ResolveServiceMethod(eGuestAbi abi, eAbiService service,
                          unsigned guestSlot, unsigned *nativeSlot);
bool GuestServiceMethodExists(eGuestAbi abi, uint16_t serviceId,
                              unsigned guestSlot);
bool GuestPostMessageHasFlags(eGuestAbi abi);
bool GuestSoundHasNetworkArgument(eGuestAbi abi);
bool GuestStimulateHasSourceArgument(eGuestAbi abi);

} // namespace osm32

#endif // __OSM32ABI_H
