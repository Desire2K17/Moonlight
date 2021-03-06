#pragma once

#include "AnimState.h"
#include "ClientClass.h"
#include "Cvar.h"
#include "Engine.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "matrix3x4.h"
#include "ModelRender.h"
#include "Utils.h"
#include "VarMapping.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponData.h"
#include "WeaponId.h"

#include "UserCmd.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"

#include <functional>

struct AnimState;

struct AnimationLayer
{
public:
    float animationTime; //0
    float fadeOut; //4
    int dispatchedSrc; //12
    int dispatchedDst; //16
    unsigned int order; //20, networked
    unsigned int sequence; //24, networked
    float prevCycle; //28, networked
    float weight; //32, networked
    float weightDeltaRate; //36, networked
    float playbackRate; //40, networked
    float cycle; //44, networked
    void* owner; //48
    int invalidatePhysicsBits; //52
    void reset()
    {
        sequence = 0;
        weight = 0;
        weightDeltaRate = 0;
        playbackRate = 0;
        prevCycle = 0;
        cycle = 0;
    }
};

enum class MoveType {
    NOCLIP = 8,
    LADDER = 9
};

enum class ObsMode {
    None = 0,
    Deathcam,
    Freezecam,
    Fixed,
    InEye,
    Chase,
    Roaming
};

class Collideable {
public:
    VIRTUAL_METHOD(const Vector&, obbMins, 1, (), (this))
    VIRTUAL_METHOD(const Vector&, obbMaxs, 2, (), (this))
};

class Entity {
public:
    VIRTUAL_METHOD(void, release, 1, (), (this + 8))
    VIRTUAL_METHOD(ClientClass*, getClientClass, 2, (), (this + 8))
    VIRTUAL_METHOD(void, preDataUpdate, 6, (int updateType), (this + 8, updateType))
    VIRTUAL_METHOD(void, postDataUpdate, 7, (int updateType), (this + 8, updateType))
    VIRTUAL_METHOD(bool, isDormant, 9, (), (this + 8))
    VIRTUAL_METHOD(int, index, 10, (), (this + 8))
    VIRTUAL_METHOD(void, setDestroyedOnRecreateEntities, 13, (), (this + 8))

    VIRTUAL_METHOD(Vector&, getRenderOrigin, 1, (), (this + 4))
    VIRTUAL_METHOD(const Model*, getModel, 8, (), (this + 4))
    VIRTUAL_METHOD(const matrix3x4&, toWorldTransform, 32, (), (this + 4))

    VIRTUAL_METHOD(int&, handle, 2, (), (this))
    VIRTUAL_METHOD(Collideable*, getCollideable, 3, (), (this))
    VIRTUAL_METHOD(Vector&, getAbsOrigin, 10, (), (this))
    VIRTUAL_METHOD(Vector&, getAbsAngle, 11, (), (this))
    VIRTUAL_METHOD(void, setModelIndex, 75, (int index), (this, index))
    VIRTUAL_METHOD(int, health, 122, (), (this))
    VIRTUAL_METHOD(bool, isAlive, 156, (), (this))
    VIRTUAL_METHOD(bool, isPlayer, 158, (), (this))
  //  VIRTUAL_METHOD(bool, getAttachment, 84, (int index, Vector& origin), (this, index, std::ref(origin)))
    VIRTUAL_METHOD(bool, isWeapon, 166, (), (this))
    VIRTUAL_METHOD(Entity*, getActiveWeapon, 268, (), (this))
    VIRTUAL_METHOD(int, getWeaponSubType, 282, (), (this))
    VIRTUAL_METHOD(ObsMode, getObserverMode, 294, (), (this))
    VIRTUAL_METHOD(Entity*, getObserverTarget, 295, (), (this))
    VIRTUAL_METHOD(Vector, getAimPunch, 345, (), (this))
    VIRTUAL_METHOD(WeaponType, getWeaponType, 455, (), (this))
    VIRTUAL_METHOD(WeaponInfo*, getWeaponData, 461, (), (this))
    VIRTUAL_METHOD(float, getInaccuracy, 483, (), (this))
    VIRTUAL_METHOD(float, getSpread, 453, (), (this))
    VIRTUAL_METHOD(void, UpdateClientSideAnimation, 224, (), (this))

