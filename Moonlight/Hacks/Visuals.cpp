#include "../fnv.h"
#include "Visuals.h"

#include "Animations.h"
#include "Resolver.h"
#include "Ragebot.h"

#include "../Helpers.h"
#include "../SDK/NetworkStringTable.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"

#include "../SDK/RenderContext.h"
#include "../SDK/Surface.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/Beams.h"
#include "../Hooks.h"

#include <array>

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

struct Color {
    std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

void Visuals::nightMode() noexcept
{
    static auto drawSpecificStaticProp = interfaces->cvar->findVar("r_drawspecificstaticprop");
    if (drawSpecificStaticProp->getInt() != config->visuals.nightMode)
        drawSpecificStaticProp->setValue(config->visuals.nightMode);

    static Color null = { 0.0f, 0.0f, 0.0f, 0.0f };

    static Color
        lastWorldColor = null,
        lastPropColor = null,
        lastSkyboxColor = null;
    static Color
        worldColor = null,
        propColor = null,
        skyboxColor = null;
    static float r = .06f, g = .06f, b = .06f;

    if (config->visuals.nightModeOverride.enabled) {
        r = config->visuals.nightModeOverride.color[0];
        g = config->visuals.nightModeOverride.color[1];
        b = config->visuals.nightModeOverride.color[2];
    }
    else {
        r = .06f, g = .06f, b = .06f;
    }

    if (config->visuals.nightMode) {
        worldColor = { r, g, b, 1.0f };
        propColor = { -((r - 1.f) * (r - 1.f)) * 0.8f + 1.f, -((g - 1.f) * (g - 1.f)) * 0.8f + 1.f, -((b - 1.f) * (b - 1.f)) * 0.8f + 1.f, 1.0f };
        skyboxColor = { -((r - 1.f) * (r - 1.f)) * 0.8f + 1.f, -((g - 1.f) * (g - 1.f)) * 0.8f + 1.f, -((b - 1.f) * (b - 1.f)) * 0.8f + 1.f, 1.0f };
    }
    else {
        worldColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        propColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        skyboxColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    }
    worldColor.color[3] = config->visuals.asusWalls;
    propColor.color[3] = config->visuals.asusProps;

    bool didColorChange = (worldColor.color != lastWorldColor.color) ||
        (propColor.color != lastPropColor.color) ||
        (skyboxColor.color != lastSkyboxColor.color);

    static bool wasInGame = false;
    if (!interfaces->engine->isInGame())
        wasInGame = false;
    if (!wasInGame && interfaces->engine->isInGame())
    {
        didColorChange = true;
        wasInGame = true;
    }

    if (!didColorChange)
        return;

    for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h)) {
        const auto mat = interfaces->materialSystem->getMaterial(h);

        if (!mat)
            continue;

        const std::string_view textureGroup = mat->getTextureGroupName();

        if (textureGroup.starts_with("World")) {
            if (config->visuals.nightModeOverride.rainbow && config->visuals.nightModeOverride.enabled)
                mat->colorModulate(rainbowColor(memory->globalVars->realtime, config->visuals.nightModeOverride.rainbowSpeed));
            else
                mat->colorModulate(worldColor.color[0], worldColor.color[1], worldColor.color[2]);
            mat->alphaModulate((100.f - worldColor.color[3]) / 100.f);
            lastWorldColor = worldColor;
        }
        else if (textureGroup.starts_with("StaticProp") || textureGroup.starts_with("Prop")) {
            if (config->visuals.nightModeOverride.rainbow && config->visuals.nightModeOverride.enabled)
                mat->colorModulate(rainbowColor(memory->globalVars->realtime, config->visuals.nightModeOverride.rainbowSpeed));
            else
                mat->colorModulate(propColor.color[0], propColor.color[1], propColor.color[2]);
            mat->alphaModulate((100.f - propColor.color[3]) / 100.f);
            lastPropColor = propColor;
        }
        else if (textureGroup.starts_with("SkyBox"))
        {
            mat->colorModulate(skyboxColor.color[0], skyboxColor.color[1], skyboxColor.color[2]);
            lastSkyboxColor = skyboxColor;
        }
    }
}

