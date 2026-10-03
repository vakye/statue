
// ==================================================================
// NOTE(vak): Linux implementation of platform.c
// ==================================================================

#pragma once

#include "wayland_window.c"
#include <dlfcn.h>

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    char** Envp;
} linux_state;

local linux_state Linux = {0};

local void LinuxEquipEnvp(char** Envp)
{
    Linux.Envp = Envp;
}

local void SetupWindow(void)
{
    string XdgSessionType = GetEnv(Str("XDG_SESSION_TYPE"));

    if (!StringEquals(XdgSessionType, Str("wayland")))
    {
        Println(StdErr, Str("error: Only Wayland is supported for now."));
        Exit(1);
    }

    WaylandSetupWindow();
}

local void ToggleFullscreen(void)
{
    WaylandToggleFullscreen();
}

local u32 GetWindowSizeX(void)
{
    return WaylandGetWindowSizeX();
}

local u32 GetWindowSizeY(void)
{
    return WaylandGetWindowSizeY();
}

local b32 IsWindowClosed(void)
{
    return WaylandIsWindowClosed();
}

local void PollEvents(void)
{
    InputPrepareForFrame();
    WaylandPollEvents();
}

local void PresentWindow(void)
{
    WaylandPresentWindow();
}

local void* GetVulkanLoader(void)
{
    void* VulkanLibrary = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);

    if (!VulkanLibrary)
        VulkanLibrary = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);

    if (!VulkanLibrary)
        return (0);

    return dlsym(VulkanLibrary, "vkGetInstanceProcAddr");
}

local void* ReserveMemory(usize Size)
{
    void* Result = mmap(0, Size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);

    if ((ssize)PointerToInteger(Result) < 0)
    {
        Println(StdErr, Str("error: failed to reserve memory"));
        Exit(1);
    }

    return (Result);
}

local void CommitMemory(void* Memory, usize Size)
{
    s32 Result = mprotect(Memory, Size, PROT_READ|PROT_WRITE);
    if (Result < 0)
    {
        Println(StdErr, Str("error: failed to commit memory"));
        Exit(1);
    }
}

local usize GetWallClock(void)
{
    struct timespec Now = {0};
    clock_gettime(CLOCK_MONOTONIC, &Now);

    usize Result =
        ((Now.tv_sec  & 0x3FFFFFFFF) << 30) |
        ((Now.tv_nsec & 0x3FFFFFFF));

    return (Result);
}

local f64 GetSecondsElapsed(usize FromWallClock, usize ToWallClock)
{
    ssize Delta         = (ssize)ToWallClock - (ssize)FromWallClock;
    ssize DeltaSec      = (Delta >> 30);
    ssize DeltaNanosec  = (Delta & 0x3FFFFFFF);

    f64 Result = (f64)DeltaSec + (f64)DeltaNanosec * 1e-9;
    return (Result);
}

local void WaitSeconds(f64 Seconds)
{
    usize Nanoseconds = (usize)(Seconds * 1e9);

    struct timespec Duration =
    {
        .tv_sec     = Nanoseconds / 1000000000,
        .tv_nsec    = Nanoseconds % 1000000000,
    };

    while (Duration.tv_sec && Duration.tv_nsec)
    {
        struct timespec Remainder = {0};
        nanosleep(&Duration, &Remainder);

        Duration = Remainder;
    }
}

local string GetEnv(string Name)
{
    string Found = NilString;

    for (usize Index = 0; Linux.Envp[Index] != 0; Index++)
    {
        string Candidate = CString(Linux.Envp[Index]);

        if (StringStartsWith(Candidate, Name))
        {
            Found = Candidate;
            break;
        }
    }

    string Result = NilString;

    if (!IsNilString(Found))
        Result = StringView(Found, Name.Size + 1, USizeMax);

    return (Result);
}

local usize WriteStdOut(void* Data, usize Size)
{
    ssize Written = write(STDOUT_FILENO, Data, Size);
    usize Result = Maximum(0, Written);
    return (Result);
}

local usize WriteStdErr(void* Data, usize Size)
{
    ssize Written = write(STDERR_FILENO, Data, Size);
    usize Result = Maximum(0, Written);
    return (Result);
}

local void Exit(u8 Code)
{
    exit_group(Code);
}