    constexpr auto isPistol() noexcept
    {
        return getWeaponType() == WeaponType::Pistol;
    }

    constexpr auto isShotgun() noexcept
    {
        return getWeaponType() == WeaponType::Shotgun;
    }

    constexpr auto isKnife() noexcept
    {
        return getWeaponType() == WeaponType::Knife;
    }

    constexpr auto isGrenade() noexcept
    {
        return getWeaponType() == WeaponType::Grenade;
    }

    constexpr auto isSniperRifle() noexcept
    {
        return getWeaponType() == WeaponType::SniperRifle;
    }
    
    constexpr auto isFullAuto() noexcept
    {
        const auto weaponData = getWeaponData();
        if (weaponData)
            return weaponData->fullAuto;
        return false;
    }

    constexpr auto requiresRecoilControl() noexcept
    {
        const auto weaponData = getWeaponData();
        if (weaponData)
            return weaponData->recoilMagnitude < 35.0f && weaponData->recoveryTimeStand > weaponData->cycletime;
        return false;
    }

    bool setupBones(matrix3x4* out, int maxBones, int boneMask, float currentTime) noexcept
    {
        if (localPlayer && this == localPlayer.get() && localPlayer->isAlive())
        {
            uint32_t* effects = this->getEffects();
            uint32_t* shouldskipframe = (uint32_t*)((uintptr_t)this + 0xA68);
            uint32_t backup_effects = *effects;
            uint32_t backup_shouldskipframe = *shouldskipframe;
            *shouldskipframe = 0;
            *effects |= 8;
            auto result = VirtualMethod::call<bool, 13>(this + 4, out, maxBones, boneMask, currentTime);
            *effects = backup_effects;
            *shouldskipframe = backup_shouldskipframe;
            return result;
        }
        else 
        {
            *reinterpret_cast<int*>(this + 0xA28) = 0;
            *reinterpret_cast<int*>(this + 0xA30) = memory->globalVars->framecount;
            int* render = reinterpret_cast<int*>(this + 0x274);
            uint32_t* shouldskipframe = (uint32_t*)((uintptr_t)this + 0xA68);
            uint32_t* effects = this->getEffects();
            int backup = *render;
            uint32_t backup_effects = *effects;
            *shouldskipframe = 0;
            *render = 0;
            boneMask |= 0x200;
            *effects |= 8;
            auto result = VirtualMethod::call<bool, 13>(this + 4, out, maxBones, boneMask, currentTime);
            *render = backup;
            *effects = backup_effects;
            return result;
        }
        return VirtualMethod::call<bool, 13>(this + 4, out, maxBones, boneMask, currentTime);
    }

    uint32_t* getEffects()
    {
        return (uint32_t*)((uintptr_t)this + 0xF0);
    }

    Vector getBonePosition(int bone) noexcept
    {
        if (matrix3x4 boneMatrices[256]; setupBones(boneMatrices, 256, 256, 0.0f))
            return Vector{ boneMatrices[bone][0][3], boneMatrices[bone][1][3], boneMatrices[bone][2][3] };
        else
            return Vector{ };
    }

    void modifyEyePos(Vector* pos) noexcept
    {
        const auto animState = getAnimstate();

        if (!animState)
            return;

        if (animState->InHitGroundAnimation || animState->duckAmount != 0.f || !groundEntity())
        {
            auto bone_pos = getBonePosition(8);

            bone_pos.z += 1.7f;

            if (pos->z > bone_pos.z)
            {
                float some_factor = 0.f;

                float delta = pos->z - bone_pos.z;

                float some_offset = (delta - 4.f) / 6.f;
                if (some_offset >= 0.f)
                    some_factor = std::fminf(some_offset, 1.f);

                pos->z += ((bone_pos.z - pos->z) * (((some_factor * some_factor) * 3.f) - (((some_factor * some_factor) * 2.f) * some_factor)));
            }
        }
    }

