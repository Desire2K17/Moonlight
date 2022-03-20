#include <mutex>
#include <numeric>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "../Hooks.h"
#include "Misc.h"
#include "../SDK/ConVar.h"
#include "../SDK/Surface.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/WeaponData.h"
#include "EnginePrediction.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GameEvent.h"
#include "../SDK/FrameStage.h"
#include "../SDK/Client.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/WeaponSystem.h"
#include "../SDK/WeaponData.h"
#include "../GUI.h"
#include "../Helpers.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/Beams.h"
#include <fstream>

#include "../encrypt.h"

#define encr(str) HanasMainKey_x000c7B(str, __TIME__[4], __TIME__[7])
#define HanasMainKey_x000c7B(str, key1, key2) []() { \
            constexpr static auto crypted = Hana::HanasCrypter \
                <sizeof(str) / sizeof(str[0]), key1, key2, Hana::clean_type<decltype(str[0])>>((Hana::clean_type<decltype(str[0])>*)str); \
                    return crypted; }()

void Misc::StrafeOptimizer(UserCmd* cmd) noexcept
{
    {
        if (localPlayer
            && localPlayer->isAlive()
            && config->misc.strafeOptimizer
            && !(localPlayer->flags() & 1)
            ) 
        {

           // bool iMoveBoth = ((cmd->mousedx > 0) &&
               // (cmd->buttons & UserCmd::IN_MOVERIGHT && cmd->buttons & UserCmd::IN_MOVELEFT));

            if (cmd->mousedx < 0)
                cmd->sidemove = -450.0f;
            else if (cmd->mousedx > 0)
                cmd->sidemove = 450.0f;
        }
    }
}

void Misc::updateClanTag(bool tagChanged) noexcept
{
    static std::string clanTag;

    if (tagChanged) 
    {
        clanTag = config->misc.clanTag;
        if (!clanTag.empty() && clanTag.front() != ' ' && clanTag.back() != ' ')
            clanTag.push_back(' ');
        return;
    }

}

void Misc::fixMouseDelta(UserCmd* cmd) noexcept
{
    if (!cmd)
        return;

    static Vector delta_viewangles{ };
    Vector delta = cmd->viewangles - delta_viewangles;

    delta.x = std::clamp(delta.x, -89.0f, 89.0f);
    delta.y = std::clamp(delta.y, -180.0f, 180.0f);
    delta.z = 0.0f;
    static ConVar* sensitivity;
    if (!sensitivity)
        sensitivity = interfaces->cvar->findVar("sensitivity");;
    if (delta.x != 0.f) {
        static ConVar* m_pitch;

        if (!m_pitch)
            m_pitch = interfaces->cvar->findVar("m_pitch");

        int final_dy = static_cast<int>((delta.x / m_pitch->getFloat()) / sensitivity->getFloat());
        if (final_dy <= 32767) {
            if (final_dy >= -32768) {
                if (final_dy >= 1 || final_dy < 0) {
                    if (final_dy <= -1 || final_dy > 0)
                        final_dy = final_dy;
                    else
                        final_dy = -1;
                }
                else {
                    final_dy = 1;
                }
            }
            else {
                final_dy = 32768;
            }
        }
        else {
            final_dy = 32767;
        }

        cmd->mousedy = static_cast<short>(final_dy);
    }

    if (delta.y != 0.f) {
        static ConVar* m_yaw;

        if (!m_yaw)
            m_yaw = interfaces->cvar->findVar("m_yaw");

        int final_dx = static_cast<int>((delta.y / m_yaw->getFloat()) / sensitivity->getFloat());
        if (final_dx <= 32767) {
            if (final_dx >= -32768) {
                if (final_dx >= 1 || final_dx < 0) {
                    if (final_dx <= -1 || final_dx > 0)
                        final_dx = final_dx;
                    else
                        final_dx = -1;
                }
                else {
                    final_dx = 1;
                }
            }
            else {
                final_dx = 32768;
            }
        }
        else {
            final_dx = 32767;
        }

        cmd->mousedx = static_cast<short>(final_dx);
    }

    delta_viewangles = cmd->viewangles;
}


void Misc::unlockHiddenCvars() noexcept
{
    auto iterator = **reinterpret_cast<conCommandBase***>(interfaces->cvar + 0x34);
    for (auto c = iterator->next; c != nullptr; c = c->next)
    {
        conCommandBase* cmd = c;
        cmd->flags &= ~(1 << 1);
        cmd->flags &= ~(1 << 4);
    }
}


