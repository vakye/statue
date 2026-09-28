
// ==================================================================
// NOTE(vak): Forward declarations for rendering-related definitions
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

local void SetupRenderer    (void);
local void SetClearColor    (v4 Color);
local void BeginRendering   (void);
local void EndRendering     (void);

// ==================================================================
// NOTE(vak): Implementations are contained inside *_render.c.
// Currently, Vulkan is the default graphics API used to implement
// the renderer:
//
//      Vulkan: vulkan_render.c
//
// ==================================================================