void Visuals::playerModel(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static int originalIdx = 0;

    if (!localPlayer) {
        originalIdx = 0;
        return;
    }
    static std::string szLastModel = "";
    constexpr auto getModel = [](int team) constexpr noexcept -> const char* {
        constexpr std::array models{
"models/player/custom_player/legacy/ctm_diver_varianta.mdl", // Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman
"models/player/custom_player/legacy/ctm_diver_variantb.mdl", // Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman
"models/player/custom_player/legacy/ctm_diver_variantc.mdl", // Lieutenant Rex Krikey | SEAL Frogman
"models/player/custom_player/legacy/ctm_fbi_varianth.mdl", // Michael Syfers | FBI Sniper
"models/player/custom_player/legacy/ctm_fbi_variantf.mdl", // Operator | FBI SWAT
"models/player/custom_player/legacy/ctm_fbi_variantb.mdl", // Special Agent Ava | FBI
"models/player/custom_player/legacy/ctm_fbi_variantg.mdl", // Markus Delrow | FBI HRT
"models/player/custom_player/legacy/ctm_gendarmerie_varianta.mdl", // Sous-Lieutenant Medic | Gendarmerie Nationale
"models/player/custom_player/legacy/ctm_gendarmerie_variantb.mdl", // Chem-Haz Capitaine | Gendarmerie Nationale
"models/player/custom_player/legacy/ctm_gendarmerie_variantc.mdl", // Chef d'Escadron Rouchard | Gendarmerie Nationale
"models/player/custom_player/legacy/ctm_gendarmerie_variantd.mdl", // Aspirant | Gendarmerie Nationale
"models/player/custom_player/legacy/ctm_gendarmerie_variante.mdl", // Officer Jacques Beltram | Gendarmerie Nationale
"models/player/custom_player/legacy/ctm_sas_variantg.mdl", // D Squadron Officer | NZSAS
"models/player/custom_player/legacy/ctm_sas_variantf.mdl", // B Squadron Officer | SAS
"models/player/custom_player/legacy/ctm_st6_variante.mdl", // Seal Team 6 Soldier | NSWC SEAL
"models/player/custom_player/legacy/ctm_st6_variantg.mdl", // Buckshot | NSWC SEAL
"models/player/custom_player/legacy/ctm_st6_varianti.mdl", // Lt. Commander Ricksaw | NSWC SEAL
"models/player/custom_player/legacy/ctm_st6_variantj.mdl", // 'Blueberries' Buckshot | NSWC SEAL
"models/player/custom_player/legacy/ctm_st6_variantk.mdl", // 3rd Commando Company | KSK
"models/player/custom_player/legacy/ctm_st6_variantl.mdl", // 'Two Times' McCoy | TACP Cavalry
"models/player/custom_player/legacy/ctm_st6_variantm.mdl", // 'Two Times' McCoy | USAF TACP
"models/player/custom_player/legacy/ctm_st6_variantn.mdl", // Primeiro Tenente | Brazilian 1st Battalion
"models/player/custom_player/legacy/ctm_swat_variante.mdl", // Cmdr. Mae 'Dead Cold' Jamison | SWAT
"models/player/custom_player/legacy/ctm_swat_variantf.mdl", // 1st Lieutenant Farlow | SWAT
"models/player/custom_player/legacy/ctm_swat_variantg.mdl", // John 'Van Healen' Kask | SWAT
"models/player/custom_player/legacy/ctm_swat_varianth.mdl", // Bio-Haz Specialist | SWAT
"models/player/custom_player/legacy/ctm_swat_varianti.mdl", // Sergeant Bombson | SWAT
"models/player/custom_player/legacy/ctm_swat_variantj.mdl", // Chem-Haz Specialist | SWAT
"models/player/custom_player/legacy/ctm_swat_variantk.mdl" // Lieutenant 'Tree Hugger' Farlow | SWAT
"models/player/custom_player/legacy/tm_professional_varj.mdl", // Getaway Sally | The Professionals
"models/player/custom_player/legacy/tm_professional_vari.mdl", // Number K | The Professionals
"models/player/custom_player/legacy/tm_professional_varh.mdl", // Little Kev | The Professionals
"models/player/custom_player/legacy/tm_professional_varg.mdl", // Safecracker Voltzmann | The Professionals
"models/player/custom_player/legacy/tm_professional_varf5.mdl", // Bloody Darryl The Strapped | The Professionals
"models/player/custom_player/legacy/tm_professional_varf4.mdl", // Sir Bloody Loudmouth Darryl | The Professionals
"models/player/custom_player/legacy/tm_professional_varf3.mdl", // Sir Bloody Darryl Royale | The Professionals
"models/player/custom_player/legacy/tm_professional_varf2.mdl", // Sir Bloody Skullhead Darryl | The Professionals
"models/player/custom_player/legacy/tm_professional_varf1.mdl", // Sir Bloody Silent Darryl | The Professionals
"models/player/custom_player/legacy/tm_professional_varf.mdl", // Sir Bloody Miami Darryl | The Professionals
"models/player/custom_player/legacy/tm_phoenix_varianti.mdl", // Street Soldier | Phoenix
"models/player/custom_player/legacy/tm_phoenix_varianth.mdl", // Soldier | Phoenix
"models/player/custom_player/legacy/tm_phoenix_variantg.mdl", // Slingshot | Phoenix
"models/player/custom_player/legacy/tm_phoenix_variantf.mdl", // Enforcer | Phoenix
"models/player/custom_player/legacy/tm_leet_variantj.mdl", // Mr. Muhlik | Elite Crew
"models/player/custom_player/legacy/tm_leet_varianti.mdl", // Prof. Shahmat | Elite Crew
"models/player/custom_player/legacy/tm_leet_varianth.mdl", // Osiris | Elite Crew
"models/player/custom_player/legacy/tm_leet_variantg.mdl", // Ground Rebel | Elite Crew
"models/player/custom_player/legacy/tm_leet_variantf.mdl", // The Elite Mr. Muhlik | Elite Crew
"models/player/custom_player/legacy/tm_jungle_raider_variantf2.mdl", // Trapper | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_variantf.mdl", // Trapper Aggressor | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_variante.mdl", // Vypa Sista of the Revolution | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_variantd.mdl", // Col. Mangos Dabisi | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_variantс.mdl", // Arno The Overgrown | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_variantb2.mdl", // 'Medium Rare' Crasswater | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_variantb.mdl", // Crasswater The Forgotten | Guerrilla Warfare
"models/player/custom_player/legacy/tm_jungle_raider_varianta.mdl", // Elite Trapper Solman | Guerrilla Warfare
"models/player/custom_player/legacy/tm_balkan_varianth.mdl", // 'The Doctor' Romanov | Sabre
"models/player/custom_player/legacy/tm_balkan_variantj.mdl", // Blackwolf | Sabre
"models/player/custom_player/legacy/tm_balkan_varianti.mdl", // Maximus | Sabre
"models/player/custom_player/legacy/tm_balkan_variantf.mdl", // Dragomir | Sabre
"models/player/custom_player/legacy/tm_balkan_variantg.mdl", // Rezan The Ready | Sabre
"models/player/custom_player/legacy/tm_balkan_variantk.mdl", // Rezan the Redshirt | Sabre
"models/player/custom_player/legacy/tm_balkan_variantl.mdl", // Dragomir | Sabre Footsoldier
        };

        switch (team) {
        case 2: return static_cast<std::size_t>(config->visuals.playerModelCT - 1) < models.size() ? models[config->visuals.playerModelCT - 1] : nullptr;
        case 3: return static_cast<std::size_t>(config->visuals.playerModelT - 1) < models.size() ? models[config->visuals.playerModelT - 1] : nullptr;
        default: return nullptr;
        }
    };

    if (const auto model = getModel(localPlayer->team())) {
        if (stage == FrameStage::RENDER_START)
            originalIdx = localPlayer->modelIndex();

        if (model != szLastModel) {
            szLastModel = model;//this might need fix.!
            SkinChanger::scheduleHudUpdate();
        }

        const auto idx = stage == FrameStage::RENDER_END && originalIdx ? originalIdx : interfaces->modelInfo->getModelIndex(model);

        localPlayer->setModelIndex(idx);

        if (const auto ragdoll = interfaces->entityList->getEntityFromHandle(localPlayer->ragdoll()))
            ragdoll->setModelIndex(idx);
    }
}