void Misc::edgebug(UserCmd* cmd) noexcept {
    if (!config->misc.edgebug || !localPlayer || !localPlayer->isAlive())
        return;

    static bool bhopWasEnabled = true;
    bool JumpDone;

    bool unduck = true;

    float max_radius = M_PI * 2;
    float step = max_radius / 128;
    float xThick = 23;

    if (config->misc.edgebugstyle == 0) {

        if (GetAsyncKeyState(config->misc.edgebugkey) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {
            cmd->buttons |= UserCmd::IN_DUCK;

        }
    }


    else if (config->misc.edgebugstyle == 1) {

        if (GetAsyncKeyState(config->misc.edgebugkey) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {


            if (unduck) {
                JumpDone = false;
                if (config->misc.jumpbughold)
                    cmd->buttons |= UserCmd::IN_JUMP; // If you want to hold JB key only.
                else
                    cmd->buttons &= ~UserCmd::IN_DUCK;
                unduck = false;
            }

            Vector pos = localPlayer->origin();

            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = (xThick * cos(a)) + pos.x;
                pt.y = (xThick * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.0f && target.fraction != 0.0f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    unduck = true;
                }
            }
            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = ((xThick - 2.f) * cos(a)) + pos.x;
                pt.y = ((xThick - 2.f) * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.f && target.fraction != 0.f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = ((xThick - 20.f) * cos(a)) + pos.x;
                pt.y = ((xThick - 20.f) * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.f && target.fraction != 0.f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    unduck = true;
                }
            }
        }
    }
    else if (config->misc.edgebugstyle == 2) {

        if (GetAsyncKeyState(config->misc.edgebugkey    ) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {


            Vector pos = localPlayer->origin();

            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = (xThick * cos(a)) + pos.x;
                pt.y = (xThick * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.0f && target.fraction != 0.0f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                }
            }
        }
    }
}

void Misc::jumpbug(UserCmd* cmd) noexcept {
    if (!config->misc.jumpbug || !localPlayer || !localPlayer->isAlive())
        return;

    static bool bhopWasEnabled = true;
    bool JumpDone;

    bool unduck = true;

    float max_radius = M_PI * 2;
    float step = max_radius / 128;
    float xThick = 23;

    if (config->misc.jumpbugstyle == 0) {

        if (GetAsyncKeyState(config->misc.jumpbugkey) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {

            if (config->misc.bunnyHop) {
                config->misc.bunnyHop = false;
                bhopWasEnabled = false;
            }

            if (unduck) {
                JumpDone = false;
                if (config->misc.jumpbughold)
                    cmd->buttons |= UserCmd::IN_JUMP; // If you want to hold JB key only.
                else
                    cmd->buttons &= ~UserCmd::IN_DUCK;
                unduck = false;
            }

            Vector pos = localPlayer->origin();

            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = ((xThick - 50.f) * cos(a)) + pos.x;
                pt.y = ((xThick - 50.f) * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 4;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.0f && target.fraction != 0.0f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = ((xThick - 50.f) * cos(a)) + pos.x;
                pt.y = ((xThick - 50.f) * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 4;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.f && target.fraction != 0.f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = ((xThick - 50.f) * cos(a)) + pos.x;
                pt.y = ((xThick - 50.f) * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 4;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.f && target.fraction != 0.f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
        }
        else if (!bhopWasEnabled) {
            config->misc.bunnyHop = true;
            bhopWasEnabled = true;
        }
    }

    else if (config->misc.jumpbugstyle == 1) {

        if (GetAsyncKeyState(config->misc.jumpbugkey) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {

            if (config->misc.bunnyHop) {
                config->misc.bunnyHop = false;
                bhopWasEnabled = false;
            }

            if (unduck) {
                JumpDone = false;
                if (config->misc.jumpbughold)
                    cmd->buttons |= UserCmd::IN_JUMP; // If you want to hold JB key only.
                else
                    cmd->buttons &= ~UserCmd::IN_DUCK;
                unduck = false;
            }

            Vector pos = localPlayer->origin();

            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = (xThick * cos(a)) + pos.x;
                pt.y = (xThick * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.0f && target.fraction != 0.0f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = (xThick * cos(a)) + pos.x;
                pt.y = (xThick * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.f && target.fraction != 0.f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
            for (float a = 0.f; a < max_radius; a += step) {
                Vector pt;
                pt.x = (xThick * cos(a)) + pos.x;
                pt.y = (xThick * sin(a)) + pos.y;
                pt.z = pos.z;

                Vector pt2 = pt;
                pt2.z -= 8192;

                Trace target;

                interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

                if (target.fraction != 1.f && target.fraction != 0.f) {
                    JumpDone = true;
                    cmd->buttons |= UserCmd::IN_DUCK;
                    cmd->buttons &= ~UserCmd::IN_JUMP;
                    unduck = true;
                }
            }
        }
        else if (!bhopWasEnabled) {
            config->misc.bunnyHop = true;
            bhopWasEnabled = true;
        }
    }
 else if (config->misc.jumpbugstyle == 2) {

 if (GetAsyncKeyState(config->misc.jumpbugkey) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {

     if (config->misc.bunnyHop) {
         config->misc.bunnyHop = false;
         bhopWasEnabled = false;
     }

     if (unduck) {
         JumpDone = false;
         if (config->misc.jumpbughold)
             cmd->buttons |= UserCmd::IN_JUMP; // If you want to hold JB key only.
         else
             cmd->buttons &= ~UserCmd::IN_DUCK;
         unduck = false;
     }

     Vector pos = localPlayer->origin();

     for (float a = 0.f; a < max_radius; a += step) {
         Vector pt;
         pt.x = (xThick * cos(a)) + pos.x;
         pt.y = (xThick * sin(a)) + pos.y;
         pt.z = pos.z;

         Vector pt2 = pt;
         pt2.z -= 6;

         Trace target;

         interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

         if (target.fraction != 1.0f && target.fraction != 0.0f) {
             JumpDone = true;
             cmd->buttons |= UserCmd::IN_DUCK;
             cmd->buttons &= ~UserCmd::IN_JUMP;
             unduck = true;
         }
     }
     for (float a = 0.f; a < max_radius; a += step) {
         Vector pt;
         pt.x = (xThick * cos(a)) + pos.x;
         pt.y = (xThick * sin(a)) + pos.y;
         pt.z = pos.z;

         Vector pt2 = pt;
         pt2.z -= 6;

         Trace target;

         interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

         if (target.fraction != 1.f && target.fraction != 0.f) {
             JumpDone = true;
             cmd->buttons |= UserCmd::IN_DUCK;
             cmd->buttons &= ~UserCmd::IN_JUMP;
             unduck = true;
         }
     }
     for (float a = 0.f; a < max_radius; a += step) {
         Vector pt;
         pt.x = (xThick * cos(a)) + pos.x;
         pt.y = (xThick * sin(a)) + pos.y;
         pt.z = pos.z;

         Vector pt2 = pt;
         pt2.z -= 6;

         Trace target;

         interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

         if (target.fraction != 1.f && target.fraction != 0.f) {
             JumpDone = true;
             cmd->buttons |= UserCmd::IN_DUCK;
             cmd->buttons &= ~UserCmd::IN_JUMP;
             unduck = true;
         }
     }
 }
 else if (!bhopWasEnabled) {
     config->misc.bunnyHop = true;
     bhopWasEnabled = true;
 }
    }
}

static constexpr bool worldToScreen(const Vector& in, Vector& out) noexcept
{
    const auto& matrix = interfaces->engine->worldToScreenMatrix();
    float w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;

    if (w > 0.001f) {
        const auto [width, height] = interfaces->surface->getScreenSize();
        out.x = width / 2 * (1 + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w);
        out.y = height / 2 * (1 - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w);
        out.z = 0.0f;
        return true;
    }
    return false;
}

void Misc::removeClientSideChokeLimit() noexcept //may cause vaccccc
{
    static bool once = false;
    if (!once)
    {
        auto clMoveChokeClamp = memory->chokeLimit;

        unsigned long protect = 0;

        VirtualProtect((void*)clMoveChokeClamp, 4, PAGE_EXECUTE_READWRITE, &protect);
        *(std::uint32_t*)clMoveChokeClamp = 62;
        VirtualProtect((void*)clMoveChokeClamp, 4, protect, &protect);
        once = true;
    }
}

void Misc::edgejump(UserCmd* cmd) noexcept
{
    if (!config->misc.edgejump || !GetAsyncKeyState(config->misc.edgejumpkey))
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if ((EnginePrediction::getFlags() & 1) && !(localPlayer->flags() & 1))
        cmd->buttons |= UserCmd::IN_JUMP;
}

void Misc::slowwalk(UserCmd* cmd) noexcept
{
    if (!config->misc.slowwalk || !GetAsyncKeyState(config->misc.slowwalkKey))
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const float maxSpeed = (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;

    if (cmd->forwardmove && cmd->sidemove) {
        const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
    } else if (cmd->forwardmove) {
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
    } else if (cmd->sidemove) {
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
    }
}

void Misc::fastStop(UserCmd* cmd) noexcept
{
    if (!config->misc.fastStop)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER || !(localPlayer->flags() & 1) || cmd->buttons & UserCmd::IN_JUMP)
        return;

    if (cmd->buttons & (UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_FORWARD | UserCmd::IN_BACK))
        return;

    auto VectorAngles = [](const Vector& forward, Vector& angles)
    {
        if (forward.y == 0.0f && forward.x == 0.0f)
        {
            angles.x = (forward.z > 0.0f) ? 270.0f : 90.0f;
            angles.y = 0.0f;
        }
        else
        {
            angles.x = atan2(-forward.z, forward.length2D()) * -180.f / M_PI;
            angles.y = atan2(forward.y, forward.x) * 180.f / M_PI;

            if (angles.y > 90)
                angles.y -= 180;
            else if (angles.y < 90)
                angles.y += 180;
            else if (angles.y == 90)
                angles.y = 0;
        }

        angles.z = 0.0f;
    };
    auto AngleVectors = [](const Vector& angles, Vector* forward)
    {
        float	sp, sy, cp, cy;

        sy = sin(degreesToRadians(angles.y));
        cy = cos(degreesToRadians(angles.y));

        sp = sin(degreesToRadians(angles.x));
        cp = cos(degreesToRadians(angles.x));

        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    };

    Vector velocity = localPlayer->velocity();
    Vector direction;
    VectorAngles(velocity, direction);
    float speed = velocity.length2D();
    if (speed < 15.f)
        return;

    direction.y = cmd->viewangles.y - direction.y;

    Vector forward;
    AngleVectors(direction, &forward);

    Vector negated_direction = forward * speed;

    cmd->forwardmove = negated_direction.x;
    cmd->sidemove = negated_direction.y;
}

void Misc::inverseRagdollGravity() noexcept
{
    static auto ragdollGravity = interfaces->cvar->findVar("cl_ragdoll_gravity");
    ragdollGravity->setValue(config->visuals.inverseRagdollGravity ? -600 : 600);
}

void Misc::clanTag1() noexcept
{
    static std::string clanTag;
    static std::string oldClanTag;

    if (config->misc.clanTag1) {
        switch (int(memory->globalVars->currenttime * 2.4) % 31)
        {
        case 0: clanTag = "moonlight"; break;
        }
    }

    if (!config->misc.clanTag1)
        clanTag = "";

    if (oldClanTag != clanTag) {
        memory->setClanTag(clanTag.c_str(), clanTag.c_str());
        oldClanTag = clanTag;
    }
}

void Misc::clanTag() noexcept
{
    static std::string clanTag;
    static std::string oldClanTag;

    if (config->misc.clanTag) 
    {
        {
            switch (int(memory->globalVars->currenttime * 2.4) % 31)
            {
            case 0: clanTag = encr("M"); break;
            case 1: clanTag = encr("Mi"); break;
            case 2: clanTag = encr("Mid"); break;
            case 3: clanTag = encr("Midl"); break;
            case 4: clanTag = encr("Midli"); break;
            case 5: clanTag = encr("Midlig"); break;
            case 6: clanTag = encr("Midligh"); break;
            case 7: clanTag = encr("Midlight"); break;
            case 8: clanTag = encr("Midlight."); break;
            case 9: clanTag = encr("Midlight.u"); break;
            case 10: clanTag = encr("Midlight.un"); break;
            case 11: clanTag = encr("Midlight.uno"); break;
            case 12: clanTag = encr("Midlight.un"); break;
            case 13: clanTag = encr("Midlight.u"); break;
            case 14: clanTag = encr("Midlight."); break;
            case 15: clanTag = encr("Midligh"); break;
            case 16: clanTag = encr("Midlig"); break;
            case 17: clanTag = encr("Midli"); break;
            case 18: clanTag = encr("Midl"); break;
            case 19: clanTag = encr("Mid"); break;
            case 20: clanTag = encr("Mi"); break;
            case 21: clanTag = encr("M"); break;
            case 22: clanTag = encr(" "); break;
            case 23: clanTag = encr(" "); break;
            }
        }
    }

    if (!config->misc.clanTag)
        clanTag = encr("");

    if (oldClanTag != clanTag) {
        memory->setClanTag(clanTag.c_str(), clanTag.c_str());
        oldClanTag = clanTag;
    }
}

void Misc::spectatorList() noexcept
{
    if (!config->misc.spectatorList)
        return;

    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_WindowBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ChildBg] = ImColor(35, 35, 35, 255);
    style.Colors[ImGuiCol_Border] = ImColor(25, 25, 25, 255);
    style.Colors[ImGuiCol_BorderShadow] = ImColor(50, 50, 50, 255);
    style.Colors[ImGuiCol_Button] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(25, 25, 25, 255);
    style.Colors[ImGuiCol_ScrollbarBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_CheckMark] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_FrameBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_SliderGrab] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_SliderGrabActive] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_PlotLines] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_PlotHistogram] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_TabActive] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_Header] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_HeaderActive] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_HeaderHovered] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_TitleBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_FrameBgActive] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_TitleBgActive] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_PopupBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_TextSelectedBg] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_TextDisabled] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_NavHighlight] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImColor(40, 40, 40, 255);

    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::SetNextWindowSize({ 250.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints({ 250.f, 0.f }, { 250.f, 1000.f });
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!gui->open)
        windowFlags |= ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Spectator list", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (interfaces->engine->isInGame() && localPlayer && localPlayer->isAlive())
    {
            for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
                const auto entity = interfaces->entityList->getEntity(i);
                if (!entity || entity->isDormant() || entity->isAlive() || entity->getObserverTarget() != localPlayer.get())
                    continue;

                PlayerInfo playerInfo;

                if (!interfaces->engine->getPlayerInfo(i, playerInfo))
                    continue;

                auto obsMode{ "" };

                switch (entity->getObserverMode()) {
                case ObsMode::InEye:
                    obsMode = "1st";
                    break;
                case ObsMode::Chase:
                    obsMode = "3rd";
                    break;
                case ObsMode::Roaming:
                    obsMode = "Freecam";
                    break;
                default:
                    continue;
                }

                ImGui::TextWrapped("%s - %s", playerInfo.name, obsMode);
            }
    }
    ImGui::End();
}


