
// ==================================================================
// NOTE(vak): Wayland protocol window + input implementation
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

local void WaylandSetupWindow       (void);
local u32  WaylandGetWindowSizeX    (void);
local u32  WaylandGetWindowSizeY    (void);
local b32  WaylandIsWindowClosed    (void);
local void WaylandPollEvents        (void);
local void WaylandPresentWindow     (void);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    s32 SizeX;
    s32 SizeY;
    b32 IsClosed;
} wayland_state;

local wayland_state Wayland = {0};

local void WaylandSetupWindow(void)
{
}

local u32 WaylandGetWindowSizeX(void)
{
    return Maximum(0, Wayland.SizeX);
}

local u32 WaylandGetWindowSizeY(void)
{
    return Maximum(0, Wayland.SizeY);
}

local b32 WaylandIsWindowClosed(void)
{
    return (Wayland.IsClosed);
}

local void WaylandPollEvents(void)
{
}

local void WaylandPresentWindow(void)
{
}

