
// ==================================================================
// NOTE(vak): Forward declarations for rendering-related definitions
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

// NOTE(vak): All coordinates are window coordinates:
//      + Origin (0, 0) is located at the top-left
//      + Positive X goes right.
//      + Positive Y goes down.

local void SetupRenderer    (void);
local void SetClearColor    (v4 Color);
local void BeginRendering   (void);
local void RenderRect       (rect2 Rect, v4 Color);
local void EndRendering     (void);

// ==================================================================
// NOTE(vak): Implementations are contained inside *_render.c.
// Currently, Vulkan is the default graphics API used to implement
// the renderer:
//
//      Vulkan: vulkan_render.c
//
// ==================================================================

