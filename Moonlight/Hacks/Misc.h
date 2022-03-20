#pragma once
#include "../SDK/Vector.h"

enum class FrameStage;
class GameEvent;
struct UserCmd;

namespace Misc
{
    void StrafeOptimizer(UserCmd* cmd) noexcept;
    void CrackUserHumiliator();
    void colorWorld() noexcept;
    void velocityGraph() noexcept;
    void clipBruh() noexcept;
    void nadePredict() noexcept;
    void updateClanTag(bool = false) noexcept;
    void showVelocity() noexcept;
    void crouchbag(UserCmd* cmd) noexcept;
    void edgebugv2(UserCmd* cmd) noexcept;
    void edgebug(UserCmd* cmd) noexcept;
    void jumpbug(UserCmd* cmd) noexcept;
	void removeClientSideChokeLimit() noexcept;
	void edgejump(UserCmd* cmd) noexcept;
    void slowwalk(UserCmd* cmd) noexcept;
	void fastStop(UserCmd* cmd) noexcept;
    void inverseRagdollGravity() noexcept;
    void Testshit() noexcept;
    void clanTag1() noexcept;
    void clanTag() noexcept;
    void spectatorList() noexcept;
    void debugwindow() noexcept;
    void sniperCrosshair() noexcept;
    void recoilCrosshair() noexcept;
    void watermark() noexcept;
    void prepareRevolver(UserCmd*) noexcept;
    void fastPlant(UserCmd*) noexcept;
    void drawBombTimer() noexcept;
    void stealNames() noexcept;
    void disablePanoramablur() noexcept;
    bool changeName(bool, const char*, float) noexcept;
    void bunnyHop(UserCmd*) noexcept;
    void fakeBan(bool = false) noexcept;
    void unlockHiddenCvars() noexcept;
    void nadeTrajectory() noexcept;
    void showImpacts() noexcept;
    void killSay(GameEvent& event) noexcept;
    void chickenDeathSay(GameEvent& event) noexcept;
    void fixMovement(UserCmd* cmd, float yaw) noexcept;
    void antiAfkKick(UserCmd* cmd) noexcept;
    void autoPistol(UserCmd* cmd) noexcept;
    void revealRanks(UserCmd* cmd) noexcept;
	float autoStrafe(UserCmd* cmd, const Vector& currentViewAngles) noexcept;
	void quickPeek(UserCmd* cmd) noexcept;
    void drawQuickPeekStartPos() noexcept;
    void fixMouseDelta(UserCmd* cmd) noexcept;
    void voteRevealer(GameEvent& event) noexcept;
	void removeCrouchCooldown(UserCmd* cmd) noexcept;
    void moonwalk(UserCmd* cmd) noexcept;
    void playHitSound(GameEvent& event) noexcept;
    void killSound(GameEvent& event) noexcept;
    void footstepSonar(GameEvent* event) noexcept;

    void purchaseList(GameEvent* event = nullptr) noexcept;
    void autoBuy(GameEvent* event = nullptr) noexcept;
    void preserveDeathNotices(GameEvent* event = nullptr) noexcept;
    void oppositeHandKnife(FrameStage stage) noexcept;
    void playerBlocker(UserCmd* cmd) noexcept;
    void doorSpam(UserCmd* cmd) noexcept;
    void DDrun(UserCmd* cmd) noexcept;
    void ShowAccuracy() noexcept;
}
