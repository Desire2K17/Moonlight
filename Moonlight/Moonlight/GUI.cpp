#include <fstream>
#include <functional>
#include <string>
#include <ShlObj.h>
#include <windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_stdlib.h"

#include "imguiCustom.h"

#include "../Moonlight/Helpers.h"
#include "GUI.h"
#include "Config.h"
#include "Hacks/Misc.h"
#include "Hacks/SkinChanger.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "SDK/InputSystem.h"
#include "imgui/imgui_internal.h"
#include "fonts.hpp"
#include "bass.h"
#include "../Moonlight/Hacks/Visuals.h"
#include "SDK/WeaponId.h"
#include "SDK/LocalPlayer.h"
#include "SDK/Entity.h"
#include "Hacks/Esp.h"

#include "Texture.h"
#include "stb_image.h"


namespace fuckingkillme
{
    ImTextureID getItemIconTexture(const std::string& iconpath) noexcept;
}

static std::unordered_map<std::string, Texture> iconTextures;

ImTextureID fuckingkillme::getItemIconTexture(const std::string& iconpath) noexcept
{
    if (iconpath.empty())
        return 0;

    if (iconTextures[iconpath].get())
        return iconTextures[iconpath].get();

    if (iconTextures.size() >= 50)
        iconTextures.erase(iconTextures.begin());

    if (const auto handle = interfaces->baseFileSystem->open(("resource/flash/" + iconpath + "_large.png").c_str(), "r", "GAME")) {
        if (const auto size = interfaces->baseFileSystem->size(handle); size > 0) {
            const auto buffer = std::make_unique<std::uint8_t[]>(size);
            if (interfaces->baseFileSystem->read(buffer.get(), size, handle) > 0) {
                int width, height;
                stbi_set_flip_vertically_on_load_thread(false);

                if (const auto data = stbi_load_from_memory((const stbi_uc*)buffer.get(), size, &width, &height, nullptr, STBI_rgb_alpha)) {
                    iconTextures[iconpath].init(width, height, data);
                    stbi_image_free(data);
                }
                else {
                    assert(false);
                }
            }
        }
        interfaces->baseFileSystem->close(handle);
    }
    else {
        assert(false);
    }

    return iconTextures[iconpath].get();
}


int tabs = 0;
namespace guif {
    ImFont* font_main;
    ImFont* font_smooth;
    ImFont* font_main_big;
    ImFont* font_title;
    ImFont* arrowsa;
    ImFont* tab_iconsa;
    ImFont* weapon_fontsa;
    ImFont* weapon_fontsa2;
    ImFont* show_moon;
}

void hotkey(int& key)
{

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);

    key ? ImGui::Text("[ %s ]", interfaces->inputSystem->virtualKeyToString(key)) : ImGui::TextUnformatted("[ key ]");

    if (!ImGui::IsItemHovered())
        return;

    ImGui::SetTooltip("Press any key to set as the bind");
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++)
        if (ImGui::IsKeyPressed(i) && i != config->misc.menuKey)
            key = i != VK_ESCAPE ? i : 0;

    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
        if (ImGui::IsMouseDown(i) && i + (i > 1 ? 2 : 1) != config->misc.menuKey)
            key = i + (i > 1 ? 2 : 1);
}

GUI::GUI() noexcept
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    ImFontConfig cfg;

    guif::font_main = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)okfont, sizeof(okfont), 17.0f, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
    guif::font_smooth = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)okfont, sizeof(okfont), 17.0f, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
    guif::font_main_big = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(SmallestPixel_compressed_data, SmallestPixel_compressed_size, 14.0f);

    guif::font_title = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)titlefont, sizeof(titlefont), 22.0f, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    guif::arrowsa = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)arrows, sizeof(arrows), 15, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    guif::tab_iconsa = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)tab_icons, sizeof(tab_icons), 15, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    guif::weapon_fontsa = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)weapon_font, sizeof(weapon_font), 35, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    guif::weapon_fontsa2 = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)weapon_font, sizeof(weapon_font), 50, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    guif::show_moon = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)moon, sizeof(moon), 17.0f, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

}

void GUI::render() noexcept
{
    renderNewGui();
}

