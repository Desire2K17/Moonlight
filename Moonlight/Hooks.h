#pragma once

#include <d3d9.h>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <Windows.h>

#include "Hooks/MinHook.h"
#include "Hooks/VmtSwap.h"

struct SoundInfo;

using HookType = MinHook;

class Hooks {
public:
    Hooks(HMODULE module) noexcept;

    void install() noexcept;
    void uninstall() noexcept;

    WNDPROC originalWndProc;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> originalPresent;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> originalReset;
    std::add_pointer_t<int __fastcall(SoundInfo&)> originalDispatchSound;

    std::uintptr_t oStandardBlendingRules;

    unsigned long tahomaBoldAA;
    unsigned long smallFonts;
    unsigned long tahomaAA;
    unsigned long tahomaNoShadowAA;
    unsigned long verdanaExtraBoldAA;

    HookType FILESYSTEM_DLL;
    HookType bspQuery;
    HookType client;
    HookType clientMode;
    HookType engine;
    HookType gameEventManager;
    HookType modelRender;
    HookType panel;
    HookType prediction;
    HookType sound;
    HookType surface;
    HookType viewRender;
    HookType fileSystem;
    VmtSwap networkChannel;


    bool fixshitnightmode = false;

    HookType svCheats;
    HMODULE module;
private:
    HWND window;
};

inline std::unique_ptr<Hooks> hooks;