void Misc::debugwindow() noexcept
{
    if (!config->misc.debugwindow)
        return;

}




void Misc::sniperCrosshair() noexcept
{
    static auto showSpread = interfaces->cvar->findVar("weapon_debug_spread_show");
    showSpread->setValue(config->misc.sniperCrosshair && localPlayer && !localPlayer->isScoped() ? 3 : 0);
}

void Misc::recoilCrosshair() noexcept
{
    static auto recoilCrosshair = interfaces->cvar->findVar("cl_crosshair_recoil");
    recoilCrosshair->setValue(config->misc.recoilCrosshair ? 1 : 0);
}


void Misc::watermark() noexcept
{
    if (!config->misc.watermark)
        return;
    
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_WindowBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ChildBg] = ImColor(35, 35, 35, 255);
    style.Colors[ImGuiCol_Border] = ImColor(25, 25, 25, 255);
    style.Colors[ImGuiCol_BorderShadow] = ImColor(50, 50, 50, 255);
    style.Colors[ImGuiCol_Button] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(25, 25, 25, 255);
    style.Colors[ImGuiCol_ScrollbarBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_CheckMark] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_FrameBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_SliderGrab] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_SliderGrabActive] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_PlotLines] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_PlotHistogram] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_TabActive] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_Header] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_HeaderActive] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_HeaderHovered] = ImColor(95, 75, 113, 255);
    style.Colors[ImGuiCol_TitleBg] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_FrameBgActive] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_TitleBgActive] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_PopupBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_TextSelectedBg] = ImColor(130, 60, 180, 255);
    style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_TextDisabled] = ImColor(255, 255, 255, 255);
    style.Colors[ImGuiCol_NavHighlight] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImColor(40, 40, 40, 255);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImColor(40, 40, 40, 255);

    ImGui::SetNextWindowBgAlpha(1.f);
    //ImGui::SetNextWindowSizeConstraints({ 0.f, 0.f }, { 1000.f, 300.f });
    ImGui::SetNextWindowSize(ImVec2(200, 20));
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Watermark", nullptr, windowFlags);
    ImGui::PopStyleVar();

    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();

    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;

    float latency = 0.0f;
    if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f) {
        latency = networkChannel->getLatency(0);
    }

    std::string fps{  std::to_string(static_cast<int>(1 / frameRate)) + " fps" };
    std::string ping{ interfaces->engine->isConnected() ? std::to_string(static_cast<int>(latency * 1000)) + " ms" : "Not connected" };

  //ImGui::PushFont(guif::font_smooth);
    ImGui::Text(" Midnight %s | %s ", fps.c_str(), ping.c_str());
  //ImGui::PopFont();
    ImGui::SetWindowPos({ screenWidth - ImGui::GetWindowSize().x - 15, 15 });
    

    ImGui::End();
}