void Visuals::modifySmoke(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr std::array smokeMaterials{
        "particle/vistasmokev1/vistasmokev1_emods",
        "particle/vistasmokev1/vistasmokev1_emods_impactdust",
        "particle/vistasmokev1/vistasmokev1_fire",
        "particle/vistasmokev1/vistasmokev1_smokegrenade"
    };

    for (const auto mat : smokeMaterials) {
        const auto material = interfaces->materialSystem->findMaterial(mat);
        material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noSmoke);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeSmoke);
    }
}

void Visuals::thirdperson() noexcept
{
    static bool isInThirdperson{ false };
    static float lastTime{ 0.0f };

    if (GetAsyncKeyState(config->visuals.thirdpersonKey) && memory->globalVars->realtime - lastTime > 0.5f) {
        isInThirdperson = !isInThirdperson;
        lastTime = memory->globalVars->realtime;
    }

    if (config->visuals.thirdperson)
        if (memory->input->isCameraInThirdPerson = (!config->visuals.thirdpersonKey || isInThirdperson)
            && localPlayer && localPlayer->isAlive())
            memory->input->cameraOffset.z = static_cast<float>(config->visuals.thirdpersonDistance);
}

void Visuals::removeVisualRecoil(FrameStage stage) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    static Vector aimPunch;
    static Vector viewPunch;

    if (stage == FrameStage::RENDER_START) {
        aimPunch = localPlayer->aimPunchAngle();
        viewPunch = localPlayer->viewPunchAngle();

        if (config->visuals.noAimPunch && !config->misc.recoilCrosshair)
            localPlayer->aimPunchAngle() = Vector{ };

        if (config->visuals.noViewPunch)
            localPlayer->viewPunchAngle() = Vector{ };

    } else if (stage == FrameStage::RENDER_END) {
        localPlayer->aimPunchAngle() = aimPunch;
        localPlayer->viewPunchAngle() = viewPunch;
    }
}

