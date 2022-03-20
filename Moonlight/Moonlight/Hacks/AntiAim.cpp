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

bool AutoDir(Entity* entity, Vector eye_) noexcept
{
    constexpr float maxRange{ 8192.0f };

    Vector eye = eye_;
    eye.x = 0.f;
    Vector eyeAnglesLeft45 = eye;
    Vector eyeAnglesRight45 = eye;
    eyeAnglesLeft45.y += 45.f;
    eyeAnglesRight45.y -= 45.f;


    Vector viewAnglesLeft45{ cos(degreesToRadians(eyeAnglesLeft45.x)) * cos(degreesToRadians(eyeAnglesLeft45.y)) * maxRange,
               cos(degreesToRadians(eyeAnglesLeft45.x)) * sin(degreesToRadians(eyeAnglesLeft45.y)) * maxRange,
              -sin(degreesToRadians(eyeAnglesLeft45.x)) * maxRange };

    Vector viewAnglesRight45{ cos(degreesToRadians(eyeAnglesRight45.x)) * cos(degreesToRadians(eyeAnglesRight45.y)) * maxRange,
                       cos(degreesToRadians(eyeAnglesRight45.x)) * sin(degreesToRadians(eyeAnglesRight45.y)) * maxRange,
                      -sin(degreesToRadians(eyeAnglesRight45.x)) * maxRange };

    static Trace traceLeft45;
    static Trace traceRight45;

    Vector headPosition{ localPlayer->getBonePosition(8) };

    interfaces->engineTrace->traceRay({ headPosition, headPosition + viewAnglesLeft45 }, 0x4600400B, { entity }, traceLeft45);
    interfaces->engineTrace->traceRay({ headPosition, headPosition + viewAnglesRight45 }, 0x4600400B, { entity }, traceRight45);

    float distanceLeft45 = sqrt(pow(headPosition.x - traceRight45.endpos.x, 2) + pow(headPosition.y - traceRight45.endpos.y, 2) + pow(headPosition.z - traceRight45.endpos.z, 2));
    float distanceRight45 = sqrt(pow(headPosition.x - traceLeft45.endpos.x, 2) + pow(headPosition.y - traceLeft45.endpos.y, 2) + pow(headPosition.z - traceLeft45.endpos.z, 2));

    float mindistance = std::min(distanceLeft45, distanceRight45);

    if (distanceLeft45 == mindistance) {
        return false;
    }
    return true;
}


void AntiAim::run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    bool lby = lbyUpdate();
    Animations::data.lby = lby;

    if (config->antiAim.enabled)
    {
        if (RunAA(cmd))
            return;

        bool invert = GetAsyncKeyState(config->antiAim.invert);

        cmd->viewangles.y += sendPacket || lbyUpdate() ? 0.f : invert ? -120.f : 120.f;
    }
}

void AntiAim::fakeLag(UserCmd* cmd, bool& sendPacket) noexcept
{
    auto chokedPackets = 0;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    chokedPackets = config->antiAim.enabled ? 2 : 0;

    if (config->antiAim.fakeLag) {
        switch (config->antiAim.fakeLagMode) {
        case 0: //Maximum
            chokedPackets = std::clamp(config->antiAim.fakeLagAmount, 1, 16);
            break;
        case 1: //Adaptive
            chokedPackets = std::clamp(static_cast<int>(std::ceilf(64 / (localPlayer->velocity().length() * memory->globalVars->intervalPerTick))), 1, config->antiAim.fakeLagAmount);
            break;
        case 2: //Random
            chokedPackets = RandomInt(1, config->antiAim.fakeLagAmount);
        }
    }


    sendPacket = interfaces->engine->getNetworkChannel()->chokedPackets >= chokedPackets;
}