void Misc::fastPlant(UserCmd* cmd) noexcept
{
    if (config->misc.fastPlant) {
        static auto plantAnywhere = interfaces->cvar->findVar("mp_plant_c4_anywhere");

        if (plantAnywhere->getInt())
            return;

        if (!localPlayer || !localPlayer->isAlive() || localPlayer->inBombZone())
            return;

        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (!activeWeapon || activeWeapon->getClientClass()->classId != ClassId::C4)
            return;

        cmd->buttons &= ~UserCmd::IN_ATTACK;

        constexpr float doorRange{ 200.0f };
        Vector viewAngles{ cos(degreesToRadians(cmd->viewangles.x)) * cos(degreesToRadians(cmd->viewangles.y)) * doorRange,
                           cos(degreesToRadians(cmd->viewangles.x)) * sin(degreesToRadians(cmd->viewangles.y)) * doorRange,
                          -sin(degreesToRadians(cmd->viewangles.x)) * doorRange };
        Trace trace;
        interfaces->engineTrace->traceRay({ localPlayer->getEyePosition(), localPlayer->getEyePosition() + viewAngles }, 0x46004009, localPlayer.get(), trace);

        if (!trace.entity || trace.entity->getClientClass()->classId != ClassId::PropDoorRotating)
            cmd->buttons &= ~UserCmd::IN_USE;
    }
}