void Visuals::removeBlur(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static auto blur = interfaces->materialSystem->findMaterial("dev/scope_bluroverlay");
    blur->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noBlur);
}

void Visuals::updateBrightness() noexcept
{
    static auto brightness = interfaces->cvar->findVar("mat_force_tonemap_scale");
    brightness->setValue(config->visuals.brightness);
}

void Visuals::removeGrass(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr auto getGrassMaterialName = []() constexpr noexcept -> const char* {
        switch (fnv::hashRuntime(interfaces->engine->getLevelName())) {
        case fnv::hash("dz_blacksite"): return "detail/detailsprites_survival";
        case fnv::hash("dz_sirocco"): return "detail/dust_massive_detail_sprites";
        case fnv::hash("dz_junglety"): return "detail/tropical_grass";
        default: return nullptr;
        }
    };

    if (const auto grassMaterialName = getGrassMaterialName())
        interfaces->materialSystem->findMaterial(grassMaterialName)->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noGrass);
}

void Visuals::remove3dSky() noexcept
{
    static auto sky = interfaces->cvar->findVar("r_3dsky");
    sky->setValue(!config->visuals.no3dSky);
}

void Visuals::removeShadows() noexcept
{
    static auto shadows = interfaces->cvar->findVar("cl_csm_enabled");
    shadows->setValue(!config->visuals.noShadows);
}

#define DRAW_SCREEN_EFFECT(material) \
{ \
    const auto drawFunction = memory->drawScreenEffectMaterial; \
    int w, h; \
    interfaces->surface->getScreenSize(w, h); \
    __asm { \
        __asm push h \
        __asm push w \
        __asm push 0 \
        __asm xor edx, edx \
        __asm mov ecx, material \
        __asm call drawFunction \
        __asm add esp, 12 \
    } \
}

