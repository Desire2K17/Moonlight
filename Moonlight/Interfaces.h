#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>
#include "../Moonlight/SDK/VirtualMethod.h"

class BaseFileSystem {
public:
    VIRTUAL_METHOD(int, read, 0, (void* output, int size, void* handle), (this, output, size, handle))
        VIRTUAL_METHOD(void*, open, 2, (const char* fileName, const char* options, const char* pathID), (this, fileName, options, pathID))
        VIRTUAL_METHOD(void, close, 3, (void* handle), (this, handle))
        VIRTUAL_METHOD(unsigned int, size, 7, (void* handle), (this, handle))
};

class MemAlloc;

class Interfaces {
public:
#define GAME_INTERFACE(type, name, module, version) \
class type* name = reinterpret_cast<type*>(find(L##module, version));


    GAME_INTERFACE(BaseFileSystem, baseFileSystem, "filesystem_stdio", "VBaseFileSystem011")
    GAME_INTERFACE(Client, client, "client", "VClient018")
    GAME_INTERFACE(Cvar, cvar, "vstdlib", "VEngineCvar007")
    GAME_INTERFACE(Engine, engine, "engine", "VEngineClient014")
    GAME_INTERFACE(EngineTrace, engineTrace, "engine", "EngineTraceClient004")
    GAME_INTERFACE(EntityList, entityList, "client", "VClientEntityList003")
    GAME_INTERFACE(GameEventManager, gameEventManager, "engine", "GAMEEVENTSMANAGER002")
    GAME_INTERFACE(GameMovement, gameMovement, "client", "GameMovement001")
    GAME_INTERFACE(GameUI, gameUI, "client", "GameUI011")
    GAME_INTERFACE(InputSystem, inputSystem, "inputsystem", "InputSystemVersion001")
    GAME_INTERFACE(Localize, localize, "localize", "Localize_001")
    GAME_INTERFACE(MaterialSystem, materialSystem, "materialsystem", "VMaterialSystem080")
    MemAlloc* memalloc = *(MemAlloc**)(GetProcAddress(GetModuleHandleA("tier0.dll"), "g_pMemAlloc"));
    GAME_INTERFACE(ModelInfo, modelInfo, "engine", "VModelInfoClient004")
    GAME_INTERFACE(ModelRender, modelRender, "engine", "VEngineModel016")
    GAME_INTERFACE(NetworkStringTableContainer, networkStringTableContainer, "engine", "VEngineClientStringTable001")
    GAME_INTERFACE(Panel, panel, "vgui2", "VGUI_Panel009")
    GAME_INTERFACE(PhysicsSurfaceProps, physicsSurfaceProps, "vphysics", "VPhysicsSurfaceProps001")
    GAME_INTERFACE(Prediction, prediction, "client", "VClientPrediction001")
    GAME_INTERFACE(RenderView, renderView, "engine", "VEngineRenderView014")
    GAME_INTERFACE(Surface, surface, "vguimatsurface", "VGUI_Surface031")
    GAME_INTERFACE(Sound, sound, "engine", "IEngineSoundClient003")
    GAME_INTERFACE(SoundEmitter, soundEmitter, "soundemittersystem", "VSoundEmitter003")
    GAME_INTERFACE(StudioRender, studioRender, "studiorender", "VStudioRender026")

#undef GAME_INTERFACE

private:
    static void* find(const wchar_t* moduleName, const char* name) noexcept
    {
        if (const auto createInterface = reinterpret_cast<std::add_pointer_t<void* __cdecl (const char* name, int* returnCode)>>(GetProcAddress(GetModuleHandleW(moduleName), "CreateInterface")))
            if (void* foundInterface = createInterface(name, nullptr))
                return foundInterface;

        MessageBoxA(nullptr, ("Failed to find " + std::string{ name } + "!").c_str(), "Moon", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
    }
};

inline std::unique_ptr<const Interfaces> interfaces;