void Misc::drawBombTimer() noexcept
{
    if (config->misc.bombTimer.enabled) {
        for (int i = interfaces->engine->getMaxClients(); i <= interfaces->entityList->getHighestEntityIndex(); i++) {
            Entity* entity = interfaces->entityList->getEntity(i);
            if (!entity || entity->isDormant() || entity->getClientClass()->classId != ClassId::PlantedC4 || !entity->c4Ticking())
                continue;

            constexpr unsigned font{ 0xc1 };
            interfaces->surface->setTextFont(hooks->verdanaExtraBoldAA);
            interfaces->surface->setTextColor(255, 255, 255);
            auto drawPositionY{ interfaces->surface->getScreenSize().second / 8 };
            std::wostringstream ss; ss << L"Bomb on " << (!entity->c4BombSite() ? 'A' : 'B') << L" : " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(entity->c4BlowTime() - memory->globalVars->currenttime, 0.0f) << L" s";
            auto bombText{ ss.str() };
            const auto bombTextX{ interfaces->surface->getScreenSize().first / 2 - static_cast<int>((interfaces->surface->getTextSize(font, bombText.c_str())).first / 2) };
            interfaces->surface->setTextPosition(bombTextX, drawPositionY);
            drawPositionY += interfaces->surface->getTextSize(font, bombText.c_str()).second;
            interfaces->surface->printText(bombText.c_str());

            //const auto progressBarX{ interfaces->surface->getScreenSize().first / 3 };
           // const auto progressBarLength{ interfaces->surface->getScreenSize().first / 3 };
          //  constexpr auto progressBarHeight{ 5 };

            interfaces->surface->setDrawColor(50, 50, 50);
           // interfaces->surface->drawFilledRect(progressBarX - 3, drawPositionY + 2, progressBarX + progressBarLength + 3, drawPositionY + progressBarHeight + 8);
            interfaces->surface->setDrawColor(config->misc.bombTimer.color);

            static auto c4Timer = interfaces->cvar->findVar("mp_c4timer");

            //interfaces->surface->drawFilledRect(progressBarX, drawPositionY + 5, static_cast<int>(progressBarX + progressBarLength * std::clamp(entity->c4BlowTime() - memory->globalVars->currenttime, 0.0f, c4Timer->getFloat()) / c4Timer->getFloat()), drawPositionY + progressBarHeight + 5);

            if (entity->c4Defuser() != -1) {
                if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(interfaces->entityList->getEntityFromHandle(entity->c4Defuser())->index(), playerInfo)) {
#ifdef _WIN32
                    if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
#else
                    if (wchar_t name[128]; true) {
#endif
                        drawPositionY += interfaces->surface->getTextSize(font, L" ").second;
                        std::wostringstream ss; ss << name << L" is defusing: " << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(entity->c4DefuseCountDown() - memory->globalVars->currenttime, 0.0f) << L" s";
                        const auto defusingText{ ss.str() };

                        interfaces->surface->setTextPosition((interfaces->surface->getScreenSize().first - interfaces->surface->getTextSize(font, defusingText.c_str()).first) / 2, drawPositionY);
                        interfaces->surface->printText(defusingText.c_str());
                        drawPositionY += interfaces->surface->getTextSize(font, L" ").second;

                        interfaces->surface->setDrawColor(50, 50, 50);
                     //   interfaces->surface->drawFilledRect(progressBarX - 3, drawPositionY + 2, progressBarX + progressBarLength + 3, drawPositionY + progressBarHeight + 8);
                        interfaces->surface->setDrawColor(0, 255, 0);
                       // interfaces->surface->drawFilledRect(progressBarX, drawPositionY + 5, progressBarX + static_cast<int>(progressBarLength * (std::max)(entity->c4DefuseCountDown() - memory->globalVars->currenttime, 0.0f) / (interfaces->entityList->getEntityFromHandle(entity->c4Defuser())->hasDefuser() ? 5.0f : 10.0f)), drawPositionY + progressBarHeight + 5);

                        drawPositionY += interfaces->surface->getTextSize(font, L" ").second;
                        const wchar_t* canDefuseText;

                        if (entity->c4BlowTime() >= entity->c4DefuseCountDown()) {
                            canDefuseText = L"Can Defuse";
                            interfaces->surface->setTextColor(0, 255, 0);
                        }
                        else {
                            canDefuseText = L"Cannot Defuse";
                            interfaces->surface->setTextColor(255, 0, 0);
                        }

                        interfaces->surface->setTextPosition((interfaces->surface->getScreenSize().first - interfaces->surface->getTextSize(font, canDefuseText).first) / 2, drawPositionY);
                        interfaces->surface->printText(canDefuseText);
                    }
                    }
                }
            break;
            }
        }
    }

void Misc::disablePanoramablur() noexcept
{
    static auto blur = interfaces->cvar->findVar("@panorama_disable_blur");
    blur->setValue(config->misc.disablePanoramablur);
}

bool Misc::changeName(bool reconnect, const char* newName, float delay) noexcept
{
    static auto exploitInitialized{ false };

    static auto name{ interfaces->cvar->findVar("name") };

    if (reconnect) {
        exploitInitialized = false;
        return false;
    }

    if (!exploitInitialized && interfaces->engine->isInGame()) {
        if (PlayerInfo playerInfo; localPlayer && interfaces->engine->getPlayerInfo(localPlayer->index(), playerInfo) && (!strcmp(playerInfo.name, "?empty") || !strcmp(playerInfo.name, "\n\xAD\xAD\xAD"))) {
            exploitInitialized = true;
        } else {
            name->onChangeCallbacks.size = 0;
            name->setValue("\n\xAD\xAD\xAD");
            return false;
        }
    }

    static auto nextChangeTime{ 0.0f };
    if (nextChangeTime <= memory->globalVars->realtime) {
        name->setValue(newName);
        nextChangeTime = memory->globalVars->realtime + delay;
        return true;
    }
    return false;
}
float flRandom2(float flMin, float flMax) {
    static auto pRandFloat = reinterpret_cast<float(*)(float, float)>(GetProcAddress(GetModuleHandleA("vstdlib.dll"), "RandomFloat"));

    if (!pRandFloat)
        return 0.0f;

    return pRandFloat(flMin, flMax);;
}
void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

        const float flHitchance = config->misc.flBhopChance;

        if (!(cmd->buttons & UserCmd::IN_JUMP) || flHitchance <= 0)
            return;

        if(!(localPlayer->flags() & 1))
            cmd->buttons &= ~UserCmd::IN_JUMP;
        else if (rand() % 100 > flHitchance) {
            cmd->buttons &= ~UserCmd::IN_JUMP;
        }
        else {
            //we hit the hop
        }
}
void Misc::fakeBan(bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    if (shouldSet && interfaces->engine->isInGame() && changeName(false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(config->misc.banColor + 1) }).append(config->misc.banText).append("\x1").c_str(), 5.0f))
        shouldSet = false;
}

void Misc::nadeTrajectory() noexcept
{
    static auto trajectoryVar{ interfaces->cvar->findVar("sv_party_mode") };

    trajectoryVar->onChangeCallbacks.size = 0;
    trajectoryVar->setValue(config->misc.nadeTrajectory);
}



void Misc::showImpacts() noexcept
{
    static auto impactsVar{ interfaces->cvar->findVar("sv_showimpacts") };

    impactsVar->onChangeCallbacks.size = 0;
    impactsVar->setValue(config->misc.showImpacts);
}

