#pragma once

#include <memory>
#include <string>

struct ImFont;

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;

    bool open = true;
    inline static int menu_animation = 0;
    inline static int border_animation = 0;
    inline static int menu_slide = 0;
    inline static bool small_tab = false;
    inline static int do_tab_anim = 0;
    inline static int child_change_pos_on_tab_change = 0;
    inline static int child_change_width_on_tab_change = 0;
private:
    void renderNewGui() noexcept;

    struct {
        bool legitbot = false;
        bool ragebot = false;
        bool antiAim = false;
        bool triggerbot = false;
        bool backtrack = false;
        bool glow = false;
        bool chams = false;
        bool esp = false;
        bool visuals = false;
        bool skinChanger = false;
        bool sound = false;
        bool style = false;
        bool misc = false;
        bool config = false;
    } window;

    struct {
        ImFont* fonto = nullptr;
    } fonts;
};

inline std::unique_ptr<GUI> gui;

namespace guif {

    extern ImFont* font_main;
}
