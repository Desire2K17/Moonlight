#define NOMINMAX

#include <sstream>
#include <locale>
#include <iostream>

#include "Esp.h"
#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Hooks.h"

#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Localize.h"
#include "../SDK/Surface.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponData.h"

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

static void renderSnaplines(Entity* entity, const Config::Esp::Shared& config) noexcept
{
    if (!config.snaplines.enabled)
        return;

    Vector position;
    if (!worldToScreen(entity->getAbsOrigin(), position))
        return;

    if (config.snaplines.rainbow)
        interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.snaplines.rainbowSpeed));
    else
        interfaces->surface->setDrawColor(config.snaplines.color);

    const auto [width, height] = interfaces->surface->getScreenSize();
    interfaces->surface->drawLine(width / 2, height, static_cast<int>(position.x), static_cast<int>(position.y));
}

static void renderEyeTraces(Entity* entity, const Config::Esp::Player& config) noexcept
{
    if (config.eyeTraces.enabled) {
        constexpr float maxRange{ 8192.0f };

        auto eyeAngles = entity->eyeAngles();
        Vector viewAngles{ cos(degreesToRadians(eyeAngles.x)) * cos(degreesToRadians(eyeAngles.y)) * maxRange,
                           cos(degreesToRadians(eyeAngles.x)) * sin(degreesToRadians(eyeAngles.y)) * maxRange,
                          -sin(degreesToRadians(eyeAngles.x)) * maxRange };
        static Trace trace;
        Vector headPosition{ entity->getBonePosition(8) };
        interfaces->engineTrace->traceRay({ headPosition, headPosition + viewAngles }, 0x46004009, { entity }, trace);
        Vector start, end;
        if (worldToScreen(trace.startpos, start) && worldToScreen(trace.endpos, end)) {
            if (config.eyeTraces.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.eyeTraces.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config.eyeTraces.color);

            interfaces->surface->drawLine(start.x, start.y, end.x, end.y);
        }
    }
}

static void renderPositionedText(unsigned font, const wchar_t* text, std::pair<float, float&> position) noexcept
{
    interfaces->surface->setTextFont(font);
    interfaces->surface->setTextPosition(position.first, position.second);
    position.second += interfaces->surface->getTextSize(font, text).second;
    interfaces->surface->printText(text);
}

struct BoundingBox {
    float x0, y0;
    float x1, y1;
    Vector vertices[8];

    BoundingBox(Entity* entity) noexcept
    {
        const auto [width, height] = interfaces->surface->getScreenSize();

        x0 = static_cast<float>(width * 2);
        y0 = static_cast<float>(height * 2);
        x1 = -x0;
        y1 = -y0;

        const auto& mins = entity->getCollideable()->obbMins();
        const auto& maxs = entity->getCollideable()->obbMaxs();

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? maxs.x : mins.x,
                                i & 2 ? maxs.y : mins.y,
                                i & 4 ? maxs.z : mins.z };

            if (!worldToScreen(point.transform(entity->toWorldTransform()), vertices[i])) {
                valid = false;
                return;
            }
            x0 = std::min(x0, vertices[i].x);
            y0 = std::min(y0, vertices[i].y);
            x1 = std::max(x1, vertices[i].x);
            y1 = std::max(y1, vertices[i].y);
        }
        valid = true;
    }

    operator bool() noexcept
    {
        return valid;
    }
private:
    bool valid;
};

static void renderBox(const BoundingBox& bbox, const Config::Esp::Shared& config) noexcept
{
    if (config.box.enabled) {
        if (config.box.rainbow)
            interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.box.rainbowSpeed));
        else
            interfaces->surface->setDrawColor(config.box.color);

        switch (config.boxType) {
        case 0:
            interfaces->surface->drawOutlinedRect(bbox.x0, bbox.y0, bbox.x1, bbox.y1);

            if (config.outline.enabled) {
                if (config.outline.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.outline.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.outline.color);

                interfaces->surface->drawOutlinedRect(bbox.x0 + 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y1 - 1);
                interfaces->surface->drawOutlinedRect(bbox.x0 - 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y1 + 1);
            }
            break;
        case 1:
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);

            if (config.outline.enabled) {
                if (config.outline.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.outline.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.outline.color);

                // TODO: get rid of fabsf()

                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 - 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 - 1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 + 1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);


                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y0 + 1, bbox.x0 + 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 2, bbox.y0 + 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 + 1);


                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, (bbox.y0 + 1));
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + 1, (bbox.y1) - fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);

                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 2, (bbox.x1 - 1), (bbox.y1 - 1) - fabsf(bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 - 1, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0, bbox.x0 + 2, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0, bbox.x1 - 2, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x0 - 1, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0, bbox.x0 + 2, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0, bbox.x1 - 2, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y0 + 1, fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y0 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y1 + 1, fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y1 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y0 + 1, fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y0 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y1 + 1, fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y1 - 2);
            }
            break;
        }
    }
}

