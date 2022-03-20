#pragma once

struct UserCmd;
struct Vector;

namespace AntiAim {
	void run(UserCmd*, const Vector&, const Vector&, bool&) noexcept;
	void fakeLag(UserCmd*, bool&) noexcept;
	void fakeDuck(UserCmd*) noexcept;
	void legit(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept;
}
