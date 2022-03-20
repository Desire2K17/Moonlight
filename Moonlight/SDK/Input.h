#pragma once

#include "VirtualMethod.h"

#include "Vector.h"
#include "UserCmd.h"

class VerifiedUserCmd
{
public:
	UserCmd cmd;
	unsigned long crc;
};
class Input {
public:
#ifdef _WIN32
	PAD(12)
#else
	PAD(16)
#endif
	bool isTrackIRAvailable;
	bool isMouseInitialized;
	bool isMouseActive;
#ifdef _WIN32
	PAD(158)
#else
	PAD(182)
#endif
		bool isCameraInThirdPerson;
	bool cameraMovingWithMouse;
	Vector cameraOffset;

	UserCmd* GetUserCmd(int sequence_number)
	{
		return VirtualMethod::call<UserCmd*, 8>(this, 0, sequence_number);
	}

	UserCmd* GetUserCmd(int nSlot, int sequence_number)
	{
		return VirtualMethod::call<UserCmd*, 8>(this, nSlot, sequence_number);
	}

	VerifiedUserCmd* GetVerifiedCmd(int sequence_number)
	{
		auto verifiedCommands = *(VerifiedUserCmd**)(reinterpret_cast<uint32_t>(this) + 0xF8);
		return &verifiedCommands[sequence_number % 150];
	}
};