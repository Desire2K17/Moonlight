#include "Backtrack.h"
#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"

bool canShift(int ticks, bool shiftAnyways = false)
{
    if (!localPlayer || !localPlayer->isAlive() || !config->ragebotExtra.enabled || ticks <= 0)
        return false;

    if (shiftAnyways)
        return true;

    if ((Tickbase::tick->ticksAllowedForProcessing - ticks) < 0)
        return false;

    if (localPlayer->nextAttack() > memory->globalVars->serverTime())
        return false;

    float nextAttack = (localPlayer->nextAttack() + (ticks * memory->globalVars->intervalPerTick));
    if (nextAttack >= memory->globalVars->serverTime())
        return false;

    auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip() || activeWeapon->isThrowing())
        return false;

    if (activeWeapon->isKnife() || activeWeapon->isGrenade() || activeWeapon->isShotgun()
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Awp
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Ssg08
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Taser
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
        return false;

    float shiftTime = (localPlayer->tickBase() - ticks) * memory->globalVars->intervalPerTick;

    if (shiftTime < activeWeapon->nextPrimaryAttack())
        return false;

    return true;
}

void recalculateTicks() noexcept
{
    Tickbase::tick->chokedPackets = std::clamp(Tickbase::tick->chokedPackets, 0, Tickbase::tick->maxUsercmdProcessticks);
    Tickbase::tick->ticksAllowedForProcessing = Tickbase::tick->maxUsercmdProcessticks - Tickbase::tick->chokedPackets;
    Tickbase::tick->ticksAllowedForProcessing = std::clamp(Tickbase::tick->ticksAllowedForProcessing, 0, Tickbase::tick->maxUsercmdProcessticks);
}

void Tickbase::shiftTicks(int ticks, UserCmd* cmd, bool shiftAnyways) noexcept //useful, for other funcs
{
    if (!localPlayer || !localPlayer->isAlive() || !config->ragebotExtra.enabled)
        return;
    if (!canShift(ticks, shiftAnyways))
        return;
    tick->commandNumber = cmd->commandNumber;
    tick->tickbase = localPlayer->tickBase();
    tick->tickshift = ticks;
    //Teleport kinda buggy
    //tick->chokedPackets += ticks;
    //recalculateTicks();
}

void Tickbase::run(UserCmd* cmd) noexcept
{

    static void* oldNetwork = nullptr;
    if(auto network = interfaces->engine->getNetworkChannel(); network && oldNetwork != network)
    {
        oldNetwork = network;
        tick->ticksAllowedForProcessing = tick->maxUsercmdProcessticks;
        tick->chokedPackets = 0;
    }
    if (auto network = interfaces->engine->getNetworkChannel(); network && network->chokedPackets > tick->chokedPackets)
        tick->chokedPackets = network->chokedPackets;

    recalculateTicks();

    tick->ticks = cmd->tickCount;
    if (!localPlayer || !localPlayer->isAlive() || !config->ragebotExtra.enabled)
        return;

    auto ticks = 0;

    switch (config->ragebotExtra.doubletapSpeed) {
    case 0: //Instant
        ticks = 15;
        break;
    case 1: //Fast
        ticks = 14;
        break;
    case 2: //Accurate
        ticks = 12;
        break;
    }

    if (config->ragebotExtra.doubletap && cmd->buttons & (UserCmd::IN_ATTACK))
        shiftTicks(ticks, cmd);


    recalculateTicks();
}