void Visuals::applyScreenEffects() noexcept
{
    if (!config->visuals.screenEffect)
        return;

    const auto material = interfaces->materialSystem->findMaterial([] {
        constexpr std::array effects{
            "effects/dronecam",
            "effects/underwater_overlay",
            "effects/healthboost",
            "effects/dangerzone_screen"
        };

        if (config->visuals.screenEffect <= 2 || static_cast<std::size_t>(config->visuals.screenEffect - 2) >= effects.size())
            return effects[0];
        return effects[config->visuals.screenEffect - 2];
    }());

    if (config->visuals.screenEffect == 1)
        material->findVar("$c0_x")->setValue(0.0f);
    else if (config->visuals.screenEffect == 2)
        material->findVar("$c0_x")->setValue(0.1f);
    else if (config->visuals.screenEffect >= 4)
        material->findVar("$c0_x")->setValue(1.0f);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::hitEffect(GameEvent* event) noexcept
{
    if (config->visuals.hitEffect && localPlayer) {
        static float lastHitTime = 0.0f;

        if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index()) {
            lastHitTime = memory->globalVars->realtime;
            return;
        }

        if (lastHitTime + config->visuals.hitEffectTime >= memory->globalVars->realtime) {
            constexpr auto getEffectMaterial = [] {
                static constexpr const char* effects[]{
                "effects/dronecam",
                "effects/underwater_overlay",
                "effects/healthboost",
                "effects/dangerzone_screen"
                };

                if (config->visuals.hitEffect <= 2)
                    return effects[0];
                return effects[config->visuals.hitEffect - 2];
            };

           
            auto material = interfaces->materialSystem->findMaterial(getEffectMaterial());
            if (config->visuals.hitEffect == 1)
                material->findVar("$c0_x")->setValue(0.0f);
            else if (config->visuals.hitEffect == 2)
                material->findVar("$c0_x")->setValue(0.1f);
            else if (config->visuals.hitEffect >= 4)
                material->findVar("$c0_x")->setValue(1.0f);

            DRAW_SCREEN_EFFECT(material)
        }
    }
}
void Visuals::fovCircle() noexcept 
{
   
    if (!localPlayer || !localPlayer->isAlive() || !config->visuals.fovCircle)
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    auto weaponClass = getWeaponClass1(activeWeapon->itemDefinitionIndex2());
    if (!config->legitbot[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!config->legitbot[weaponIndex].enabled)
        weaponIndex = 0;
    
    if (!config->legitbot[weaponIndex].enabled)
        return;

    //make these hoes const if theres problems
    const float flCfgFov = config->legitbot[weaponIndex].fov;
    const float flSilentFov = config->legitbot[weaponIndex].silentFov;
    if (flCfgFov > 0 && flCfgFov < 90) {
        //add a small value to make the fov circle appear more accurate.
        //flCfgFov += 0.1f;

        //draw circle
        const auto [width, height] = interfaces->surface->getScreenSize();

        float flRadius = flCfgFov / 90.0f * width / 2;
        float flRadius2 = flSilentFov / 90.0f * width / 2;

        interfaces->surface->setDrawColor(255, 255, 255, 255);
        interfaces->surface->drawOutlinedCircle(width / 2, height / 2, flRadius - 1, flRadius);
       // if (!config->legitbot[weaponIndex].silent)
            return;
        interfaces->surface->setDrawColor(255, 0, 0, 255);
        interfaces->surface->drawOutlinedCircle(width / 2, height / 2, flRadius2 - 1, flRadius2);
    }
}
void Visuals::hitMarker(GameEvent* event) noexcept
{
    if (config->visuals.hitMarker == 0 || !localPlayer)
        return;

    static float alpha = 0.f;

    if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index()) {
        alpha = 255.f;
        return;
    }

    if (alpha < 1.f)
        return;

    switch (config->visuals.hitMarker) {
    case 1:
        const auto [width, height] = interfaces->surface->getScreenSize();

        const auto width_mid = width / 2;
        const auto height_mid = height / 2;

        interfaces->surface->setDrawColor(255, 255, 255, alpha);
        interfaces->surface->drawLine(width_mid + 10, height_mid + 10, width_mid + 4, height_mid + 4);
        interfaces->surface->drawLine(width_mid - 10, height_mid + 10, width_mid - 4, height_mid + 4);
        interfaces->surface->drawLine(width_mid + 10, height_mid - 10, width_mid + 4, height_mid - 4);
        interfaces->surface->drawLine(width_mid - 10, height_mid - 10, width_mid - 4, height_mid - 4);
        alpha *= .95f;
        break;
    }
}