    auto getEyePosition() noexcept
    {
        Vector vec;
        VirtualMethod::call<void, 169>(this, std::ref(vec));//VirtualMethod::call<void, 284>(this, std::ref(vec));
        if(localPlayer && localPlayer->isAlive() && this == localPlayer.get())
            modifyEyePos(&vec);
        return vec;
    }

    bool isVisible(const Vector& position = { }) noexcept
    {
        if (!localPlayer)
            return false;

        Trace trace;
        interfaces->engineTrace->traceRay({ localPlayer->getEyePosition(), position ? position : getBonePosition(8) }, 0x46004009, { localPlayer.get() }, trace);
        return trace.entity == this || trace.fraction > 0.97f;
    }

    bool isOtherEnemy(Entity* other) noexcept;

    VarMap* getVarMap() noexcept
    {
        return reinterpret_cast<VarMap*>(this + 0x24);
    }
   
    AnimState* getAnimstate() noexcept
    {
        return *reinterpret_cast<AnimState**>(this + 0x9960);
    }

    float getMaxDesyncAngle() noexcept
    {
        const auto animState = getAnimstate();

        if (!animState)
            return 0.0f;

        float yawModifier = (animState->stopToFullRunningFraction * -0.3f - 0.2f) * std::clamp(animState->footSpeed, 0.0f, 1.0f) + 1.0f;

        if (animState->duckAmount > 0.0f)
            yawModifier += (animState->duckAmount * std::clamp(animState->footSpeed2, 0.0f, 1.0f) * (0.5f - yawModifier));

        return animState->velocitySubtractY * yawModifier;
    }

    bool isInReload() noexcept
    {
        return *reinterpret_cast<bool*>(uintptr_t(&clip()) + 0x41);
    }

    auto getUserId() noexcept
    {
        if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
            return playerInfo.userId;

        return -1;
    }

    [[nodiscard]] auto getPlayerName(bool normalize) noexcept
    {
        std::string playerName = "unknown";

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
            return playerName;

        playerName = playerInfo.name;

        if (normalize) {
            if (wchar_t wide[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, 128, wide, 128)) {
                if (wchar_t wideNormalized[128]; NormalizeString(NormalizationKC, wide, -1, wideNormalized, 128)) {
                    if (char nameNormalized[128]; WideCharToMultiByte(CP_UTF8, 0, wideNormalized, -1, nameNormalized, 128, nullptr, nullptr))
                        playerName = nameNormalized;
                }
            }
        }

        playerName.erase(std::remove(playerName.begin(), playerName.end(), '\n'), playerName.cend());
        return playerName;
    }

    int getAnimationLayerCount() noexcept
    {
        return *reinterpret_cast<int*>(this + 0x298C);
    }

    AnimationLayer* animOverlays()
    {
        return *reinterpret_cast<AnimationLayer**>(uintptr_t(this) + 0x2990);// 0x2990  0x2980
    }

    AnimationLayer* getAnimationLayer(int overlay) noexcept
    {
        return &animOverlays()[overlay];
    }

    std::array<float, 24>& pose_parameters()
    {
        return *reinterpret_cast<std::add_pointer_t<std::array<float, 24>>>((uintptr_t)this + netvars->operator[](fnv::hash("CBaseAnimating->m_flPoseParameter")));
    }

    void CreateState(AnimState* state)
    {
        static auto CreateAnimState = reinterpret_cast<void(__thiscall*)(AnimState*, Entity*)>(memory->CreateState);
        if (!CreateAnimState)
            return;

        CreateAnimState(state, this);
    }

    void UpdateState(AnimState* state, Vector angle) {
        if (!state || !angle)
            return;
        static auto UpdateAnimState = reinterpret_cast<void(__vectorcall*)(void*, void*, float, float, float, void*)>(memory->UpdateState);
        if (!UpdateAnimState)
            return;
        UpdateAnimState(state,nullptr,0.0f,angle.y,angle.x,nullptr);
    }

    void ResetState(AnimState* state)
    {
        if (!state)
            return;
        static auto ResetAnimState = reinterpret_cast<void(__thiscall*)(AnimState*)>(memory->ResetState);
        if (!ResetAnimState)
            return;

        ResetAnimState(state);
    }
    