void Misc::killSay(GameEvent& event) noexcept
{
    if (!config->misc.killSay)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    std::string cheatNames[] = {
        "2157 is a pedo!",
        "2157 groomed a 15yo and 16yo into sending nudes!",
        "Moonlight is an osiris paste!",
        "2157 lies to his staff and scams friends!"
    };

    srand(time(NULL));
    auto randomMessage = rand() % 3;
    std::string killMessage = "";

    
    switch (randomMessage) 
    {
    case 0:
        killMessage = "2157 scams little kids!";
        break;
    case 1:
        killMessage = "You think moonlight is good? think again.";
        break;
    case 2:
        killMessage = "Get good, Don't get moonlight!";
        break;
    case 3:
        killMessage = "2157 scammed his friends for $40";
        break;
    case 4:
        killMessage = "Moonlight.uno source is now public!";
        break;
    case 5:
        killMessage = "Don't pay for Moonlight you can get it for free!";
        break;
    case 6:
        killMessage = "Midlight > Moonlight";
        break;
    }

    std::string cmd = "say \"";
    cmd += killMessage;
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::chickenDeathSay(GameEvent& event) noexcept
{
    if (!config->misc.chickenDeathSay)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") == localUserId)
        return;

    std::string deathMessages[] = {
        "2157 is a pedo!",
        "2157 groomed a 15yo and 16yo into sending nudes!",
        "Moonlight is an osiris paste!",
        "2157 lies to his staff and scams friends!"
    };

    srand(time(NULL));
    int randomMessage = rand() % ARRAYSIZE(deathMessages);

    std::string cmd = "say \"";
    cmd += deathMessages[randomMessage];
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::fixMovement(UserCmd* cmd, float yaw) noexcept
{
    float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
    float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
    float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
    yawDelta = 360.0f - yawDelta;

    const float forwardmove = cmd->forwardmove;
    const float sidemove = cmd->sidemove;
    cmd->forwardmove = std::cos(degreesToRadians(yawDelta)) * forwardmove + std::cos(degreesToRadians(yawDelta + 90.0f)) * sidemove;
    cmd->sidemove = std::sin(degreesToRadians(yawDelta)) * forwardmove + std::sin(degreesToRadians(yawDelta + 90.0f)) * sidemove;
}

void Misc::antiAfkKick(UserCmd* cmd) noexcept
{
    if (config->misc.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 27;
}

void Misc::prepareRevolver(UserCmd* cmd) noexcept
{
    constexpr auto timeToTicks = [](float time) {  return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };
    constexpr float revolverPrepareTime{ 0.234375f };

    static float readyTime;
    if (config->misc.prepareRevolver && localPlayer && (!config->misc.prepareRevolverKey || GetAsyncKeyState(config->misc.prepareRevolverKey))) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver) {
            if (!readyTime) readyTime = memory->globalVars->serverTime() + revolverPrepareTime;
            auto ticksToReady = timeToTicks(readyTime - memory->globalVars->serverTime() - interfaces->engine->getNetworkChannel()->getLatency(0));
            if (ticksToReady > 0 && ticksToReady <= timeToTicks(revolverPrepareTime))
                cmd->buttons |= UserCmd::IN_ATTACK;
            else
                readyTime = 0.0f;
        }
    }
}



void Misc::autoPistol(UserCmd* cmd) noexcept
{
    if (config->misc.autoPistol && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->isPistol() && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime()) {
            if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
                cmd->buttons &= ~UserCmd::IN_ATTACK2;
            else
                cmd->buttons &= ~UserCmd::IN_ATTACK;
        }
    }
}

void Misc::revealRanks(UserCmd* cmd) noexcept
{
    if (config->misc.revealRanks && cmd->buttons & UserCmd::IN_SCORE)
        interfaces->client->dispatchUserMessage(50, 0, 0, nullptr);
}

float Misc::autoStrafe(UserCmd* cmd, const Vector& currentViewAngles) noexcept
{
    static float angle = 0.f;
    static float direction = 0.f;
    static float AutoStrafeAngle = 0.f;
    if (!config->misc.autoStrafe)
    {
        angle = 0.f;
        direction = 0.f;
        AutoStrafeAngle = 0.f;
        return AutoStrafeAngle;
    }
    if (!localPlayer || !localPlayer->isAlive())
        return 0.f;
    if (localPlayer->velocity().length2D() < 5.f)
    {
        angle = 0.f;
        direction = 0.f;
        AutoStrafeAngle = 0.f;
        return AutoStrafeAngle;
    }
    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER || (!(cmd->buttons & UserCmd::IN_JUMP)))
    {
        angle = 0.f;
        if (cmd->buttons & UserCmd::IN_FORWARD)
        {
            angle = 0.f;
            if (cmd->buttons & UserCmd::IN_MOVELEFT)
            {
                angle = 45.f;
            }
            else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
            {
                angle = -45.f;
            }
        }
        if (!(cmd->buttons & (UserCmd::IN_FORWARD | UserCmd::IN_BACK)))
        {
            if (cmd->buttons & UserCmd::IN_MOVELEFT)
            {
                angle = 90.f;
            }
            if (cmd->buttons & UserCmd::IN_MOVERIGHT)
            {
                angle = -90.f;
            }
        }
        if (cmd->buttons & UserCmd::IN_BACK)
        {
            angle = 180.f;
            if (cmd->buttons & UserCmd::IN_MOVELEFT)
            {
                angle = 135.f;
            }
            else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
            {
                angle = -135.f;
            }
        }
        direction = angle;
        AutoStrafeAngle = 0.f;
        return AutoStrafeAngle;
    }
    Vector base;
    interfaces->engine->getViewAngles(base);
    float delta = std::clamp(radiansToDegrees(std::atan2(15.f, localPlayer->velocity().length2D())), 0.f, 45.f);

    static bool flip = true;
    if (cmd->buttons & (UserCmd::IN_FORWARD | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_BACK))
    {
        cmd->forwardmove = 0;
        cmd->sidemove = 0;
        cmd->upmove = 0;
    }
    angle = 0.f;
    if (cmd->buttons & UserCmd::IN_FORWARD)
    {
        angle = 0.f;
        if (cmd->buttons & UserCmd::IN_MOVELEFT)
        {
            angle = 45.f;
        }
        else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
        {
            angle = -45.f;
        }
    }
    if (!(cmd->buttons & (UserCmd::IN_FORWARD | UserCmd::IN_BACK)))
    {
        if (cmd->buttons & UserCmd::IN_MOVELEFT)
        {
            angle = 90.f;
        }
        if (cmd->buttons & UserCmd::IN_MOVERIGHT)
        {
            angle = -90.f;
        }
    }
    if (cmd->buttons & UserCmd::IN_BACK)
    {
        angle = 180.f;
        if (cmd->buttons & UserCmd::IN_MOVELEFT)
        {
            angle = 135.f;
        }
        else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
        {
            angle = -135.f;
        }
    }
    if (std::abs(direction - angle) <= 180)
    {
        if (direction < angle)
        {
            direction += delta;
        }
        else
        {
            direction -= delta;
        }
    }
    else {
        if (direction < angle)
        {
            direction -= delta;
        }
        else
        {
            direction += delta;
        }
    }
    direction = std::isfinite(direction) ? std::remainder(direction, 360.0f) : 0.0f;
    if (cmd->mousedx < 0)
    {
        cmd->sidemove = -450.0f;
    }
    else if (cmd->mousedx > 0)
    {
        cmd->sidemove = 450.0f;
    }
    flip ? base.y += direction + delta : base.y += direction - delta;
    flip ? AutoStrafeAngle = direction + delta : AutoStrafeAngle = direction - delta;
    if (cmd->viewangles.y == currentViewAngles.y)
    {
        cmd->viewangles.y = base.y;
    }
    flip ? cmd->sidemove = 450.f : cmd->sidemove = -450.f;
    flip = !flip;
    return AutoStrafeAngle;
}