void Visuals::disablePostProcessing(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    *memory->disablePostProcessing = stage == FrameStage::RENDER_START && config->visuals.disablePostProcessing;
}

void Visuals::reduceFlashEffect() noexcept
{
    if (localPlayer)
        localPlayer->flashMaxAlpha() = 255.0f - config->visuals.flashReduction * 2.55f;
}


bool Visuals::removeHands(const char* modelName) noexcept
{
    return config->visuals.noHands && std::strstr(modelName, "arms") && !std::strstr(modelName, "sleeve");
}

bool Visuals::removeSleeves(const char* modelName) noexcept
{
    return config->visuals.noSleeves && std::strstr(modelName, "sleeve");
}

bool Visuals::removeWeapons(const char* modelName) noexcept
{
    return config->visuals.noWeapons && std::strstr(modelName, "models/weapons/v_")
        && !std::strstr(modelName, "arms") && !std::strstr(modelName, "tablet")
        && !std::strstr(modelName, "parachute") && !std::strstr(modelName, "fists");
}

void Visuals::skybox(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;
    static ConVar* r_3dsky = interfaces->cvar->findVar("r_3dsky");
    r_3dsky->setValue(0);
    if (config->visuals.skybox == 1) {
        memory->loadSky(config->visuals.customSkybox.c_str());
    }
    else if ((config->visuals.skybox) > 1 && static_cast<std::size_t>(config->visuals.skybox) < skyboxes.size()) {
        memory->loadSky(skyboxes[config->visuals.skybox]);
    }
    else {
        r_3dsky->setValue(1);
        static const auto sv_skyname = interfaces->cvar->findVar("sv_skyname");
        memory->loadSky(sv_skyname->string);
    }
}

void Visuals::viewmodelxyz() noexcept
{
    if (!localPlayer)
        return;
    float config_x = config->visuals.viewmodel_x;
    float config_y = config->visuals.viewmodel_y;
    float config_z = config->visuals.viewmodel_z;


    static ConVar* viewmodel_x = interfaces->cvar->findVar("viewmodel_offset_x");
    static ConVar* viewmodel_y = interfaces->cvar->findVar("viewmodel_offset_y");
    static ConVar* viewmodel_z = interfaces->cvar->findVar("viewmodel_offset_z");

    static ConVar* cl_righthand = interfaces->cvar->findVar("cl_righthand");
    bool config_righthand = config->visuals.viewmodel_clright;

    static ConVar* sv_minspec = interfaces->cvar->findVar("sv_competitive_minspec");

    bool sv_minspec_toggle = false;

    if (config->visuals.viewmodelxyz) {
        sv_minspec_toggle = true;
        viewmodel_x->setValue(config_x);
        viewmodel_y->setValue(config_y);
        viewmodel_z->setValue(config_z);
        //cl_righthand->setValue(config_righthand);
    }
    else {
        sv_minspec_toggle = false;
        viewmodel_x->setValue(1);
        viewmodel_y->setValue(1);
        viewmodel_z->setValue(-1);
        //cl_righthand->setValue(1);
    }
    if (sv_minspec_toggle) {
        *(int*)((DWORD)&sv_minspec->onChangeCallbacks + 0xC) = 0;
        sv_minspec->setValue(0);
    }
    else {
        sv_minspec->setValue(1);
    }

}