static void renderPlayerBox(Entity* entity, const Config::Esp::Player& config) noexcept
{
    if (BoundingBox bbox{ entity }) {
        renderBox(bbox, config);

        float drawPositionLeft = bbox.x0 - 3;
        float drawPositionRight = bbox.x1 + 8;
        float drawPositionBottom = 3.5f;
        float drawPositionBottomEh = 6.5f;
        float drawPositionX = bbox.x0 - 5;


        if (config.healthBar) {
            static auto gameType{ interfaces->cvar->findVar("game_type") };
            static auto survivalMaxHealth{ interfaces->cvar->findVar("sv_dz_player_max_health") };

            const auto maxHealth{ (std::max)((gameType->getInt() == 6 ? survivalMaxHealth->getInt() : 100), entity->health()) };

            interfaces->surface->setDrawColor(0, 0, 0, 180);
            interfaces->surface->drawFilledRect(drawPositionLeft - 3, bbox.y0 - 1, drawPositionLeft + 1, bbox.y1 + 1);

            interfaces->surface->setDrawColor(0, 0, 0);
            interfaces->surface->drawOutlinedRect(drawPositionLeft - 3, bbox.y0 - 1, drawPositionLeft + 1, bbox.y1 + 1);

            if (config.healthBarColor.enabled) {
                if (config.healthBarColor.rainbow)
                    interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.healthBarColor.rainbowSpeed));
                else
                    interfaces->surface->setDrawColor(config.healthBarColor.color);
            }
            else {
                interfaces->surface->setDrawColor(std::min(255 - ((std::max(0, std::min(entity->health(), 100)) * 5.10f)), 255.f), std::min(std::max(0, std::min(entity->health(), 100)) * 2.55f, 255.f), 0);
            }

            interfaces->surface->drawFilledRect(drawPositionLeft - 2, bbox.y0 + abs(bbox.y1 - bbox.y0) * (maxHealth - entity->health()) / static_cast<float>(maxHealth), drawPositionLeft, bbox.y1);

            drawPositionLeft -= 5;
        }

        if (config.armorBar.enabled) {
            interfaces->surface->setDrawColor(0, 0, 0, 180);
            interfaces->surface->drawFilledRect(drawPositionLeft - 3, bbox.y0 - 1, drawPositionLeft + 1, bbox.y1 + 1);

            interfaces->surface->setDrawColor(0, 0, 0);
            interfaces->surface->drawOutlinedRect(drawPositionLeft - 3, bbox.y0 - 1, drawPositionLeft + 1, bbox.y1 + 1);

            if (config.armorBar.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.armorBar.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config.armorBar.color);

            interfaces->surface->drawFilledRect(drawPositionLeft - 2, bbox.y0 + abs(bbox.y1 - bbox.y0) * (100.0f - entity->armor()) / 100.0f, drawPositionLeft, bbox.y1);

            drawPositionLeft -= 5;
        }

        if (config.name.enabled) {
            if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(entity->index(), playerInfo)) {
                if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
                    const auto [width, height] { interfaces->surface->getTextSize(hooks->tahomaAA, name) };
                    interfaces->surface->setTextFont(hooks->tahomaBoldAA);
                    if (config.name.rainbow)
                        interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.name.rainbowSpeed));
                    else
                        interfaces->surface->setTextColor(config.name.color);

                    interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y0 - height);
                    interfaces->surface->printText(name);
                }
            }
        }

        if (const auto activeWeapon{ entity->getActiveWeapon() };  config.activeWeapon.enabled && activeWeapon) {
            const auto name{ interfaces->localize->find(activeWeapon->getWeaponData()->name) };

            std::locale::global(std::locale(""));
            std::wcout.imbue(std::locale());
            auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());
            std::wstring str = name;
            f.toupper(&str[0], &str[0] + str.size());
            //std::wcout << str << std::endl;

            const auto [width, height] { interfaces->surface->getTextSize(hooks->smallFonts, str.c_str()) };
            if (config.activeWeapon.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.activeWeapon.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.activeWeapon.color);

            interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 1);

            interfaces->surface->setTextFont(hooks->tahomaBoldAA);
            interfaces->surface->printText(str);
        }

        float drawPositionY = bbox.y0;

        if (config.health.enabled) {
            if (config.health.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.health.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.health.color);

            renderPositionedText(hooks->tahomaBoldAA, (std::to_wstring(entity->health()) + L" HP").c_str(), { bbox.x1 + 5, drawPositionY });
        }

        if (config.armor.enabled) {
            if (config.armor.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.armor.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.armor.color);

            renderPositionedText(hooks->tahomaBoldAA, (std::to_wstring(entity->armor()) + L" AR").c_str(), { bbox.x1 + 5, drawPositionY });
        }

        if (config.money.enabled) {
            if (config.money.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.money.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.money.color);

            renderPositionedText(hooks->tahomaBoldAA, (L'$' + std::to_wstring(entity->account())).c_str(), { bbox.x1 + 5, drawPositionY });
        }

        if (config.distance.enabled && localPlayer) {
            if (config.distance.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.distance.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.distance.color);

            renderPositionedText(hooks->tahomaBoldAA, (std::wostringstream{ } << std::fixed << std::showpoint << std::setprecision(2) << (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f << L'm').str().c_str(), { bbox.x1 + 5, drawPositionY });
        }
    }
}