struct customCmd
{
    float forwardmove;
    float sidemove;
    float upmove;
};

bool hasShot;
Vector quickPeekStartPos;
Vector drawPos;
std::vector<customCmd>usercmdQuickpeek;
int qpCount;

void Misc::drawQuickPeekStartPos() noexcept
{
    if (!worldToScreen(quickPeekStartPos, drawPos))
        return;

    if (quickPeekStartPos != Vector{ 0, 0, 0 }) {
        interfaces->surface->setDrawColor(255, 255, 255);
        interfaces->surface->drawCircle(drawPos.x, drawPos.y, 0, 10);
    }
}

void gotoStart(UserCmd* cmd) {
    if (usercmdQuickpeek.empty()) return;
    if (hasShot)
    {
        if (qpCount > 0)
        {
            cmd->upmove = -usercmdQuickpeek.at(qpCount).upmove;
            cmd->sidemove = -usercmdQuickpeek.at(qpCount).sidemove;
            cmd->forwardmove = -usercmdQuickpeek.at(qpCount).forwardmove;
            qpCount--;
        }
    }
    else
    {
        qpCount = usercmdQuickpeek.size();
    }
}



void Misc::removeCrouchCooldown(UserCmd* cmd) noexcept
{
    if (config->misc.fastDuck)
        cmd->buttons |= UserCmd::IN_BULLRUSH;
}

void Misc::moonwalk(UserCmd* cmd) noexcept
{
    if (config->misc.moonwalk && localPlayer && localPlayer->moveType() != MoveType::LADDER)
        cmd->buttons ^= UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT;
}

void Misc::playHitSound(GameEvent& event) noexcept
{
    if (!config->misc.hitSound)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    constexpr std::array hitSounds{
        "play survival/money_collect_04",
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play physics/shield/bullet_hit_shield_01",
        "play physics/glass/glass_impact_bullet1"
    };
    if (config->misc.hitSound == 5)
        interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customHitSound).c_str());
    else if (static_cast<std::size_t>(config->misc.hitSound - 1) < hitSounds.size())
        interfaces->engine->clientCmdUnrestricted(hitSounds[config->misc.hitSound - 1]);
    
}

void Misc::killSound(GameEvent& event) noexcept
{
    if (!config->misc.killSound)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    constexpr std::array killSounds{
        "play survival/money_collect_04",
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play physics/shield/bullet_hit_shield_01",
        "play physics/glass/glass_impact_bullet1"
    };

    if (config->misc.killSound == 5)
        interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customKillSound).c_str());
    else if (static_cast<std::size_t>(config->misc.killSound - 1) < killSounds.size())
        interfaces->engine->clientCmdUnrestricted(killSounds[config->misc.killSound - 1]);
}

void Misc::purchaseList(GameEvent* event) noexcept
{
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    static std::unordered_map<std::string, std::pair<std::vector<std::string>, int>> purchaseDetails;
    static std::unordered_map<std::string, int> purchaseTotal;
    static int totalCost;

    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("item_purchase"): {
            const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

            if (player && localPlayer && memory->isOtherEnemy(player, localPlayer.get())) {
                const auto weaponName = event->getString("weapon");
                auto& purchase = purchaseDetails[player->getPlayerName(true)];

                if (const auto definition = memory->itemSystem()->getItemSchema()->getItemDefinitionByName(weaponName)) {
                    if (const auto weaponInfo = memory->weaponSystem->getWeaponInfo(definition->getWeaponId())) {
                        purchase.second += weaponInfo->price;
                        totalCost += weaponInfo->price;
                    }
                }
                std::string weapon = weaponName;

                if (weapon.starts_with("weapon_"))
                    weapon.erase(0, 7);
                else if (weapon.starts_with("item_"))
                    weapon.erase(0, 5);

                if (weapon.starts_with("smoke"))
                    weapon.erase(5);
                else if (weapon.starts_with("m4a1_s"))
                    weapon.erase(6);
                else if (weapon.starts_with("usp_s"))
                    weapon.erase(5);

                purchase.first.push_back(weapon);
                ++purchaseTotal[weapon];
            }
            break;
        }
        case fnv::hash("round_start"):
            freezeEnd = 0.0f;
            purchaseDetails.clear();
            purchaseTotal.clear();
            totalCost = 0;
            break;
        case fnv::hash("round_freeze_end"):
            freezeEnd = memory->globalVars->realtime;
            break;
        }
    } else {
        

    }
}

void Misc::ShowAccuracy() noexcept
{
    if (config->misc.show_weapon_acc)
        interfaces->cvar->findVar("cl_weapon_debug_show_accuracy")->setValue(2);
    else
        interfaces->cvar->findVar("cl_weapon_debug_show_accuracy")->setValue(0);
}

