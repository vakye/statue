
// ==================================================================
// NOTE(vak): Forward declarations for platform-related definitions
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

local void      SetupWindow     (void);
local u32       GetWindowSizeX  (void);
local u32       GetWindowSizeY  (void);
local b32       IsWindowClosed  (void);
local void      PollEvents      (void);
local void      PresentWindow   (void);

local string    GetEnv      (string Name);
local usize     WriteStdOut (void* Data, usize Size);
local usize     WriteStdErr (void* Data, usize Size);
local void      Exit        (u8 Code);

// ==================================================================
// NOTE(vak): Implementations are contained inside *_platform.c
// files, and are selected depending on the operating system:
//
//      Linux: linux_platform.c
//
// ==================================================================