void Visuals::bulletBeams(GameEvent* event) noexcept
{
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected() || memory->renderBeams == nullptr)
        return;

    const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

    if (!player || !localPlayer || player->isDormant() || !player->isAlive())
        return;

    if (player == localPlayer.get() && !config->visuals.bulletTracersLocal.enabled) 
        return;

    if (localPlayer->isOtherEnemy(player) && !config->visuals.bulletTracersEnemy.enabled)
        return;

    if (!localPlayer->isOtherEnemy(player) && player != localPlayer.get() && !config->visuals.bulletTracersAllies.enabled)
        return;

    Vector position;
    position.x = event->getFloat("x");
    position.y = event->getFloat("y");
    position.z = event->getFloat("z");

    BeamInfo_t beam_info;
    beam_info.m_nType = TE_BEAMPOINTS;
    beam_info.m_pszModelName = "sprites/purplelaser1.vmt";
    beam_info.m_nModelIndex = -1;
    beam_info.m_flHaloScale = 0.f;
    beam_info.m_flLife = 4.f;
    beam_info.m_flWidth = 2.f;
    beam_info.m_flEndWidth = 2.f;
    beam_info.m_flFadeLength = 0.1f;
    beam_info.m_flAmplitude = 2.f;
    beam_info.m_flBrightness = 255.f;
    beam_info.m_flSpeed = 0.2f;
    beam_info.m_nStartFrame = 0;
    beam_info.m_flFrameRate = 0.f;
    if (player == localPlayer.get()) {
        beam_info.m_flRed = config->visuals.bulletTracersLocal.color[0] * 255;
        beam_info.m_flGreen = config->visuals.bulletTracersLocal.color[1] * 255;
        beam_info.m_flBlue = config->visuals.bulletTracersLocal.color[2] * 255;
    }
    else if (localPlayer->isOtherEnemy(player)) {
        beam_info.m_flRed = config->visuals.bulletTracersEnemy.color[0] * 255;
        beam_info.m_flGreen = config->visuals.bulletTracersEnemy.color[1] * 255;
        beam_info.m_flBlue = config->visuals.bulletTracersEnemy.color[2] * 255;
    }
    else {
        beam_info.m_flRed = config->visuals.bulletTracersAllies.color[0] * 255;
        beam_info.m_flGreen = config->visuals.bulletTracersAllies.color[1] * 255;
        beam_info.m_flBlue = config->visuals.bulletTracersAllies.color[2] * 255;
    }
    beam_info.m_nSegments = 2;
    beam_info.m_bRenderable = true;
    beam_info.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;

    static Vector pos{ };
    if (auto record = Resolver::getClosest(memory->globalVars->serverTime()); record.time != 1.0f)
        pos = record.position;

    beam_info.m_vecStart = localPlayer->getEyePosition();
    beam_info.m_vecEnd = position;

    auto beam = memory->renderBeams->CreateBeamPoints(beam_info);
    if (beam)
        memory->renderBeams->DrawBeam(beam);
}



