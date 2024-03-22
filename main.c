#include <stdio.h>
#include "windows.h"
#include <tlhelp32.h>
#include <Psapi.h>
#include <limits.h>

#define UNICODE
#define NMODULES 1000

HANDLE prochandle;
HMODULE supertuxmodule;
MODULEINFO stmoduleinfo;

#include "window_helper.c"

HANDLE getsupertuxprocess(void){
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 100);
    if(hSnapshot == INVALID_HANDLE_VALUE){
        printf("An error occurred while enumerating the processes...\n"
               "Exiting...");
        exit(1);
    }
    PROCESSENTRY32W proc;
    proc.dwSize = sizeof(PROCESSENTRY32W);

    while (Process32NextW(hSnapshot, &proc))
    {   
        if(!wcscmp(proc.szExeFile, L"supertux2.exe")){
            printf("SuperTux process found.\n");
            printf("Opening process...\n");
            HANDLE prochandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc.th32ProcessID);
            if(prochandle == NULL){
                printf("Could not obtain handle to SuperTux process...\n"
                       "Exiting...\n");
                exit(1);
            }
            printf("Process handle was successfully obtained!\n");
            return prochandle;
        }

    }

    printf("SuperTux process not found...\n"
           "Exiting...");
    exit(1);
}

HMODULE getsupertux2module(void){
    printf("Obtaining process modules...\n");
    HMODULE procmodules[NMODULES];
    DWORD lpcbneeded;
    if(!K32EnumProcessModules(prochandle, procmodules, sizeof(HMODULE) * NMODULES, &lpcbneeded)){
        printf("An error ocurred when enumerating SuperTux modules...\n"
               "Exiting...\n");
        exit(1);
    }
    DWORD nmodules = lpcbneeded / sizeof(HMODULE);
    printf("%d modules found\n", nmodules);
    #define NSIZE 50
    WCHAR lpBaseName[NSIZE];
    for(int i = 0; i < nmodules; i++){
        K32GetModuleBaseNameW(prochandle, procmodules[i], lpBaseName, NSIZE);
        if(!wcscmp(lpBaseName, L"supertux2.exe")){
            printf("supertux2.exe module found!\n");
            if(!GetModuleInformation(prochandle, procmodules[i], &stmoduleinfo, sizeof(MODULEINFO))){
                printf("Couldn't obtain info from supertux2.exe module...\n"
                       "Exiting...\n");
                exit(1);
            }
            return procmodules[i];
        }
    }
}

LPVOID traversepointerchain(int * offsets, int noffsets){
    LPBYTE addr = stmoduleinfo.lpBaseOfDll;
    SIZE_T lpNumberOfBytesRead;
    for(int i = 0; i < noffsets-1; i++){
        ReadProcessMemory(prochandle, (LPCVOID) (addr + offsets[i]), (LPVOID) &addr, 8, &lpNumberOfBytesRead);
    }
    return addr + offsets[noffsets-1];
}

void getFireballs(HANDLE prochandle, LPBYTE lpBaseofDll){
    int offsets[4] = {0x002F0930, 0x140, 0x20, 0x8};
    LPVOID fireballaddr = traversepointerchain(offsets, 4);
    unsigned short fireballs;
    SIZE_T lpNumberOfBytesRead;
    while (TRUE)
    {   
        ReadProcessMemory(prochandle, fireballaddr, &fireballs, sizeof(unsigned short), &lpNumberOfBytesRead);
        printf("The number of fireballs is: %d\n", fireballs);
    }
}

void teleport(){
    int offsets[5] = {0x002F0AC0, 0x190, 0x8, 0x0, 0x8};
    float xpos;
    SIZE_T lpNumberOfBytesRead;
    SIZE_T lpNumberOfBytesWritten;
    while (TRUE)
    {   
        if(GetAsyncKeyState(VK_CONTROL))
        {
            LPVOID xposaddr = traversepointerchain(offsets, 5);
            ReadProcessMemory(prochandle, xposaddr, &xpos, sizeof(float), &lpNumberOfBytesRead);
            xpos = xpos+200;
            WriteProcessMemory(prochandle, xposaddr, &xpos, sizeof(float), &lpNumberOfBytesWritten);
            Sleep(100);
        }
        Sleep(50);
    }
}

void flyhack(){
    int offsets[5] = {0x002F0AC0, 0x190, 0x8, 0x0, 0x110};
    unsigned char onGround = 1;
    SIZE_T lpNumberOfBytesWritten;
    while(TRUE){
        LPVOID onGroundaddr = traversepointerchain(offsets, 5);
        WriteProcessMemory(prochandle, onGroundaddr, &onGround, sizeof(unsigned char), &lpNumberOfBytesWritten);
        Sleep(50);
    }
}

BOOL checkplaying(){
    int leveltime_offs[5] = {0x002F06B8, 0x78, 0x70, 0x10, 0x198};
    int onmenu_offs[5] = {0x002F06B8, 0x78, 0x70, 0x10, 0xA8};
    LPVOID leveltimeaddr = traversepointerchain(leveltime_offs, 5);
    LPVOID onmenuaddr = traversepointerchain(onmenu_offs, 5);
    float leveltime = 0;
    BYTE onmenu = 1;
    SIZE_T lpNumberOfBytesRead;

    ReadProcessMemory(prochandle, leveltimeaddr, &leveltime, sizeof(float), &lpNumberOfBytesRead);

    ReadProcessMemory(prochandle, onmenuaddr, &onmenu, sizeof(BYTE), &lpNumberOfBytesRead);

    BYTE *onLevelptr = (BYTE *) stmoduleinfo.lpBaseOfDll + 0x002F0AC0;
    DWORD64 onLevel = 1;
    ReadProcessMemory(prochandle, onLevelptr,  &onLevel, sizeof(DWORD64), &lpNumberOfBytesRead);

    if(!onLevel || leveltime <= 0 || onmenu)
        return FALSE;
    
    return TRUE;
}

