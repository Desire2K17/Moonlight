#include <clocale>
#ifdef _WIN32
#include <Windows.h>
#endif
#include "Hooks.h"
#include <String.h>
#include <intrin.h>
#include <sstream>
#include <string>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <string>
#include <fstream>
#include <tchar.h>
#pragma warning(disable:4996)

using namespace std;

void catchcrackuserbydesire()
{
    std::ifstream myReadFile;
    myReadFile.open("C:/Windows/notepad.bin");

    if (myReadFile.fail()) //checks if the file CANNOT be opened
    {
        MessageBox(0, "Poor crack user detected, laff", "Moonlight.uno", MB_OK); //cool message

        Sleep(4000); //wait 4 seconds

        exit(0); //exit
    }
}

#ifdef _WIN32

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);


//This is very important imo
void CreateConsole() {
    if (AllocConsole()) {//Allocate console
        freopen("conin$", "r+t", stdin);//so we can write to standard out, read standard in and what not.
        freopen("conout$", "w+t", stdout);
        freopen("conout$", "w+t", stderr);
        SetConsoleTitleA("Moonlight.uno Debug Console");
        printf("Console Initialized\n");
    }
}

void ErasePEHeaders(HANDLE hModule) {
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + (DWORD)pDosHeader->e_lfanew);

    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
        return;

    if (pNTHeader->FileHeader.SizeOfOptionalHeader)
    {
        DWORD Protect;
        WORD Size = pNTHeader->FileHeader.SizeOfOptionalHeader;
        VirtualProtect((void*)hModule, Size, PAGE_EXECUTE_READWRITE, &Protect);
        RtlZeroMemory((void*)hModule, Size);
        VirtualProtect((void*)hModule, Size, Protect, &Protect);
    }
}


void SizeOf() {
    __asm
    {
        mov eax, fs: [0x30]// PEB
        mov eax, [eax + 0x0c]// PEB_LDR_DATA
        mov eax, [eax + 0x0c]// InOrderModuleList
        mov dword ptr[eax + 20h], 100000h// SizeOfImage    
    }
}

BOOL APIENTRY DllEntryPoint(HMODULE moduleHandle, DWORD reason, LPVOID reserved)
{
    if (!_CRT_INIT(moduleHandle, reason, reserved))
        return FALSE;


    if (reason == DLL_PROCESS_ATTACH) {

        while (!GetModuleHandleA("serverbrowser.dll"))
            Sleep(200);



        while (!GetModuleHandleA("steamnetworkingsockets.dll"))
            Sleep(200);
#ifdef _DEBUG
        CreateConsole();

#else
        catchcrackuserbydesire();
#endif

       std::setlocale(LC_CTYPE, ".utf8");
       
       printf("[+] Initializing Hooks\n");
       hooks = std::make_unique<Hooks>(moduleHandle);
    }
    return TRUE;
}

#else

void __attribute__((constructor)) DllEntryPoint()
{
    hooks = std::make_unique<Hooks>();
}

#endif    