void Visuals::aaLines() noexcept
{
    if (!config->visuals.aaLines)
        return;

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

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    if (!memory->input->isCameraInThirdPerson)
        return;


    float lbyAngle = localPlayer->lby();
    float realAngle = localPlayer.get()->getAnimstate()->GoalFeetYaw;
    float fakeAngle = Animations::data.fakeAngle;
    float absAngle = localPlayer->getAbsAngle().y;

    Vector src3D, dst3D, forward, src, dst;

    //
    // LBY
    //

    AngleVectors(Vector{ 0, lbyAngle, 0 }, &forward);
    src3D = localPlayer->getAbsOrigin();
    dst3D = src3D + (forward * 40.f);

    if (!worldToScreen(src3D, src) || !worldToScreen(dst3D, dst))
        return;

    interfaces->surface->setDrawColor( 0, 255, 0 );
    //interfaces->surface->drawLine(src.x, src.y, dst.x, dst.y);

   // const auto [widthLby, heightLby] { interfaces->surface->getTextSize(hooks->smallFonts, L"LBY") };
    interfaces->surface->setTextColor(0, 255, 0);
   // interfaces->surface->setTextFont(hooks->smallFonts);
   // interfaces->surface->setTextPosition(dst.x - widthLby / 2, dst.y - heightLby / 2);
    //interfaces->surface->printText(L"LBY");

    //
    // Fake
    //

    AngleVectors(Vector{ 0, fakeAngle, 0 }, &forward);
    dst3D = src3D + (forward * 40.f);


    if (!worldToScreen(src3D, src) || !worldToScreen(dst3D, dst))
        return;

    interfaces->surface->setDrawColor(0, 0, 255);
    interfaces->surface->drawLine(src.x, src.y, dst.x, dst.y);

    const auto [widthFake, heightFake] { interfaces->surface->getTextSize(hooks->smallFonts, L"FAKE") };
    interfaces->surface->setTextColor(0, 0, 255);
    interfaces->surface->setTextFont(hooks->smallFonts);
    interfaces->surface->setTextPosition(dst.x - widthFake / 2, dst.y - heightFake / 2);
    interfaces->surface->printText(L"FAKE");

    //
    // Abs
    //

    AngleVectors(Vector{ 0, absAngle, 0 }, &forward);
    dst3D = src3D + (forward * 40.f);

    if (!worldToScreen(src3D, src) || !worldToScreen(dst3D, dst))
        return;

    interfaces->surface->setDrawColor(255, 0, 0);
    interfaces->surface->drawLine(src.x, src.y, dst.x, dst.y);

    const auto [widthAbs, heightAbs] { interfaces->surface->getTextSize(hooks->smallFonts, L"REAL") };
    interfaces->surface->setTextColor(255, 0, 0);
    interfaces->surface->setTextFont(hooks->smallFonts);
    interfaces->surface->setTextPosition(dst.x - widthAbs / 2, dst.y - heightAbs / 2);
    interfaces->surface->printText(L"REAL");
}

/////////////////////////////////////
void PenetrationCrosshair() noexcept
{
    if (!config->visuals.PenetrationCrosshair)
        return;

}

void Visuals::rainbowHud() noexcept
{
    const auto cvar = interfaces->cvar->findVar("cl_hud_color");

    constexpr float interval = 0.5f;

    static bool enabled = false;
    static int backup = 0;
    static float lastTime = 0.f;
    float curtime = memory->globalVars->currenttime;

    if (config->visuals.rainbowHud) {
        static int currentColor = 0;

        if (curtime - lastTime > interval) {
            cvar->setValue(currentColor);

            currentColor++;

            if (currentColor > 10)
                currentColor = 0;

            lastTime = curtime;
        }

        enabled = true;
    }
    else {
        if (enabled) {
            cvar->setValue(backup);
        }
        backup = cvar->getInt();
        enabled = false;
    }
}



void Visuals::velocityGraph() noexcept
{
    if (!config->visuals.velocityGraph)
        return;

    if (!localPlayer)
        return;
    if (!interfaces->engine->isInGame() || !localPlayer.get()->isAlive())
        return;

    static std::vector<float> velData(120, 0);

    Vector vecVelocity = localPlayer.get()->velocity();
    float currentVelocity = sqrt(vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y);

    velData.erase(velData.begin());
    velData.push_back(currentVelocity);

    float vel = localPlayer->velocity().length2D();
    std::wstring velwstr{std::to_wstring(static_cast<int>(vel))};

    const auto [width, height] = interfaces->surface->getScreenSize();
    interfaces->surface->setTextColor(255, 255, 255, 255);
    interfaces->surface->setTextFont(5);
    interfaces->surface->setTextPosition(width / 2 - interfaces->surface->getTextSize(5, velwstr.c_str()).first / 2, height / 2 + 460);
    interfaces->surface->printText(velwstr);

    interfaces->surface->setDrawColor(255, 255, 255, 255);

    for (auto i = 0; i < velData.size() - 1; i++)
    {
        int cur = velData.at(i);
        int next = velData.at(i + 1);

        interfaces->surface->drawLine( width / 2 + (velData.size() * 5 / 2) - (i - 1) * 5.f, height / 2 - (std::clamp(cur, 0, 450) * .2f) + 450,
            width / 2 + (velData.size() * 5 / 2) - i * 5.f,
            height / 2 - (std::clamp(next, 0, 450) * .2f) + 450
        );
    }
}

