#include "Animations.h"
#include "AntiAim.h"
#include "Tickbase.h"

#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Surface.h"
#include "../SDK/GameEvent.h"


#include "../Interfaces.h"
#include "../Memory.h"
#include "../Hooks.h"

int RandomInt(int min, int max) noexcept
{
    return (min + 1) + (((int)rand()) / (int)RAND_MAX) * (max - (min + 1));
}

float RandomFloat(float min, float max) noexcept
{
    return (min + 1) + (((float)rand()) / (float)RAND_MAX) * (max - (min + 1));
}

bool RunAA(UserCmd* cmd) noexcept {
    if (!localPlayer || !localPlayer->isAlive())
        return true;

    if (localPlayer.get()->moveType() == MoveType::LADDER || localPlayer.get()->moveType() == MoveType::NOCLIP)
        return true;

    auto activeWeapon = localPlayer.get()->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return false;

    if (activeWeapon->isThrowing())
        return true;

    if (activeWeapon->isGrenade())
        return false;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto() || localPlayer->waitForNoAttack())
        return false;

    if (localPlayer.get()->nextAttack() > memory->globalVars->serverTime())
        return false;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return false;

    if (activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return false;

    if (localPlayer.get()->nextAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK)))
        return true;

    if (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK)))
        return true;

    if (activeWeapon->isKnife())
    {
        if (activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
            return false;

        if (activeWeapon->nextSecondaryAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK2)))
            return true;
    }
    if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver && activeWeapon->readyTime() > memory->globalVars->serverTime())
        return false;

    if ((activeWeapon->itemDefinitionIndex2() == WeaponId::Famas || activeWeapon->itemDefinitionIndex2() == WeaponId::Glock) && activeWeapon->burstMode() && activeWeapon->burstShotRemaining() > 0)
        return false;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return false;

    return false;
}


bool lbyUpdate() noexcept
{
    static float nextUpdate = 0.f;

    if (!(localPlayer->flags() & 1))
        return false;
    if (localPlayer->velocity().length() > 0.1f)
        nextUpdate = memory->globalVars->serverTime() + 0.22f;
    if (nextUpdate <= memory->globalVars->serverTime()) {
        nextUpdate = memory->globalVars->serverTime() + 1.1f;
        return true;
    }
    return false;
}

static bool invert = true;

bool getBestSide(float originalYaw) noexcept//supreme self coded genius code
{
    const float dst = 50.f;

    float startYaw = originalYaw + 90.f;
    float traceLength[2];
    float yawAdd[2] = { -90.f, 90.f };

    const Vector eyePos = localPlayer->getEyePosition();

    for (int i = 0; i < 2; i++) {
        const Vector traceStart = Vector{ eyePos.x + (20.f * cos(degreesToRadians(startYaw))), eyePos.y + (20.f * sin(degreesToRadians(startYaw))), eyePos.z };
        const Vector traceEnd = Vector{ traceStart.x + (dst * cos(degreesToRadians(startYaw + yawAdd[i]))), traceStart.y + (dst * sin(degreesToRadians(startYaw + yawAdd[i]))), traceStart.z };

        Trace trace;
        interfaces->engineTrace->traceRay({ traceStart, traceEnd }, 0x4600400B, localPlayer.get(), trace);
        traceLength[i] = fabsf(traceStart.distance(trace.endpos));

        startYaw += 180.f;
    }

    if (int delta = traceLength[0] - traceLength[1]; fabs(delta) != 0)
        return radiansToDegrees(atanf((dst * 2) / delta)) < 0.f;
    return invert;
}

void AntiAim::run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    bool jitter = memory->globalVars->tickCount % 2;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!config->antiAim.enabled)
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();

    if (cmd->buttons & UserCmd::IN_ATTACK && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime())
        return;

    if (cmd->buttons & UserCmd::IN_USE)
        return;

    if (activeWeapon->isGrenade() && activeWeapon->isThrowing())
        return;

    if (localPlayer->moveType() == MoveType::LADDER || localPlayer->moveType() == MoveType::NOCLIP)
        return;

    cmd->buttons &= ~(UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT);

        if (GetAsyncKeyState(config->antiAim.invert) & 1)
            invert = !invert;

        bool side = invert;
        if (config->antiAim.autoDirection)
            side = getBestSide(cmd->viewangles.y);


    switch (config->antiAim.pitch)
    {        //skip 0 because 0 is Default
    case 1: cmd->viewangles.x = -89; break;
    case 2: cmd->viewangles.x = 89;  break;
    case 3: cmd->viewangles.x = 0;   break;
    }

    switch (config->antiAim.yaw)
    {        //skip 0 because 0 is Default
    case 1: cmd->viewangles.y += 180; break;
    case 2: cmd->viewangles.y += RandomFloat(180, -180);  break;
    case 3: cmd->viewangles.y += 180; cmd->viewangles.y += RandomFloat(45, -45);   break;
    }

    sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= 1;
    cmd->viewangles.y += sendPacket || lbyUpdate() ? 0.f : side ? 120.f : -120.f;

    if (config->antiAim.extended) {
        if (lbyUpdate()) {
            sendPacket = false;
            cmd->viewangles.y -= side ? 90.f : -90.f;
        }
    }
    else {
        int move = localPlayer->flags() & 2 ? 3 : 2;
        if (localPlayer->flags() & 1 && cmd->sidemove > -3 && cmd->sidemove < 3)
            cmd->sidemove = memory->globalVars->tickCount % 2 ? move : -move;
    }
}

void AntiAim::fakeLag(UserCmd* cmd, bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

}