void freedomtux(){
    HWND stwnd = find_main_window(GetProcessId(prochandle));
    int onGround_offs[5] = {0x002F0AC0, 0x190, 0x8, 0x0, 0x110};
    int direction_offs[5] = {0x002F0AC0, 0x190, 0x8, 0x0, 0x100};
    int tuxypos_offs[5] = {0x002F0AC0, 0x190, 0x8, 0x0, 0xC};
    int tuxform_offs[4] = {0x002F0B70, 0x28, 0x20, 0x4};
    int tuxheight_offs[5] = {0x002F0AC0, 0x190, 0x8, 0x0, 0x14};
    int fireballs_offs[4] = {0x002F0930, 0x140, 0x20, 0x8};
    unsigned char onGround = 1;
    unsigned char direction = 1;
    unsigned int tuxform = 2;
    unsigned int fireballs = 99999;
    float tuxypos;
    DWORD tuxheight = 0x427B3333; //should be a float but I don't know the exact floating pooint number
    SIZE_T lpNumberOfBytesWritten;
    INPUT launchfireball;
    launchfireball.type = INPUT_KEYBOARD;
    launchfireball.ki.wVk = VK_MENU;
    launchfireball.ki.wScan = MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC);
    launchfireball.ki.time = 0;
    launchfireball.ki.dwFlags = KEYEVENTF_SCANCODE;
    launchfireball.ki.dwExtraInfo = 0;
    SetForegroundWindow(stwnd);


    LPVOID tuxformaddr = traversepointerchain(tuxform_offs, 4);
    WriteProcessMemory(prochandle, tuxformaddr, &tuxform, sizeof(unsigned int), &lpNumberOfBytesWritten);

    LPVOID tuxheightaddr = traversepointerchain(tuxheight_offs, 5);
    WriteProcessMemory(prochandle, tuxheightaddr, &tuxheight, sizeof(float), &lpNumberOfBytesWritten);

    LPVOID fireballsaddr = traversepointerchain(fireballs_offs, 4);
    WriteProcessMemory(prochandle, fireballsaddr, &fireballs, sizeof(unsigned int), &lpNumberOfBytesWritten);

    LPVOID tuxyposaddr = traversepointerchain(tuxypos_offs, 5);
    ReadProcessMemory(prochandle, tuxyposaddr, &tuxypos, sizeof(float), &lpNumberOfBytesWritten);
    tuxypos = tuxypos-40;
    WriteProcessMemory(prochandle, tuxyposaddr, &tuxypos, sizeof(float), &lpNumberOfBytesWritten);

    while(TRUE){

        if(IsForegroundProcess(GetProcessId(prochandle)) && checkplaying()){
            LPVOID onGroundaddr = traversepointerchain(onGround_offs, 5);
            LPVOID directionaddr = traversepointerchain(direction_offs, 5);
            WriteProcessMemory(prochandle, onGroundaddr, &onGround, sizeof(unsigned char), &lpNumberOfBytesWritten);

            if(!checkplaying())
                continue;
            direction = 1;
            WriteProcessMemory(prochandle, directionaddr, &direction, sizeof(unsigned char), &lpNumberOfBytesWritten);
            launchfireball.ki.dwFlags = KEYEVENTF_SCANCODE;
            SendInput(1, &launchfireball, sizeof(INPUT));
            Sleep(5);
            launchfireball.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            SendInput(1, &launchfireball, sizeof(INPUT));

            Sleep(15);

            if(!checkplaying())
                continue;
            direction = 2;
            WriteProcessMemory(prochandle, directionaddr, &direction, sizeof(unsigned char), &lpNumberOfBytesWritten);
            launchfireball.ki.dwFlags = KEYEVENTF_SCANCODE;
            SendInput(1, &launchfireball, sizeof(INPUT));
            Sleep(5);
            launchfireball.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            SendInput(1, &launchfireball, sizeof(INPUT));
        }
        Sleep(25);
    }
}
static HANDLE threads[] = {NULL, NULL, NULL};
#define HACKACTIVE(X) ((threads[X] != NULL) ? "X": " ")
BOOLEAN start_stop(int i){
    switch (i)
    {
    case 1:
        if(threads[0] != NULL){
            TerminateThread(threads[0], 0);
            threads[0] = NULL;
            break;
        }
        threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) teleport, NULL, 0, NULL);
        break;
    case 2:
        if(threads[1] != NULL){
            TerminateThread(threads[1], 0);
            threads[1] = NULL;
            break;
        }
        threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) flyhack, NULL, 0, NULL);
        break;
    case 3:
        if(threads[2] != NULL){
            TerminateThread(threads[2], 0);
            threads[2] = NULL;
            break;
        }
        threads[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) freedomtux, NULL, 0, NULL);
        break;
    default:
        break;
    }
}

int main(void){
    prochandle = getsupertuxprocess();
    supertuxmodule = getsupertux2module();
    int hack;
    while(TRUE){
        printf("SuperTux Hacks\n"
               "The available hacks are:\n"
               "1 - Teleport hack - Teleport around with this neat cheat (CRTL to activate)                                       [%s]\n"
               "2 - Fly hack - \"Who said penguins can't fly?\" (Repeatedly press your jump key to fly around)                      [%s]\n"
               "3 - FredoomTux - \"Turn Tux into a true patriot and distribute freedom from above to the inhabitants of Antartica\" [%s]\n"
               "Please select the hack to activate/deactivate: ", HACKACTIVE(0), HACKACTIVE(1), HACKACTIVE(2));
        scanf("%d", &hack);
        start_stop(hack);
    }
}