#pragma once
#include <array>
enum class FrameStage;
class GameEvent;
struct ImDrawList;

namespace Visuals
{
    void playerModel(FrameStage stage) noexcept;
    void nightMode() noexcept;
    void modifySmoke(FrameStage stage) noexcept;
    void thirdperson() noexcept;
    void removeVisualRecoil(FrameStage stage) noexcept;
    void removeBlur(FrameStage stage) noexcept;
    void updateBrightness() noexcept;
    void removeGrass(FrameStage stage) noexcept;
    void remove3dSky() noexcept;
    void removeShadows() noexcept;
    void applyScreenEffects() noexcept;
    void hitEffect(GameEvent* event = nullptr) noexcept;
    void hitMarker(GameEvent* event = nullptr) noexcept;
    void fovCircle() noexcept;
    void disablePostProcessing(FrameStage stage) noexcept;
    void reduceFlashEffect() noexcept;
    bool removeHands(const char* modelName) noexcept;
    bool removeSleeves(const char* modelName) noexcept;
    bool removeWeapons(const char* modelName) noexcept;
    void skybox(FrameStage stage) noexcept;
    void viewmodelxyz() noexcept;
    void bulletBeams(GameEvent* event) noexcept;
    void aaLines() noexcept;
    void PenetrationCrosshair() noexcept;
    void rainbowHud() noexcept;
    void velocityGraph() noexcept;
    void drawMolotovHull(ImDrawList* drawList) noexcept;
    constexpr std::array skyboxes{ "Disabled", "Custom", "cs_baggage_skybox_", "cs_tibet", "embassy", "italy", "jungle", "nukeblank", "office", "sky_cs15_daylight01_hdr", "sky_cs15_daylight02_hdr", "sky_cs15_daylight03_hdr", "sky_cs15_daylight04_hdr", "sky_csgo_cloudy01", "sky_csgo_night_flat", "sky_csgo_night02", "sky_day02_05_hdr", "sky_day02_05", "sky_dust", "sky_l4d_rural02_ldr", "sky_venice", "vertigo_hdr", "vertigo", "vertigoblue_hdr", "vietnam" };

}