void GUI::renderNewGui() noexcept
{
    ImGuiStyle& style = ImGui::GetStyle();

    //RECT screen_rect;
    //GetWindowRect(GetDesktopWindow(), &screen_rect);

    if (open)
    {
        menu_animation = menu_animation + 6;
        border_animation = border_animation + 6;
        menu_slide = menu_slide + 10;
    }
    else if (!open)
    {
        menu_animation = menu_animation - 6;
        border_animation = border_animation - 6;
        menu_slide = menu_slide - 10;
    }

    if (menu_animation > 254)
        menu_animation = 255;
    if (menu_animation < 1)
        menu_animation = 0;

    if (border_animation > 3.4)
        border_animation = 3.5;
    if (border_animation < 1)
        border_animation = 0;

    if (menu_slide > 560)
        menu_slide = 561;
    if (menu_slide < 1)
        menu_slide = 0;

    if (small_tab)
    {
        do_tab_anim = do_tab_anim - 6;
        if (do_tab_anim < 31)
            do_tab_anim = 30;

        child_change_pos_on_tab_change = child_change_pos_on_tab_change - 6;
        if (child_change_pos_on_tab_change < 42)
            child_change_pos_on_tab_change = 41;
    }

    if (!small_tab)
    {
        do_tab_anim = do_tab_anim + 6;
        if (do_tab_anim > 99)
            do_tab_anim = 100;

        child_change_pos_on_tab_change = child_change_pos_on_tab_change + 6;
        if (child_change_pos_on_tab_change > 110)
            child_change_pos_on_tab_change = 111;
    }

    style.Colors[ImGuiCol_WindowBg] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_ChildBg] = ImColor(35, 35, 35, menu_animation);
    style.Colors[ImGuiCol_Border] = ImColor(25, 25, 25, menu_animation);
    style.Colors[ImGuiCol_BorderShadow] = ImColor(50, 50, 50, menu_animation);
    style.Colors[ImGuiCol_Button] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_ButtonActive] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_ButtonHovered] = ImColor(25, 25, 25, menu_animation);
    style.Colors[ImGuiCol_ScrollbarBg] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_CheckMark] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_FrameBg] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_SliderGrab] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_SliderGrabActive] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_PlotLines] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_PlotHistogram] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_TabActive] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_Header] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_HeaderActive] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_HeaderHovered] = ImColor(95, 75, 113, menu_animation);
    style.Colors[ImGuiCol_TitleBg] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_FrameBgActive] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_TitleBgActive] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_PopupBg] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_TextSelectedBg] = ImColor(130, 60, 180, menu_animation);
    style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255, menu_animation);
    style.Colors[ImGuiCol_TextDisabled] = ImColor(255, 255, 255, menu_animation);
    style.Colors[ImGuiCol_NavHighlight] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImColor(40, 40, 40, menu_animation);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImColor(40, 40, 40, menu_animation);

    style.WindowRounding = 0;
    style.ChildRounding = 0;
    style.WindowBorderSize = 0;
    style.ChildBorderSize = 0;
    style.WindowPadding = ImVec2(0, 0);
    style.DisplayWindowPadding = ImVec2(0, 0);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.DisplaySafeAreaPadding = ImVec2(0, 0);

    style.ScrollbarSize = 4;
    style.ItemSpacing = ImVec2(10, 5);
    style.FramePadding = ImVec2(0, 0);

    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;

    static int sub_tab_aim = 0;
    static int sub_tab_vis = 0;
    static int sub_tab_mis = 0;

    ImGui::SetNextWindowSize(ImVec2(menu_slide, 473));
    ImGui::Begin("MoonlightMenuByDesire", nullptr, ImGuiWindowFlags_NoDecoration);
    {
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::BeginChild("TopBar", ImVec2(561, 30));
        {
            ImGui::PushFont(guif::font_smooth);
            ImGui::BeginGroup();
            {
                if (tabs == 0)
                {
                    if (sub_tab_aim == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim, 5));
                        if (ImGui::Button("Legit", ImVec2(60, 20)))
                            sub_tab_aim = 0;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim, 5));
                        if (ImGui::Button("Legit", ImVec2(60, 20)))
                            sub_tab_aim = 0;
                        ImGui::PopStyleColor();
                    }

                    if (sub_tab_aim == 1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 60, 5));
                        if (ImGui::Button("Trigger", ImVec2(60, 20)))
                            sub_tab_aim = 1;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 60, 5));
                        if (ImGui::Button("Trigger", ImVec2(60, 20)))
                            sub_tab_aim = 1;
                        ImGui::PopStyleColor();
                    }

                    if (sub_tab_aim == 2)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 120, 5));
                        if (ImGui::Button("Rage", ImVec2(60, 20)))
                            sub_tab_aim = 2;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 120, 5));
                        if (ImGui::Button("Rage", ImVec2(60, 20)))
                            sub_tab_aim = 2;
                        ImGui::PopStyleColor();
                    }
                }
                if (tabs == 1)
                {
                    if (sub_tab_vis == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim, 5));
                        if (ImGui::Button("ESP", ImVec2(60, 20)))
                            sub_tab_vis = 0;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim, 5));
                        if (ImGui::Button("ESP", ImVec2(60, 20)))
                            sub_tab_vis = 0;
                        ImGui::PopStyleColor();
                    }

                    if (sub_tab_vis == 1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 60, 5));
                        if (ImGui::Button("World", ImVec2(60, 20)))
                            sub_tab_vis = 1;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 60, 5));
                        if (ImGui::Button("World", ImVec2(60, 20)))
                            sub_tab_vis = 1;
                        ImGui::PopStyleColor();
                    }         
                }

                if (tabs == 4)
                {
                    if (sub_tab_mis == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim, 5));
                        if (ImGui::Button("Main", ImVec2(60, 20)))
                            sub_tab_mis = 0;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim, 5));
                        if (ImGui::Button("Main", ImVec2(60, 20)))
                            sub_tab_mis = 0;
                        ImGui::PopStyleColor();
                    }

                    if (sub_tab_mis == 1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 60, 5));
                        if (ImGui::Button("Movement", ImVec2(60, 20)))
                            sub_tab_mis = 1;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(do_tab_anim + 60, 5));
                        if (ImGui::Button("Movement", ImVec2(60, 20)))
                            sub_tab_mis = 1;
                        ImGui::PopStyleColor();
                    }
                }

                ImGui::PushFont(guif::show_moon);
                ImGui::SetCursorPos(ImVec2(480, 5));
                ImGui::TextColored(ImColor(155, 155, 155, menu_animation), "A");
                ImGui::PopFont();

                ImGui::SetCursorPos(ImVec2(500, 5));
                ImGui::TextColored(ImColor(155, 155, 155, menu_animation), "moonlight");
            }
            ImGui::EndGroup();
            ImGui::PopFont();
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::BeginChild("TabsBg", ImVec2(do_tab_anim, 473));
        {
            ImGui::PushFont(guif::font_smooth);
            ImGui::BeginGroup();
            {
                ImGui::PushFont(guif::arrowsa);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                if (!small_tab)
                {
                    ImGui::SetCursorPos(ImVec2(7, 6));
                    if (ImGui::Button("B", ImVec2(0, 18)))
                        small_tab = true;

                    ImGui::PushFont(guif::font_smooth);

                    ImGui::SetCursorPos(ImVec2(7, 455));
                    ImGui::TextColored(ImColor(155, 155, 155, menu_animation), "Version 6.4");

                    if (tabs == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 30));
                        if (ImGui::Button("Aimbot", ImVec2(0, 30)))
                            tabs = 0;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 30));
                        if (ImGui::Button("Aimbot", ImVec2(0, 30)))
                            tabs = 0;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 60));
                        if (ImGui::Button("Visuals", ImVec2(0, 30)))
                            tabs = 1;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 60));
                        if (ImGui::Button("Visuals", ImVec2(0, 30)))
                            tabs = 1;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 2)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 90));
                        if (ImGui::Button("Chams", ImVec2(0, 30)))
                            tabs = 2;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 90));
                        if (ImGui::Button("Chams", ImVec2(0, 30)))
                            tabs = 2;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 3)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 120));
                        if (ImGui::Button("Skins", ImVec2(0, 30)))
                            tabs = 3;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 120));
                        if (ImGui::Button("Skins", ImVec2(0, 30)))
                            tabs = 3;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 4)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 150));
                        if (ImGui::Button("Misc", ImVec2(0, 30)))
                            tabs = 4;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 150));
                        if (ImGui::Button("Misc", ImVec2(0, 30)))
                            tabs = 4;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 5)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 180));
                        if (ImGui::Button("Configs", ImVec2(0, 30)))
                            tabs = 5;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 180));
                        if (ImGui::Button("Configs", ImVec2(0, 30)))
                            tabs = 5;
                        ImGui::PopStyleColor();
                    }


                    ImGui::PopFont();
                }
                else if (small_tab)
                {
                    ImGui::SetCursorPos(ImVec2(7, 6));
                    ImGui::PushID("ok");
                    if (ImGui::Button("A", ImVec2(0, 18)))
                        small_tab = false;
                    ImGui::PopID();

                    ImGui::PushFont(guif::tab_iconsa);

                    if (tabs == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 30));
                        if (ImGui::Button("A", ImVec2(0, 20)))
                            tabs = 0;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 30));
                        if (ImGui::Button("A", ImVec2(0, 20)))
                            tabs = 0;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 50));
                        if (ImGui::Button("B", ImVec2(0, 20)))
                            tabs = 1;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 50));
                        if (ImGui::Button("B", ImVec2(0, 20)))
                            tabs = 1;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 2)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 70));
                        if (ImGui::Button("C", ImVec2(0, 20)))
                            tabs = 2;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 70));
                        if (ImGui::Button("C", ImVec2(0, 20)))
                            tabs = 2;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 3)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 90));
                        if (ImGui::Button("D", ImVec2(0, 20)))
                            tabs = 3;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 90));
                        if (ImGui::Button("D", ImVec2(0, 20)))
                            tabs = 3;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 4)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 110));
                        if (ImGui::Button("E", ImVec2(0, 20)))
                            tabs = 4;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 110));
                        if (ImGui::Button("E", ImVec2(0, 20)))
                            tabs = 4;
                        ImGui::PopStyleColor();
                    }

                    if (tabs == 5)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 130));
                        if (ImGui::Button("F", ImVec2(0, 20)))
                            tabs = 5;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(7, 130));
                        if (ImGui::Button("F", ImVec2(0, 20)))
                            tabs = 5;
                        ImGui::PopStyleColor();
                    }

                    ImGui::PopFont();
                }
                ImGui::PopStyleColor();
                ImGui::PopFont();
            }
            ImGui::EndGroup();
            ImGui::PopFont();
        }
        ImGui::EndChild();

        if (tabs == 0)
        {
            static int currentCategory{ 0 };
            static int currentWeapon{ 0 };

            if (sub_tab_aim == 0 || sub_tab_aim == 1)
            {
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
                ImGui::BeginChild("SubTabsAimbot", ImVec2(284, 30));
                {
                    ImGui::PushFont(guif::font_smooth);
                    if (currentCategory == 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(0, 5));
                        if (ImGui::Button("ALL", ImVec2(60, -1)))
                            currentCategory = 0;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(0, 5));
                        if (ImGui::Button("ALL", ImVec2(60, -1)))
                            currentCategory = 0;
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopFont();

                    ImGui::PushFont(guif::weapon_fontsa);
                    if (currentCategory == 1)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(60, -1));
                        if (ImGui::Button("U", ImVec2(60, -1)))
                            currentCategory = 1;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(60, -1));
                        if (ImGui::Button("U", ImVec2(60, -1)))
                            currentCategory = 1;
                        ImGui::PopStyleColor();
                    }

                    ImGui::PushFont(guif::weapon_fontsa2);
                    if (currentCategory == 2)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(110, -11));
                        if (ImGui::Button("H", ImVec2(60, -1)))
                            currentCategory = 2;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(110, -11));
                        if (ImGui::Button("H", ImVec2(60, -1)))
                            currentCategory = 2;
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopFont();

                    if (currentCategory == 3)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(160, -1));
                        if (ImGui::Button("S", ImVec2(60, -1)))
                            currentCategory = 3;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(160, -1));
                        if (ImGui::Button("S", ImVec2(60, -1)))
                            currentCategory = 3;
                        ImGui::PopStyleColor();
                    }

                    ImGui::PushFont(guif::weapon_fontsa2);
                    if (currentCategory == 4)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(147, 109, 181, menu_animation));
                        ImGui::SetCursorPos(ImVec2(210, -9));
                        if (ImGui::Button("A", ImVec2(60, -1)))
                            currentCategory = 4;
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, menu_animation));
                        ImGui::SetCursorPos(ImVec2(210, -9));
                        if (ImGui::Button("A", ImVec2(60, -1)))
                            currentCategory = 4;
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopFont();
                    ImGui::PopFont();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 290, 40));
                ImGui::BeginChild("SubTabsAimbotWeapons", ImVec2(101, 30));
                {
                    ImGui::PushFont(guif::font_smooth);
                    ImGui::SetCursorPos(ImVec2(8, 7));
                    ImGui::BeginGroup();
                    {
                        switch (currentCategory) {
                        case 0:
                            currentWeapon = 0;
                            ImGui::NewLine();
                            break;
                        case 1: {
                            static int currentPistol{ 0 };
                            static constexpr const char* pistols[]{ " All", " Glock-18", " P2000", " USP-S", " Dual Berettas", " P250", " Tec-9", " Five-Seven", " CZ-75", " Desert Eagle", " Revolver" };

                            ImGui::PushItemWidth(85);
                            ImGui::Combo("", &currentPistol, [](void* data, int idx, const char** out_text) {
                                if (config->legitbot[idx ? idx : 35].enabled) {
                                    static std::string name;
                                    name = pistols[idx];
                                    *out_text = name.append(" *").c_str();
                                }
                                else {
                                    *out_text = pistols[idx];
                                }
                                return true;
                                }, nullptr, IM_ARRAYSIZE(pistols));

                            currentWeapon = currentPistol ? currentPistol : 35;
                            break;
                        }
                        case 2: {
                            static int currentHeavy{ 0 };
                            static constexpr const char* heavies[]{ " All", " Nova", " XM1014", " Sawed-off", " MAG-7", " M249", " Negev" };

                            ImGui::PushItemWidth(85);
                            ImGui::Combo("", &currentHeavy, [](void* data, int idx, const char** out_text) {
                                if (config->legitbot[idx ? idx + 10 : 36].enabled) {
                                    static std::string name;
                                    name = heavies[idx];
                                    *out_text = name.append(" *").c_str();
                                }
                                else {
                                    *out_text = heavies[idx];
                                }
                                return true;
                                }, nullptr, IM_ARRAYSIZE(heavies));

                            currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
                            break;
                        }
                        case 3: {
                            static int currentSmg{ 0 };
                            static constexpr const char* smgs[]{ " All", " Mac-10", " MP9", " MP7", " MP5-SD", " UMP-45", " P90", " PP-Bizon" };

                            ImGui::PushItemWidth(85);
                            ImGui::Combo("", &currentSmg, [](void* data, int idx, const char** out_text) {
                                if (config->legitbot[idx ? idx + 16 : 37].enabled) {
                                    static std::string name;
                                    name = smgs[idx];
                                    *out_text = name.append(" *").c_str();
                                }
                                else {
                                    *out_text = smgs[idx];
                                }
                                return true;
                                }, nullptr, IM_ARRAYSIZE(smgs));

                            currentWeapon = currentSmg ? currentSmg + 16 : 37;
                            break;
                        }
                        case 4: {
                            static int currentRifle{ 0 };
                            static constexpr const char* rifles[]{ " All", " Galil AR", " Famas", " AK-47", " M4A4", " M4A1-S", " SSG-08", " SG-553", " AUG", " AWP", " G3SG1", " SCAR-20" };

                            ImGui::PushItemWidth(85);
                            ImGui::Combo("", &currentRifle, [](void* data, int idx, const char** out_text) {
                                if (config->legitbot[idx ? idx + 23 : 38].enabled) {
                                    static std::string name;
                                    name = rifles[idx];
                                    *out_text = name.append(" *").c_str();
                                }
                                else {
                                    *out_text = rifles[idx];
                                }
                                return true;
                                }, nullptr, IM_ARRAYSIZE(rifles));

                            currentWeapon = currentRifle ? currentRifle + 23 : 38;
                            break;
                        }
                        }
                    }
                    ImGui::EndGroup();
                    ImGui::PopFont();
                }
                ImGui::EndChild();
            }

            if (sub_tab_aim == 0)
            {
                ImGui::PushFont(guif::font_smooth);
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 80));
                ImGui::BeginChild("General", ImVec2(200, 130));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("Enable Aimbot", &config->legitbot[currentWeapon].enabled);
                        static const char* hitboxes[]{ " Head"," Chest"," Stomach"," Arms"," Legs" };
                        static std::string previewvalue = "";
                        if (ImGui::BeginCombo("Hitboxes", previewvalue.c_str()))
                        {
                            previewvalue = "";
                            for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
                            {
                                ImGui::Selectable(hitboxes[i], &config->legitbot[currentWeapon].hitboxes[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                            }
                            ImGui::EndCombo();
                        }
                        bool once = false;
                        for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
                        {
                            if (!once)
                            {
                                previewvalue = "";
                                once = true;
                            }
                            if (config->legitbot[currentWeapon].hitboxes[i])
                            {
                                previewvalue += previewvalue.size() ? std::string(", ") + hitboxes[i] : hitboxes[i];
                            }
                        }
                        ImGui::Checkbox("Scoped Only", &config->legitbot[currentWeapon].scopedOnly);
                        ImGui::Checkbox("Ignore Flash", &config->legitbot[currentWeapon].ignoreFlash);
                        ImGui::Checkbox("Ignore Smoke", &config->legitbot[currentWeapon].ignoreSmoke);
                        ImGui::Checkbox("Aimlock", &config->legitbot[currentWeapon].aimlock);
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 80));
                ImGui::BeginChild("Accuracy", ImVec2(200, 120));
                {
                    ImGui::PushItemWidth(210);
                    ImGui::SetCursorPos(ImVec2(-15, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::SliderFloat("Fov", &config->legitbot[currentWeapon].fov, 0.0f, 180.0f, "%.2f", 2.5f);
                        ImGui::SliderFloat("Smooth", &config->legitbot[currentWeapon].smooth, 1.0f, 60.0f, "%.2f");
                        ImGui::SliderInt("Aimpoint Size", &config->legitbot[currentWeapon].multipoint, 0, 100, "%d");
                    }
                    ImGui::EndGroup();
                    ImGui::PopItemWidth();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 220));
                ImGui::BeginChild("Backtrack", ImVec2(200, 160));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("Enable Backtrack", &config->backtrack.enabled);
                        ImGui::Checkbox("Ignore Smoke", &config->backtrack.ignoreSmoke);
                        ImGui::Checkbox("Ignore Flash", &config->backtrack.ignoreFlash);

                        int iMaxBt = 200;

                        if (iMaxBt > 200 && !config->backtrack.fakeLatency)
                        {
                            iMaxBt = 200;

                            if (config->backtrack.timeLimit > 199)
                                config->backtrack.timeLimit = 200;
                        }
                        else if (config->backtrack.fakeLatency)
                            iMaxBt = 200 + config->backtrack.fakeLatencyAmount;

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderInt("Bactrack Time", &config->backtrack.timeLimit, 1, iMaxBt, "%d ms");
                        ImGui::PopItemWidth();

                        ImGui::Checkbox("Fake Ping", &config->backtrack.fakeLatency);

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderInt("Ping Amount", &config->backtrack.fakeLatencyAmount, 1, 200, "%d ms");
                        ImGui::PopItemWidth();

                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 210));
                ImGui::BeginChild("Other", ImVec2(200, 70));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("Visualize FOV", &config->visuals.fovCircle);
                        ImGui::Checkbox("Recoil Control", &config->legitbot[currentWeapon].standaloneRcs);
                        ImGui::Checkbox("Prioritize Backtrack", &config->legitbot[currentWeapon].backtrack);
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 290));
                ImGui::BeginChild("Key", ImVec2(200, 70));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("On Key", &config->legitbot[currentWeapon].onKey);
                        hotkey(config->legitbot[currentWeapon].key);
                        ImGui::Combo("Key Mode", &config->legitbot[currentWeapon].keyMode, " Hold\0 Toggle\0");
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();


                ImGui::PopFont();
            }

            ImGui::PushFont(guif::font_smooth);

            if (sub_tab_aim == 1)
            {
                static std::string previewvalue = "";
                bool once = false;
                static const char* hitboxes1[]{ " Head"," Chest"," Stomach"," Arms"," Legs" };
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 80));
                ImGui::BeginChild("General", ImVec2(200, 130));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("Enable Triggerbot", &config->triggerbot[currentWeapon].enabled);
                        hotkey(config->triggerbot[currentWeapon].key);
                        if (ImGui::BeginCombo("Hitboxes", previewvalue.c_str()))
                        {
                            previewvalue = "";
                            for (size_t i = 0; i < ARRAYSIZE(hitboxes1); i++)
                            {
                                ImGui::Selectable(hitboxes1[i], &config->triggerbot[currentWeapon].hitboxes[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                            }
                            ImGui::EndCombo();
                        }
                        for (size_t i = 0; i < ARRAYSIZE(hitboxes1); i++)
                        {
                            if (!once)
                            {
                                previewvalue = "";
                                once = true;
                            }
                            if (config->triggerbot[currentWeapon].hitboxes[i])
                            {
                                previewvalue += previewvalue.size() ? std::string(", ") + hitboxes1[i] : hitboxes1[i];
                            }
                        }
                        ImGui::Checkbox("Scoped Only", &config->triggerbot[currentWeapon].scopedOnly);
                        ImGui::Checkbox("Ignore Flash", &config->triggerbot[currentWeapon].ignoreFlash);
                        ImGui::Checkbox("Ignore Smoke", &config->triggerbot[currentWeapon].ignoreSmoke);
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 80));
                ImGui::BeginChild("Accuracy", ImVec2(200, 120));
                {
                    ImGui::PushItemWidth(210);
                    ImGui::SetCursorPos(ImVec2(-15, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::SliderInt("Hitchance", &config->triggerbot[currentWeapon].hitChance, 0, 100, "%d");
                        ImGui::SliderInt("Shot Delay", &config->triggerbot[currentWeapon].shotDelay, 0, 250, "%d ms");
                        ImGui::SliderInt("Min Dmg", &config->triggerbot[currentWeapon].minDamage, 0, 100, "%d");
                    }
                    ImGui::EndGroup();
                    ImGui::PopItemWidth();
                }
                ImGui::EndChild();
            }

            if (sub_tab_aim == 2)
            {
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
                ImGui::Text("Coming Soon...");
            }

            ImGui::PopFont();
        }

        ImGui::PushFont(guif::font_smooth);
        if (tabs == 1)
        {
            if (sub_tab_vis == 0)
            {
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
                ImGui::BeginChild("General", ImVec2(200, 170));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        if (open)
                        {
                            ImGui::Checkbox("Enable Players", &config->esp.players[ENEMIES_ALL].enabled);
                            ImGuiCustom::colorPicker("Render Box", config->esp.players[ENEMIES_ALL].box);
                            ImGuiCustom::colorPicker("Render Name", config->esp.players[ENEMIES_ALL].name);
                            ImGui::Checkbox("Render Health", &config->esp.players[ENEMIES_ALL].healthBar);
                            ImGuiCustom::colorPicker("Render Armor", config->esp.players[ENEMIES_ALL].armor);
                            ImGuiCustom::colorPicker("Render Money", config->esp.players[ENEMIES_ALL].money);
                            ImGuiCustom::colorPicker("Display Distance", config->esp.players[ENEMIES_ALL].distance);
                            ImGuiCustom::colorPicker("Display Weapon", config->esp.players[ENEMIES_ALL].activeWeapon);
                        }
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 40));
                ImGui::BeginChild("Weapons", ImVec2(200, 160));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        if (open)
                        {
                            ImGui::Checkbox("Enable Weapons", &config->esp.weapon.enabled);
                            ImGuiCustom::colorPicker("Render Box", config->esp.weapon.box);
                            ImGuiCustom::colorPicker("Render Name", config->esp.weapon.name);
                            ImGuiCustom::colorPicker("Render Ammo", config->esp.weapon.ammo);
                            ImGuiCustom::colorPicker("Display Distance", config->esp.weapon.distance);
                        }
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();
            }
            if (sub_tab_vis == 1)
            {
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
                ImGui::BeginChild("General", ImVec2(202, 370));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Button("Removals", { 70, 0 });
                        ImGui::SetNextWindowSize(ImVec2(245, 270), ImGuiCond_Always);
                        if (ImGui::BeginPopupContextItem(0, 0))
                        {
                            ImGui::Checkbox(" Disable PostProces", &config->visuals.disablePostProcessing);
                            ImGui::Checkbox(" Ragdoll Gravity", &config->visuals.inverseRagdollGravity);
                            ImGui::Checkbox(" Disable Fog", &config->visuals.noFog);
                            ImGui::Checkbox(" Disable 3d Sky", &config->visuals.no3dSky);
                            ImGui::Checkbox(" Disable Aim Punch", &config->visuals.noAimPunch);
                            ImGui::Checkbox(" Disable View Punch", &config->visuals.noViewPunch);
                            ImGui::Checkbox(" Disable Hands", &config->visuals.noHands);
                            ImGui::Checkbox(" Disable Sleeves", &config->visuals.noSleeves);
                            ImGui::Checkbox(" Disable Weapons", &config->visuals.noWeapons);
                            ImGui::Checkbox(" Disable Smoke", &config->visuals.noSmoke);
                            ImGui::Checkbox(" Disable Blur", &config->visuals.noBlur);
                            ImGui::Checkbox(" Disable Scope overlay", &config->visuals.noScopeOverlay);
                            ImGui::Checkbox(" Disable Grass", &config->visuals.noGrass);
                            ImGui::Checkbox(" Disable Shadows", &config->visuals.noShadows);
                            ImGui::Checkbox(" Wireframe Smoke", &config->visuals.wireframeSmoke);
                            ImGui::PushItemWidth(210);
                            ImGui::SetCursorPosX(-15);
                            ImGui::SliderInt(" Flash Reduction", &config->visuals.flashReduction, 0, 100, "%d%%");
                            ImGui::PopItemWidth();
                            ImGui::EndPopup();
                        }

                        ImGui::Checkbox("Bomb Timer", &config->misc.bombTimer.enabled);
                        ImGui::Checkbox("Footstep Beams", &config->visuals.footstepBeams);

                        if (open)
                            ImGuiCustom::colorPicker("Beam Color", config->visuals.BeamColor.color, nullptr, nullptr, nullptr);

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderInt("Beam Thickness", &config->visuals.footstepBeamThickness, 1, 5);
                        ImGui::PopItemWidth();

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderInt("Beam Radius", &config->visuals.footstepBeamRadius, 40, 400);
                        ImGui::PopItemWidth();

                        ImGui::Checkbox("Nightmode", &config->visuals.nightMode);
                        if (open)
                            ImGuiCustom::colorPicker("Nightmode Override", config->visuals.nightModeOverride);

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderInt("Asus Walls", &config->visuals.asusWalls, 0, 100, "%d%%");
                        ImGui::PopItemWidth();

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderInt("Asus Props", &config->visuals.asusProps, 0, 100, "%d%%");
                        ImGui::PopItemWidth();

                        ImGui::Combo("Skybox", &config->visuals.skybox, " Default\0 Custom\0 cs_baggage_skybox_\0 cs_tibet\0 embassy\0 italy\0 jungle\0 nukeblank\0 office\0 sky_cs15_daylight01_hdr\0 sky_cs15_daylight02_hdr\0 sky_cs15_daylight03_hdr\0 sky_cs15_daylight04_hdr\0 sky_csgo_cloudy01\0 sky_csgo_night_flat\0 sky_csgo_night02\0 sky_day02_05_hdr\0 sky_day02_05\0 sky_dust\0 sky_l4d_rural02_ldr\0 sky_venice\0 vertigo_hdr\0 vertigo\0 vertigoblue_hdr\0 vietnam\0");
                        if (config->visuals.skybox == 1)
                            ImGui::InputText("Custom Skybox", &config->visuals.customSkybox);   

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderFloat("Aspect Ratio", &config->misc.aspectratio, 0.0f, 5.0f, "%.2f");
                        ImGui::PopItemWidth();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 40));
                ImGui::BeginChild("XYZ", ImVec2(210, 390));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        if (open)
                        {
                            ImGui::Checkbox("Third Person", &config->visuals.thirdperson);
                            hotkey(config->visuals.thirdpersonKey);

                            ImGui::PushItemWidth(210);
                            ImGui::SetCursorPosX(-15);
                            ImGui::BeginGroup();
                            {
                                ImGui::SliderInt("Thirdperson Distance", &config->visuals.thirdpersonDistance, 0, 1000, "%d");
                                ImGui::SliderInt("Scope Transparency", &config->visuals.thirdpersonTransparency, 0, 100, "%d%%");
                                ImGui::SliderInt("Viewmodel FOV", &config->visuals.viewmodelFov, -60, 60, "%d");
                                ImGui::SliderInt("FOV", &config->visuals.fov, -60, 60, "%d");
                                ImGui::SliderInt("Scope FOV", &config->visuals.fovScoped, -60, 60, "%d");
                            }
                            ImGui::EndGroup();
                            ImGui::PopItemWidth();

                            ImGui::Checkbox("Viewmodel XYZ", &config->visuals.viewmodelxyz);
                            ImGui::PushItemWidth(210);
                            ImGui::SetCursorPosX(-15);
                            ImGui::BeginGroup();
                            {
                                ImGui::SliderFloat("X", &config->visuals.viewmodel_x, -20, 20, "%.2f");
                                ImGui::SliderFloat("Y", &config->visuals.viewmodel_y, -20, 20, "%.2f");
                                ImGui::SliderFloat("Z", &config->visuals.viewmodel_z, -20, 20, "%.2f");
                                ImGui::SliderInt("Roll", &config->visuals.viewmodelRoll, -90, 90, "%d");
                            }
                            ImGui::EndGroup();
                            ImGui::PopItemWidth();
                        }
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();
            }
        }
        ImGui::PopFont();

        ImGui::PushFont(guif::font_smooth);
        if (tabs == 2)
        {
            static int currentCategory{ 0 };
            static int material = 1;
            static int currentItem{ 0 };
            auto& chams{ config->chams[currentItem].materials[material - 1] };

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
            ImGui::BeginChild("Mod", ImVec2(135, 30));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {

                    if (ImGui::Combo("Model", &currentCategory, " Allies\0 Enemies\0 Local player\0 Weapons\0 Hands\0 Backtrack\0 Knife\0 Sleeves\0 Desync\0"))
                        material = 1;
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 141, 40));
            ImGui::BeginChild("Mat", ImVec2(176, 30));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    ImGui::Combo("Materials", &chams.material, " Normal\0 Flat\0 Animated\0 Platinum\0 Bobble\0 Silver\0 Silver Reflective\0 Metal\0 Gold\0 Plastic\0 Glow\0 Pearlescent\0");
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 80));
            ImGui::BeginChild("General", ImVec2(200, 160));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    ImGui::Checkbox("Enable Chams", &chams.enabled);
                    if (open)
                        ImGuiCustom::colorPopup("Color", chams.color, &chams.rainbow, &chams.rainbowSpeed);
                    ImGui::Checkbox("Health Based", &chams.healthBased);
                    ImGui::Checkbox("Flash", &chams.blinking);
                    ImGui::Checkbox("Wireframe", &chams.wireframe);

                    if (material <= 1)
                        ImGuiCustom::arrowButtonDisabled("##left", ImGuiDir_Left);
                    else if (ImGui::ArrowButton("##left", ImGuiDir_Left))
                        --material;

                    ImGui::SameLine();
                    ImGui::Text("%d", material);
                    ImGui::SameLine();

                    if (material >= int(config->chams[0].materials.size()))
                        ImGuiCustom::arrowButtonDisabled("##right", ImGuiDir_Right);
                    else if (ImGui::ArrowButton("##right", ImGuiDir_Right))
                        ++material;

                    if (currentCategory <= 1) {

                        static int currentType{ 0 };
                        ImGui::PushID(1);
                        if (ImGui::Combo("Mode", &currentType, " Visible\0 Occluded\0"))
                            material = 1;
                        ImGui::PopID();
                        currentItem = currentCategory * 2 + currentType;
                    }
                    else {
                        currentItem = currentCategory + 2;

                        if (currentCategory > 5) {
                            currentItem = currentItem + 1;
                        }
                        if (currentCategory == 5) {
                            static int currentType{ 0 };
                            ImGui::PushID(1);
                            if (ImGui::Combo("Mode", &currentType, " Visible\0 Occluded\0"))
                                material = 1;
                            ImGui::PopID();
                            currentItem = currentItem + currentType;
                        }
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();
        }
        ImGui::PopFont();

        ImGui::PushFont(guif::font_smooth);
        if (tabs == 3)
        {
            static auto itemIndex = 0;
            auto& selected_entry = config->skinChanger[itemIndex];
            selected_entry.itemIdIndex = itemIndex;
            static ImGuiTextFilter filter;
            static auto selectedStickerSlot = 0;
            auto& selected_sticker = selected_entry.stickers[selectedStickerSlot];

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
            ImGui::BeginChild("Skins", ImVec2(200, 200));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    filter.Draw();

                    const auto& kits = itemIndex == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits;
                    if (ImGui::ListBoxHeader("Paint Kit")) {
                        for (size_t i = 0; i < kits.size(); ++i) {
                            if (filter.PassFilter((itemIndex == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits)[i].name.c_str()))
                                if (ImGui::Selectable((itemIndex == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits)[i].name.c_str(), i == selected_entry.paint_kit_vector_index))
                                    selected_entry.paint_kit_vector_index = i;                           
                        }

                        ImGui::ListBoxFooter();
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 250));
            ImGui::BeginChild("Models", ImVec2(200, 50));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    ImGui::Combo("Weapon##1", &itemIndex, [](void* data, int idx, const char** out_text) {
                        *out_text = game_data::weapon_names[idx].name;
                        return true;
                        }, nullptr, IM_ARRAYSIZE(game_data::weapon_names), 5);


                    if (itemIndex == 0) {
                        ImGui::Combo("Knife", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                            *out_text = game_data::knife_names[idx].name;
                            return true;
                            }, nullptr, IM_ARRAYSIZE(game_data::knife_names), 5);
                    }
                    else if (itemIndex == 1) {
                        ImGui::Combo("Glove", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                            *out_text = game_data::glove_names[idx].name;
                            return true;
                            }, nullptr, IM_ARRAYSIZE(game_data::glove_names), 5);
                    }
                    else {
                        static auto unused_value = 0;
                        selected_entry.definition_override_vector_index = 0;
                        ImGui::Combo("Unavailable", &unused_value, "For knives or gloves\0");
                    }

                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 40));
            ImGui::BeginChild("stickers", ImVec2(200, 200));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    static ImGuiTextFilter filter2;
                    ImGui::PushID("ok");
                    filter2.Draw();
                    ImGui::PopID();

                    if (ImGui::ListBoxHeader("Stickers")) {
                        for (size_t i = 0; i < SkinChanger::stickerKits.size(); ++i) {
                            if (filter2.PassFilter((SkinChanger::stickerKits)[i].name.c_str()))
                                if (ImGui::Selectable((SkinChanger::stickerKits)[i].name.c_str(), i == selected_sticker.kit_vector_index))
                                    selected_sticker.kit_vector_index = i;
                        }

                        ImGui::ListBoxFooter();
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 210, 250));
            ImGui::BeginChild("stickersw", ImVec2(200, 110));
            {
                ImGui::PushItemWidth(210);
                ImGui::SetCursorPosX(-15);
                ImGui::BeginGroup();
                {                 
                    ImGui::SliderFloat("Wear", &selected_sticker.wear, FLT_MIN, 1.0f, "%.4f", 5.0f);
                    ImGui::SliderFloat("Scale", &selected_sticker.scale, 0.1f, 5.0f);
                    ImGui::SliderFloat("Rotation", &selected_sticker.rotation, 0.0f, 360.0f);
                }
                ImGui::EndGroup();
                ImGui::PopItemWidth();

                auto& selected_entry = config->skinChanger[itemIndex];
                selected_entry.itemIdIndex = itemIndex;
                selected_entry.update();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 310));
            ImGui::BeginChild("Apply", ImVec2(200, 40));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    if (ImGui::Button("Update", { 190.0f, 30.0f }))
                        SkinChanger::scheduleHudUpdate();
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();
        }
        ImGui::PopFont();

        ImGui::PushFont(guif::font_smooth);
        if (tabs == 4)
        {
            if (sub_tab_mis == 0)
            {
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
                ImGui::BeginChild("General", ImVec2(250, 275));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("Auto Pistol", &config->misc.autoPistol);
                        ImGui::Checkbox("Auto Accept", &config->misc.autoAccept);
                        ImGui::Checkbox("EngineCrosshair", &config->misc.sniperCrosshair);
                        ImGui::Checkbox("RecoilCrosshair", &config->misc.recoilCrosshair);
                        ImGui::Checkbox("Hitmarker", &config->visuals.hitMarker);
                        ImGui::Combo("Hit Sound", &config->misc.hitSound, " None\0 Coins\0 Metal\0 Arena Switch\0 Shield\0 Custom\0");
                        if (config->misc.hitSound == 5) {
                            ImGui::InputText("Hit Sound filename", &config->misc.customHitSound);
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("audio file must be put in csgo/sound/ folder");
                        }

                        ImGui::Combo("Kill Sound", &config->misc.killSound, " None\0 Coins\0 Metal\0 Arena Switch\0 Shield\0 Custom\0");
                        if (config->misc.killSound == 5) {
                            ImGui::InputText("Kill Sound filename", &config->misc.customKillSound);
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("audio file must be put in csgo/sound/ folder");
                        }

                        ImGui::Combo("Screen Effect", &config->visuals.screenEffect, " None\0 Drone cam\0 Drone cam with noise\0 Underwater\0 Healthboost\0 Dangerzone\0");
                        ImGui::Combo("Hit Effect", &config->visuals.hitEffect, " None\0 Drone cam\0 Drone cam with noise\0 Underwater\0 Healthboost\0 Dangerzone\0");

                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderFloat("Hit Effect Time", &config->visuals.hitEffectTime, 0.1f, 1.5f, "%.2fs");
                        ImGui::PopItemWidth();

                        ImGui::Checkbox("Extend Chams Distance", &config->misc.disableModelOcclusion);
                        ImGui::Checkbox("Disable Panorama Blur", &config->misc.disablePanoramablur);

                        ImGui::Checkbox("Speedhack", &config->visuals.velocityGraph);
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 260, 40));
                ImGui::BeginChild("ok", ImVec2(180, 110));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::Checkbox("Clantag", &config->misc.clanTag);
                        ImGui::Checkbox("Watermark", &config->misc.watermark);
                        ImGui::Checkbox("Spectator List", &config->misc.spectatorList);
                        ImGui::Checkbox("Preserve Killfeed", &config->misc.preserveDeathNotices);
                        ImGui::Checkbox("Killsay", &config->misc.killSay);
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();
            }
            if (sub_tab_mis == 1)
            {
                ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
                ImGui::BeginChild("General", ImVec2(287, 290));
                {
                    ImGui::SetCursorPos(ImVec2(5, 5));
                    ImGui::BeginGroup();
                    {
                        ImGui::PushItemWidth(210);
                        ImGui::SetCursorPosX(-15);
                        ImGui::SliderFloat("Bhop Hitchance", &config->misc.flBhopChance, 0, 100);
                        ImGui::PopItemWidth();
                        ImGui::Checkbox("Moonwalk", &config->misc.moonwalk);
                        ImGui::Checkbox("FastStop", &config->misc.fastStop);

                        ImGui::Checkbox("Slowwalk", &config->misc.slowwalk);
                        hotkey(config->misc.slowwalkKey);

                        ImGui::Checkbox("Block Bot", &config->misc.playerBlocker);
                        hotkey(config->misc.playerBlockerKey);

                        ImGui::Checkbox("JumpBug", &config->misc.jumpbug);
                        hotkey(config->misc.jumpbugkey);
                        ImGui::Combo("Jump Bug Mode", &config->misc.jumpbugstyle, " Exclusive\0 Hybrid\0 Simple\0");

                        ImGui::Checkbox("EdgeBug", &config->misc.edgebug);
                        hotkey(config->misc.edgebugkey);
                        ImGui::Combo("Edge Bug Mode", &config->misc.edgebugstyle, " Standard\0 Short\0 Exclusive\0 StandardNoSound\0");

                    }
                    ImGui::EndGroup();
                }
                ImGui::EndChild();      
            }
        }
        ImGui::PopFont();
        ImGui::PushFont(guif::font_smooth);
        if (tabs == 5)
        {
            auto& configItems = config->getConfigs();
            static int currentConfig = -1;
            if (static_cast<std::size_t>(currentConfig) >= configItems.size())
                currentConfig = -1;
            static std::string buffer;

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change, 40));
            ImGui::BeginChild("General", ImVec2(160, 142));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {
                    ImGui::PushItemWidth(150);

                    if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
                        auto& vector = *static_cast<std::vector<std::string>*>(data);
                        *out_text = vector[idx].c_str();
                        return true;
                        }, &configItems, configItems.size(), 5) && currentConfig != -1)
                        buffer = configItems[currentConfig];

                        if (ImGui::InputTextWithHint("", "config name", &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
                            if (currentConfig != -1)
                                config->rename(currentConfig, buffer.c_str());
                        }

                    ImGui::PopItemWidth();
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(child_change_pos_on_tab_change + 170, 40));
            ImGui::BeginChild("Buttons", ImVec2(160, 140));
            {
                ImGui::SetCursorPos(ImVec2(5, 5));
                ImGui::BeginGroup();
                {

                    if (ImGui::Button("Create"))
                        config->add(buffer.c_str());

                    if (currentConfig != -1) {
                        if (ImGui::Button("Load")) {
                            config->load(currentConfig);
                            SkinChanger::scheduleHudUpdate();
                        }

                        if (ImGui::Button("Save"))
                            config->save(currentConfig);


                        if (ImGui::Button("Delete")) {
                            config->remove(currentConfig);
                            currentConfig = -1;
                            buffer.clear();
                        }
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();
        }
        ImGui::PopFont();

        ImVec2 windowpos = ImGui::GetWindowPos();
        auto drawlist = ImGui::GetWindowDrawList();
    }
    ImGui::End();
}