static void renderWeaponBox(Entity* entity, const Config::Esp::Weapon& config) noexcept
{
    BoundingBox bbox{ entity };

    if (!bbox)
        return;

    renderBox(bbox, config);

    if (config.name.enabled) {
        const auto name{ interfaces->localize->find(entity->getWeaponData()->name) };

        std::locale::global(std::locale(""));
        std::wcout.imbue(std::locale());
        auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());

        std::wstring str = name;

        f.toupper(&str[0], &str[0] + str.size());
        std::wcout << str << std::endl;

        const auto [width, height] { interfaces->surface->getTextSize(hooks->tahomaBoldAA, str.c_str()) };
        interfaces->surface->setTextFont(hooks->tahomaBoldAA);
        if (config.name.rainbow)
            interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.name.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.name.color);

        interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y0 - 10);

        interfaces->surface->printText(str);
    }
    if (config.ammo.enabled)
    {
        int clip = entity->clip();
        int reserveAmmo = entity->reserveAmmoCount();
        const auto text{ std::to_wstring(clip) + L" / " + std::to_wstring(reserveAmmo) };
        const auto [width, height] { interfaces->surface->getTextSize(hooks->tahomaBoldAA, text.c_str()) };
        interfaces->surface->setTextFont(hooks->tahomaBoldAA);
        if (config.ammo.rainbow)
            interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.ammo.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.ammo.color);
        interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 6);
        if (clip > -1)
        {
            interfaces->surface->printText(text);
        }
    }

    float drawPositionY = bbox.y0;

    if (!localPlayer || !config.distance.enabled)
        return;

    if (config.distance.rainbow)
        interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.distance.rainbowSpeed));
    else
        interfaces->surface->setTextColor(config.distance.color);

    renderPositionedText(hooks->tahomaBoldAA, (std::wostringstream{ } << std::fixed << std::showpoint << std::setprecision(2) << (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f << L'm').str().c_str(), { bbox.x1 + 5, drawPositionY });
}