    float spawnTime()
    {
        return *(float*)((uintptr_t)this + 0x103C0);//0x103C0return *(float*)((uintptr_t)this + 0xA370);
    }

    void InvalidateBoneCache()
    {
        static auto invalidate_bone_cache_fn = memory->InvalidateBoneCache;
        reinterpret_cast<void(__fastcall*) (void*)> (invalidate_bone_cache_fn) (this);
    }

    bool isThrowing() {
        if (this->isGrenade())
        {
            if (!this->pinPulled())
            {
                float throwtime = this->throwTime();
                if (throwtime > 0)
                    return true;
            }
        }
        return false;
    }

    Vector* getAbsVelocity()
    {
        return (Vector*)(this + 0x94);
    }

    bool isFlashed() noexcept
    {
        return flashDuration() > 75.0f;
    }

       
        NETVAR(groundEntity, "CBasePlayer", "m_hGroundEntity", Entity*);
        NETVAR(throwTime, "CBaseCSGrenade", "m_fThrowTime", float_t);
        NETVAR(ClientSideAnimation, "CBaseAnimating", "m_bClientSideAnimation", bool)
        NETVAR(pinPulled, "CBaseCSGrenade", "m_bPinPulled", bool);
        NETVAR(nextSecondaryAttack, "CBaseCombatWeapon", "m_flNextSecondaryAttack", float)
        NETVAR(recoilIndex, "CBaseCombatWeapon", "m_flRecoilIndex", float)
        NETVAR(readyTime, "CBaseCombatWeapon", "m_flPostponeFireReadyTime", float)
        NETVAR(burstMode, "CBaseCombatWeapon", "m_bBurstMode", bool)
        NETVAR(burstShotRemaining, "CBaseCombatWeapon", "m_iBurstShotsRemaining", int)
        NETVAR(sentryHealth, "CDronegun", "m_iHealth", int)
 
        NETVAR(body, "CBaseAnimating", "m_nBody", int)
        NETVAR(hitboxSet, "CBaseAnimating", "m_nHitboxSet", int)

        NETVAR(modelIndex, "CBaseEntity", "m_nModelIndex", unsigned)
        NETVAR(origin, "CBaseEntity", "m_vecOrigin", Vector)
        NETVAR_OFFSET(moveType, "CBaseEntity", "m_nRenderMode", 1, MoveType)
        NETVAR(simulationTime, "CBaseEntity", "m_flSimulationTime", float)
        NETVAR(ownerEntity, "CBaseEntity", "m_hOwnerEntity", int)
        NETVAR(team, "CBaseEntity", "m_iTeamNum", int)
        NETVAR(spotted, "CBaseEntity", "m_bSpotted", bool)

        NETVAR(weapons, "CBaseCombatCharacter", "m_hMyWeapons", int[64])
        PNETVAR(wearables, "CBaseCombatCharacter", "m_hMyWearables", int)

        NETVAR(viewModel, "CBasePlayer", "m_hViewModel[0]", int)
        NETVAR(fov, "CBasePlayer", "m_iFOV", int)
        NETVAR(fovStart, "CBasePlayer", "m_iFOVStart", int)
        NETVAR(defaultFov, "CBasePlayer", "m_iDefaultFOV", int)
        NETVAR(flags, "CBasePlayer", "m_fFlags", int)
        NETVAR(tickBase, "CBasePlayer", "m_nTickBase", int)
        NETVAR(aimPunchAngle, "CBasePlayer", "m_aimPunchAngle", Vector)
        NETVAR(viewPunchAngle, "CBasePlayer", "m_viewPunchAngle", Vector)
        NETVAR(velocity, "CBasePlayer", "m_vecVelocity[0]", Vector)
        NETVAR(lastPlaceName, "CBasePlayer", "m_szLastPlaceName", char[18])

