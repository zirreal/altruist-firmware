#ifndef __RWS_DEVICES_REGISTRY_H__
#define __RWS_DEVICES_REGISTRY_H__

class Robonomics;

/** Clear stale registration hash when owner was never saved (no config.json edit needed). */
void repairInconsistentRwsRegistrationState();

/** Register RWS device list on-chain via rws.set_devices (also syncs owner when needed). */
void ensureRwsDevicesRegistered(Robonomics* robonomics);

#endif