static void renderEntityBox(Entity* entity, const Config::Esp::Shared& config, const wchar_t* name) noexcept
{
    if (BoundingBox bbox{ entity }) {
        renderBox(bbox, config);

        if (config.name.enabled) {
            std::locale::global(std::locale(""));
            std::wcout.imbue(std::locale());
            auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());

            std::wstring str = name;

            f.toupper(&str[0], &str[0] + str.size());
            std::wcout << str << std::endl;

            interfaces->surface->printText(str);

            const auto [width, height] { interfaces->surface->getTextSize(hooks->tahomaBoldAA, str.c_str()) };
            interfaces->surface->setTextFont(hooks->tahomaBoldAA);
            if (config.name.rainbow)
                interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.name.rainbowSpeed));
            else
                interfaces->surface->setTextColor(config.name.color);

            interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 5);
            interfaces->surface->printText(str);
        }

        float drawPositionY = bbox.y0;

        if (!localPlayer || !config.distance.enabled)
            return;

        if (config.distance.rainbow)
            interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config.distance.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.distance.color);

        renderPositionedText(hooks->tahomaBoldAA, (std::wostringstream{ } << std::fixed << std::showpoint << std::setprecision(2) << (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f << L'm').str().c_str(), { bbox.x1 + 5, drawPositionY });
    }
}

static void renderHeadDot(Entity* entity, const Config::Esp::Player& config) noexcept
{
    if (!config.headDot.enabled)
        return;

    if (!localPlayer)
        return;

    Vector head;
    if (!worldToScreen(entity->getBonePosition(8), head))
        return;

    if (config.headDot.rainbow)
        interfaces->surface->setDrawColor(rainbowColor(memory->globalVars->realtime, config.headDot.rainbowSpeed));
    else
        interfaces->surface->setDrawColor(config.headDot.color);

    interfaces->surface->drawCircle(head.x, head.y, 0, static_cast<int>(100 / std::sqrt((localPlayer->getAbsOrigin() - entity->getAbsOrigin()).length())));
}

static bool isInRange(Entity* entity, float maxDistance) noexcept
{
    if (!localPlayer)
        return false;

    return maxDistance == 0.0f || (entity->getAbsOrigin() - localPlayer->getAbsOrigin()).length() * 0.0254f <= maxDistance;
}

static constexpr bool renderPlayerEsp(Entity* entity, EspId id) noexcept
{
    if (localPlayer && (config->esp.players[id].enabled ||
        config->esp.players[id].deadesp && !localPlayer->isAlive()) &&
        isInRange(entity, config->esp.players[id].maxDistance)) {
        renderSnaplines(entity, config->esp.players[id]);
        renderEyeTraces(entity, config->esp.players[id]);
        renderPlayerBox(entity, config->esp.players[id]);
        renderHeadDot(entity, config->esp.players[id]);
    }
    return config->esp.players[id].enabled;
}

static void renderWeaponEsp(Entity* entity) noexcept
{
    if (config->esp.weapon.enabled && isInRange(entity, config->esp.weapon.maxDistance)) {
        renderWeaponBox(entity, config->esp.weapon);
        renderSnaplines(entity, config->esp.weapon);
    }
}

static constexpr void renderEntityEsp(Entity* entity, const Config::Esp::Shared& config, const wchar_t* name) noexcept
{
    if (config.enabled && isInRange(entity, config.maxDistance)) {
        renderEntityBox(entity, config, name);
        renderSnaplines(entity, config);
    }
}

