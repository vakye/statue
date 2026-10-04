
// ==================================================================
// NOTE(vak): Game code
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

typedef struct
{
    // NOTE(vak): Update

    v2  CameraP;
    v2  CameraLastP;
    v2  CameraDDP;
    f32 FocalLength;
    f32 AspectRatio;

    v2 PlayerP;
    v2 PlayerLastP;
    v2 PlayerDDP;
    v2 PlayerSize;
    v4 PlayerColor;

    // NOTE(vak): Render

    f32 StrayTime;
} game_state;

local void GameSetup    (game_state* Game);
local void GameTick     (game_state* Game, f32 DeltaTime);
local void GameRender   (game_state* Game, f32 StrayTime);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

local void GameSetup(game_state* Game)
{
    ZeroStruct(Game);

    Game->CameraP       = V2(0, 0);
    Game->FocalLength   = 0.035f;
    Game->AspectRatio   = 1.0f;

    Game->PlayerP       = V2(0, 0);
    Game->PlayerLastP   = Game->PlayerP;
    Game->PlayerSize    = V2(1.0f, 1.0f);
    Game->PlayerColor   = V4(1.0f, 0.8f, 0.5f, 1.0f);
}

local v2 ToWorldUnits(game_state* Game, v2 ScreenP)
{
    v2 CameraP      = Game->CameraP;
    v2 CameraSize   = V2(Game->AspectRatio / Game->FocalLength, 1.0f / Game->FocalLength);
    v2 CameraMin    = V2Sub(CameraP, V2MulScalar(CameraSize, 0.5f));
    v2 WindowSize   = V2((f32)GetWindowSizeX(), (f32)GetWindowSizeY());

    v2 Result = V2Add(V2Mul(V2Div(ScreenP, WindowSize), CameraSize), CameraMin);

    return (Result);
}

local void GameTick(game_state* Game, f32 DeltaTime)
{
    Game->AspectRatio = (f32)GetWindowSizeX() / (f32)GetWindowSizeY();

    {
        b32 MoveU = InputIsDown(InputButton_KeyW) || InputIsDown(InputButton_KeyUp);
        b32 MoveD = InputIsDown(InputButton_KeyS) || InputIsDown(InputButton_KeyDown);
        b32 MoveL = InputIsDown(InputButton_KeyA) || InputIsDown(InputButton_KeyLeft);
        b32 MoveR = InputIsDown(InputButton_KeyD) || InputIsDown(InputButton_KeyRight);

        v2 MoveDirection = V2NormalizeOrZero(V2(
            1.0f * ((s32)MoveR - (s32)MoveL),
            1.0f * ((s32)MoveD - (s32)MoveU)
        ));

        f32 Friction = 50.0f;
        f32 Force = Friction * 8.0f;

        v2 PlayerDP = V2DivScalar(V2Sub(Game->PlayerP, Game->PlayerLastP), DeltaTime);

        Game->PlayerDDP = V2Sub(
            V2MulScalar(MoveDirection, Force),
            V2MulScalar(PlayerDP, Friction)
        );

        v2 ChangeInP = V2Add(
            V2Sub(Game->PlayerP, Game->PlayerLastP),
            V2MulScalar(Game->PlayerDDP, 0.5f*Square(DeltaTime))
        );

        Game->PlayerLastP = Game->PlayerP;
        Game->PlayerP = V2Add(Game->PlayerP, ChangeInP);
    }

    {
        v2 MouseWorldP = ToWorldUnits(Game, InputGetMouseP());

        f32 MouseOffsetFactor = 0.05f;
        v2 MouseOffset = V2ScalarMul(MouseOffsetFactor, V2Sub(MouseWorldP, Game->PlayerP));

        v2 TargetP = V2Add(Game->PlayerP, MouseOffset);
        v2 Delta = V2Sub(TargetP, Game->CameraP);

        f32 Friction = 50.0f;
        f32 Force = Friction * 5.0f;

        v2 CameraDP = V2DivScalar(V2Sub(Game->CameraP, Game->CameraLastP), DeltaTime);

        Game->CameraDDP = V2Sub(
            V2MulScalar(Delta, Force),
            V2MulScalar(CameraDP, Friction)
        );

        v2 ChangeInP = V2Add(
            V2Sub(Game->CameraP, Game->CameraLastP),
            V2MulScalar(Game->CameraDDP, 0.5f*Square(DeltaTime))
        );

        Game->CameraLastP = Game->CameraP;
        Game->CameraP = V2Add(Game->CameraP, ChangeInP);
    }
}

local v2 ToPredictedScreenUnits(game_state* Game, v2 WorldP)
{
    v2 PredictedDelta = V2Add(
        V2Sub(Game->CameraP, Game->CameraLastP),
        V2MulScalar(Game->CameraDDP, 0.5f*Square(Game->StrayTime))
    );

    v2 CameraP      = V2Add(Game->CameraP, PredictedDelta);
    v2 CameraSize   = V2(Game->AspectRatio / Game->FocalLength, 1.0f / Game->FocalLength);
    v2 CameraMin    = V2Sub(CameraP, V2MulScalar(CameraSize, 0.5f));
    v2 WindowSize   = V2((f32)GetWindowSizeX(), (f32)GetWindowSizeY());

    v2 Result = V2Mul(V2Div(V2Sub(WorldP, CameraMin), CameraSize), WindowSize);

    return (Result);
}

local void GameDrawRect(game_state* Game, rect2 RectInWorld, v4 Color)
{
    rect2 RectInScreen = R2MinMax(
        ToPredictedScreenUnits(Game, RectInWorld.Min),
        ToPredictedScreenUnits(Game, RectInWorld.Max)
    );

    RenderRect(RectInScreen, Color);
}

local void GameRender(game_state* Game, f32 StrayTime)
{
    Game->StrayTime = StrayTime;

    SetClearColor(V4(0.07f, 0.08f, 0.1f, 1.0f));
    BeginRendering();

    {
        v2 PredictedDelta = V2Add(
            V2Sub(Game->PlayerP, Game->PlayerLastP),
            V2MulScalar(Game->PlayerDDP, 0.5f*Square(StrayTime))
        );

        v2 PlayerP      = V2Add(Game->PlayerP, PredictedDelta);
        v2 PlayerSize   = Game->PlayerSize;
        v4 PlayerColor  = Game->PlayerColor;

        GameDrawRect(Game, R2CenterSize(PlayerP, PlayerSize), PlayerColor);
    }

    GameDrawRect(Game, R2CenterSize(V2(4.0f, 4.0f), V2(1.0f, 1.0f)), V4(0.9f, 0.9f, 0.9f, 1.0f));

    EndRendering();
}

