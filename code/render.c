
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

// NOTE(vak): All textures are assumed to be RGBA8

#define MaxTextureCount (64)

typedef u32 texture_id;

local void          SetupRenderer       (void);

local texture_id    MakeTexture         (u32 SizeX, u32 SizeY);
local u32           GetTextureSizeX     (texture_id TextureID);
local u32           GetTextureSizeY     (texture_id TextureID);
local void          UploadTexture       (texture_id TextureID, void* PixelsRGBA);

local void          SetClearColor       (v4 Color);
local void          BeginRendering      (void);
local void          RenderRect          (rect2 Rect, v4 Color);
local void          RenderRectTextured  (rect2 Rect, v4 Color, rect2 TextureMap, texture_id TextureID);
local void          EndRendering        (void);

// ==================================================================
// NOTE(vak): Implementations are contained inside *_render.c.
// Currently, Vulkan is the default graphics API used to implement
// the renderer:
//
//      Vulkan: vulkan_render.c
//
// ==================================================================