        NETVAR(armor, "CCSPlayer", "m_ArmorValue", int)
        NETVAR(eyeAngles, "CCSPlayer", "m_angEyeAngles", Vector)
        NETVAR(isScoped, "CCSPlayer", "m_bIsScoped", bool)
        NETVAR(isDefusing, "CCSPlayer", "m_bIsDefusing", bool)
        NETVAR_OFFSET(flashDuration, "CCSPlayer", "m_flFlashMaxAlpha", -8, float)
        NETVAR(flashMaxAlpha, "CCSPlayer", "m_flFlashMaxAlpha", float)
        NETVAR(gunGameImmunity, "CCSPlayer", "m_bGunGameImmunity", bool)
        NETVAR(account, "CCSPlayer", "m_iAccount", int)
        NETVAR(inBombZone, "CCSPlayer", "m_bInBombZone", bool)
        NETVAR(hasDefuser, "CCSPlayer", "m_bHasDefuser", bool)
        NETVAR(hasHelmet, "CCSPlayer", "m_bHasHelmet", bool)
        NETVAR(lby, "CCSPlayer", "m_flLowerBodyYawTarget", float)
        NETVAR(ragdoll, "CCSPlayer", "m_hRagdoll", int)
        NETVAR(shotsFired, "CCSPlayer", "m_iShotsFired", int)
        NETVAR(waitForNoAttack, "CCSPlayer", "m_bWaitForNoAttack", bool)

        NETVAR(viewModelIndex, "CBaseCombatWeapon", "m_iViewModelIndex", int)
        NETVAR(worldModelIndex, "CBaseCombatWeapon", "m_iWorldModelIndex", int)
        NETVAR(worldDroppedModelIndex, "CBaseCombatWeapon", "m_iWorldDroppedModelIndex", int)
        NETVAR(weaponWorldModel, "CBaseCombatWeapon", "m_hWeaponWorldModel", int)
        NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int)
        NETVAR(reserveAmmoCount, "CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount", int)
        NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float)

        NETVAR(nextAttack, "CBaseCombatCharacter", "m_flNextAttack", float)

        NETVAR(accountID, "CBaseAttributableItem", "m_iAccountID", int)
        NETVAR(itemDefinitionIndex, "CBaseAttributableItem", "m_iItemDefinitionIndex", short)
        NETVAR(itemDefinitionIndex2, "CBaseAttributableItem", "m_iItemDefinitionIndex", WeaponId)
        NETVAR(itemIDHigh, "CBaseAttributableItem", "m_iItemIDHigh", int)
        NETVAR(entityQuality, "CBaseAttributableItem", "m_iEntityQuality", int)
        NETVAR(customName, "CBaseAttributableItem", "m_szCustomName", char[32])
        NETVAR(fallbackPaintKit, "CBaseAttributableItem", "m_nFallbackPaintKit", unsigned)
        NETVAR(fallbackSeed, "CBaseAttributableItem", "m_nFallbackSeed", unsigned)
        NETVAR(fallbackWear, "CBaseAttributableItem", "m_flFallbackWear", float)
        NETVAR(fallbackStatTrak, "CBaseAttributableItem", "m_nFallbackStatTrak", unsigned)
        NETVAR(initialized, "CBaseAttributableItem", "m_bInitialized", bool)

        NETVAR(owner, "CBaseViewModel", "m_hOwner", int)
        NETVAR(weapon, "CBaseViewModel", "m_hWeapon", int)

        NETVAR(c4StartedArming, "CC4", "m_bStartedArming", bool)

        NETVAR(c4BlowTime, "CPlantedC4", "m_flC4Blow", float)
        NETVAR(c4BombSite, "CPlantedC4", "m_nBombSite", int)
        NETVAR(c4Ticking, "CPlantedC4", "m_bBombTicking", bool)
        NETVAR(c4DefuseCountDown, "CPlantedC4", "m_flDefuseCountDown", float)
        NETVAR(c4Defuser, "CPlantedC4", "m_hBombDefuser", int)

        NETVAR(tabletReceptionIsBlocked, "CTablet", "m_bTabletReceptionIsBlocked", bool)

        NETVAR(droneTarget, "CDrone", "m_hMoveToThisEntity", int)

        NETVAR(thrower, "CBaseGrenade", "m_hThrower", int)
};