void Esp::render() noexcept
{
    if (interfaces->engine->isInGame()) {
        if (!localPlayer)
            return;

        const auto observerTarget = localPlayer->getObserverTarget();

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity == observerTarget
                || entity->isDormant() || !entity->isAlive())
                continue;

            if (!entity->isOtherEnemy(localPlayer.get())) {
                if (!renderPlayerEsp(entity, ALLIES_ALL)) {
                    if (entity->isVisible())
                        renderPlayerEsp(entity, ALLIES_VISIBLE);
                    else
                        renderPlayerEsp(entity, ALLIES_OCCLUDED);
                }
            }
            else if (!renderPlayerEsp(entity, ENEMIES_ALL)) {
                if (entity->isVisible())
                    renderPlayerEsp(entity, ENEMIES_VISIBLE);
                else
                    renderPlayerEsp(entity, ENEMIES_OCCLUDED);
            }
        }

        for (int i = interfaces->engine->getMaxClients() + 1; i <= interfaces->entityList->getHighestEntityIndex(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity->isDormant())
                continue;

            if (entity->isWeapon() && entity->ownerEntity() == -1)
                renderWeaponEsp(entity);
            else {
                switch (entity->getClientClass()->classId) {
                case ClassId::Dronegun: {
                    renderEntityEsp(entity, config->esp.dangerZone[0], std::wstring{ L"AUTO SENTRY" }.append(L" ").append(std::to_wstring(entity->sentryHealth())).append(L" HP").c_str());
                    break;
                }
                case ClassId::Drone: {
                    std::wstring text{ L"DRONE" };
                    if (const auto tablet{ interfaces->entityList->getEntityFromHandle(entity->droneTarget()) }) {
                        if (const auto player{ interfaces->entityList->getEntityFromHandle(tablet->ownerEntity()) }) {
                            if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(player->index(), playerInfo)) {
                                if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
                                    std::locale::global(std::locale(""));
                                    std::wcout.imbue(std::locale());
                                    auto& f = std::use_facet<std::ctype<wchar_t>>(std::locale());

                                    std::wstring str = name;

                                    f.toupper(&str[0], &str[0] + str.size());
                                    std::wcout << str << std::endl;

                                    text += L" ";
                                    text += str;
                                }
                            }
                        }
                    }
                    renderEntityEsp(entity, config->esp.dangerZone[1], text.c_str());
                    break;
                }
                case ClassId::Cash:
                    renderEntityEsp(entity, config->esp.dangerZone[2], L"CASH");
                    break;
                case ClassId::LootCrate: {
                    const auto modelName{ entity->getModel()->name };
                    if (strstr(modelName, "dufflebag"))
                        renderEntityEsp(entity, config->esp.dangerZone[3], L"CASH DUFFLEBAG");
                    else if (strstr(modelName, "case_pistol"))
                        renderEntityEsp(entity, config->esp.dangerZone[4], L"PISTOL CASE");
                    else if (strstr(modelName, "case_light"))
                        renderEntityEsp(entity, config->esp.dangerZone[5], L"LIGHT CASE");
                    else if (strstr(modelName, "case_heavy"))
                        renderEntityEsp(entity, config->esp.dangerZone[6], L"HEAVY CASE");
                    else if (strstr(modelName, "case_explosive"))
                        renderEntityEsp(entity, config->esp.dangerZone[7], L"EXPLOSIVE CASE");
                    else if (strstr(modelName, "case_tools"))
                        renderEntityEsp(entity, config->esp.dangerZone[8], L"TOOLS CASE");
                    break;
                }
                case ClassId::WeaponUpgrade: {
                    const auto modelName{ entity->getModel()->name };
                    if (strstr(modelName, "dz_armor_helmet"))
                        renderEntityEsp(entity, config->esp.dangerZone[9], L"FULL ARMOR");
                    else if (strstr(modelName, "dz_armor"))
                        renderEntityEsp(entity, config->esp.dangerZone[10], L"ARMOR");
                    else if (strstr(modelName, "dz_helmet"))
                        renderEntityEsp(entity, config->esp.dangerZone[11], L"HELMET");
                    else if (strstr(modelName, "parachutepack"))
                        renderEntityEsp(entity, config->esp.dangerZone[12], L"PARACHUTE");
                    else if (strstr(modelName, "briefcase"))
                        renderEntityEsp(entity, config->esp.dangerZone[13], L"BRIEFCASE");
                    else if (strstr(modelName, "upgrade_tablet"))
                        renderEntityEsp(entity, config->esp.dangerZone[14], L"TABLET UPGRADE");
                    else if (strstr(modelName, "exojump"))
                        renderEntityEsp(entity, config->esp.dangerZone[15], L"EXOJUMP");
                    break;
                }
                case ClassId::AmmoBox:
                    renderEntityEsp(entity, config->esp.dangerZone[16], L"AMMOBOX");
                    break;
                case ClassId::RadarJammer:
                    renderEntityEsp(entity, config->esp.dangerZone[17], L"TABLET JAMMER");
                    break;
                case ClassId::BaseCSGrenadeProjectile:
                    if (strstr(entity->getModel()->name, "flashbang"))
                        renderEntityEsp(entity, config->esp.projectiles[0], L"FLASHBANG");
                    else
                        renderEntityEsp(entity, config->esp.projectiles[1], L"Grenade");
                    break;
                case ClassId::DecoyProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[4], L"DECOY");
                    break;
                case ClassId::MolotovProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[5], L"MOLOTOV");
                    break;
                case ClassId::SmokeGrenadeProjectile:
                    renderEntityEsp(entity, config->esp.projectiles[7], L"SMOKE");
                    break;
                }
            }
        }
    }
}
