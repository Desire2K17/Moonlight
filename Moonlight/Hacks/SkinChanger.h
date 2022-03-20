#pragma once

#include <array>
#include <string>
#include <vector>


enum class FrameStage;
class GameEvent;

#include "../imgui/imgui.h"
namespace SkinChanger
{
    void initializeKits() noexcept;
    void run(FrameStage) noexcept;
    void scheduleHudUpdate() noexcept;
    void overrideHudIcon(GameEvent& event) noexcept;
    void updateStatTrak(GameEvent& event) noexcept;

   // ImTextureID getItemIconTexture(const std::string& iconpath) noexcept;

    struct PaintKit {
        PaintKit(int id, std::string&& name) noexcept : id(id), name(name) { }

        int id;
        int rarity;
        std::string name;
        std::wstring nameUpperCase;
        std::string iconPath;

        auto operator<(const PaintKit& other) const noexcept
        {
            return nameUpperCase < other.nameUpperCase;
        }
    };

    inline std::vector<PaintKit> skinKits;
    inline std::vector<PaintKit> gloveKits;
    inline std::vector<PaintKit> stickerKits{ {0, "None"} };
}