void Misc::oppositeHandKnife(FrameStage stage) noexcept
{
    if (config->misc.oppositeHandKnife == 0)
        return;

    if (!localPlayer)
        return;

    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static auto hand_value = interfaces->cvar->findVar("cl_righthand");

    if (stage == FrameStage::RENDER_START) 
    {
        if (const auto activeWeapon = localPlayer->getActiveWeapon()) 
        {
            const auto classId = activeWeapon->getClientClass()->classId;

            if (classId == ClassId::Knife || classId == ClassId::KnifeGG)
            {
                if (config->misc.oppositeHandKnife == 1)
                    interfaces->cvar->findVar("cl_righthand")->setValue(0);
                if (config->misc.oppositeHandKnife == 2)
                    interfaces->cvar->findVar("cl_righthand")->setValue(1);
            }
            else if (activeWeapon->getClientClass()->classId; classId != ClassId::Knife || classId != ClassId::KnifeGG)
            {                 
                if (config->misc.oppositeHandKnife == 1)
                    interfaces->cvar->findVar("cl_righthand")->setValue(1);
                if (config->misc.oppositeHandKnife == 2)
                    interfaces->cvar->findVar("cl_righthand")->setValue(0);
            }
        }
    }
}

void Misc::footstepSonar(GameEvent* event) noexcept
{
    if (!config->visuals.footstepBeams)
        return;
    auto entity = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));
    if (!entity || !localPlayer.get())
        return;
    if (entity->team() == localPlayer->team())
        return;
    if (entity == localPlayer.get())
        return;
 //   if (entity->getAbsOrigin() == localPlayer.get()->getAbsOrigin()) // fix for weird bug
//        return;
    if (entity->isDormant())
        return;
    if (!entity->isAlive())
        return; 
    auto model_index = interfaces->modelInfo->getModelIndex("sprites/physbeam.vmt");
    BeamInfo_t info;

    info.m_nType = TE_BEAMRINGPOINT;
    info.m_pszModelName = "sprites/physbeam.vmt";
    info.m_nModelIndex = model_index;
    info.m_nHaloIndex = -1;
    info.m_flHaloScale = 3.0f;
    info.m_flLife = 2.0f;
    info.m_flWidth = config->visuals.footstepBeamThickness;
    info.m_flFadeLength = 1.0f;
    info.m_flAmplitude = 0.0f;
    info.m_flRed = config->visuals.BeamColor.color[0]*255;
    info.m_flGreen = config->visuals.BeamColor.color[1]*255;
    info.m_flBlue = config->visuals.BeamColor.color[2]*255;
    info.m_flBrightness = 255;
    info.m_flSpeed = 0.0f;
    info.m_nStartFrame = 0.0f;
    info.m_flFrameRate = 60.0f;
    info.m_nSegments = -1;
    info.m_nFlags = FBEAM_FADEOUT;
    info.m_vecCenter = entity->getAbsOrigin() + Vector(0.0f, 0.0f, 5.0f);
    info.m_flStartRadius = 5.0f;
    info.m_flEndRadius = config->visuals.footstepBeamRadius;
    info.m_bRenderable = true;

    auto beam_draw = memory->renderBeams->CreateBeamRingPoint(info);

    if (beam_draw)
        memory->renderBeams->DrawBeam(beam_draw);
}
void Misc::preserveDeathNotices(GameEvent* event) noexcept
{
    bool freezeTime = true;
    static int* deathNotice;
    static bool reallocatedDeathNoticeHUD{ false };

    /*

    if (!strcmp(event->getName(), "round_prestart") || !strcmp(event->getName(), "round_end")) {
        if (!strcmp(event->getName(), "round_end")) {
            freezeTime = false;
        } 
        else {
            freezeTime = true;
        }
    }

    if (!strcmp(event->getName(), "round_freeze_end"))
        freezeTime = false;

    if (!strcmp(event->getName(), "round_end"))
        freezeTime = true;

    if (!reallocatedDeathNoticeHUD)
    {
        reallocatedDeathNoticeHUD = true;
        deathNotice = memory->findHudElement(memory->hud, "CCSGO_HudDeathNotice");
    }
    else
    {
        if (deathNotice)
        {
            if (!freezeTime) {
                float* localDeathNotice = (float*)((DWORD)deathNotice + 0x50);
                if (localDeathNotice)
                    *localDeathNotice = config->misc.preserveDeathNotices ? FLT_MAX : 1.5f;
            }

            if (freezeTime && deathNotice - 20)
            {
                if (memory->clearDeathNotices)
                    memory->clearDeathNotices(((DWORD)deathNotice - 20));
            }
        }
    }
    */

}

void Misc::playerBlocker(UserCmd* cmd) noexcept
{
    if (config->misc.playerBlocker && GetAsyncKeyState(config->misc.playerBlockerKey)) {
        float bestdist = 250.f;
        int index = -1;
        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);

            if (!entity)
                continue;

            if (!entity->isAlive() || entity->isDormant() || entity == localPlayer.get())
                continue;

            float dist;

            double distance;
            distance = sqrt(((int)localPlayer->origin().x - (int)entity->origin().x) * ((int)localPlayer->origin().x - (int)entity->origin().x) +
                ((int)localPlayer->origin().y - (int)entity->origin().y) * ((int)localPlayer->origin().y - (int)entity->origin().y) +
                ((int)localPlayer->origin().z - (int)entity->origin().z) * ((int)localPlayer->origin().z - (int)entity->origin().z));
            dist = (float)abs(round(distance));

            if (dist < bestdist)
            {
                bestdist = dist;
                index = i;
            }
        }

        if (index == -1)
            return;

        auto target = interfaces->entityList->getEntity(index);

        if (!target)
            return;


        Vector delta = target->origin() - localPlayer->origin();
        Vector angles{ radiansToDegrees(atan2f(-delta.z, std::hypotf(delta.x, delta.y))), radiansToDegrees(atan2f(delta.y, delta.x)) };
        angles.normalize();

        angles.y -= localPlayer->eyeAngles().y;
        angles.normalize();
        angles.y = std::clamp(angles.y, -180.f, 180.f);

        if (angles.y < -1.0f)
            cmd->sidemove = 450.f;
        else if (angles.y > 1.0f)
            cmd->sidemove = -450.f;

    }
}

void Misc::DDrun(UserCmd* cmd) noexcept
{
    if(config->misc.DDrun && GetAsyncKeyState(config->misc.DDrunKey)) 
    {
        if (cmd->buttons & UserCmd::IN_DUCK && cmd->tickCount & 1)
            cmd->buttons &= ~UserCmd::IN_DUCK;
    }
}

