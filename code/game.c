
// ==================================================================
// NOTE(vak): Game code
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

typedef struct
{
    entity_id CameraID;
    entity_id PlayerID;

    usize LastShootTime;
    f32 ShootCooldown;

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

    {
        Game->CameraID = MakeEntity();

        f32 ViewHeight = 30.0f;

        SetEntityP      (Game->CameraID, V2(0, 0));
        SetEntitySize   (Game->CameraID, V2(0, ViewHeight));
    }

    {
        Game->PlayerID = MakeEntity();
        Game->ShootCooldown = 0.01f;

        SetEntityProp   (Game->PlayerID, EntityProp_Render, true);
        SetEntityP      (Game->PlayerID, V2(0, 0));
        SetEntitySize   (Game->PlayerID, V2(1, 1));
        SetEntityColor  (Game->PlayerID, V4(1.0f, 0.8f, 0.5f, 1.0f));
    }

    {
        entity_id SomeEntityID = MakeEntity();

        SetEntityProp   (SomeEntityID, EntityProp_Render, true);
        SetEntityP      (SomeEntityID, V2(4, 2));
        SetEntitySize   (SomeEntityID, V2(1, 1));
        SetEntityColor  (SomeEntityID, V4(0.9f, 0.9f, 0.9f, 1.0f));
    }
}

local v2 ToWorldUnits(game_state* Game, v2 ScreenP)
{
    v2 CameraP      = GetEntityP(Game->CameraID);
    v2 CameraSize   = GetEntitySize(Game->CameraID);
    v2 CameraMin    = V2Sub(CameraP, V2MulScalar(CameraSize, 0.5f));
    v2 WindowSize   = V2((f32)GetWindowSizeX(), (f32)GetWindowSizeY());

    v2 Result = V2Add(V2Mul(V2Div(ScreenP, WindowSize), CameraSize), CameraMin);

    return (Result);
}

local void GameTick(game_state* Game, f32 DeltaTime)
{
    // NOTE(vak): Update camera view size
    {
        f32 AspectRatio = (f32)GetWindowSizeX() / (f32)GetWindowSizeY();
        v2 ViewSize = GetEntitySize(Game->CameraID);

        v2 NewViewSize = ViewSize;
        NewViewSize.X = ViewSize.Y * AspectRatio;

        SetEntitySize(Game->CameraID, NewViewSize);
    }

    // NOTE(vak): Update camera force
    {
        v2 MouseWorldP = ToWorldUnits(Game, InputGetMouseP());
        v2 PlayerP = GetEntityP(Game->PlayerID);
        v2 CameraP = GetEntityP(Game->CameraID);

        f32 MouseOffsetFactor = 0.05f;
        v2 MouseOffset = V2ScalarMul(MouseOffsetFactor, V2Sub(MouseWorldP, PlayerP));

        v2 TargetP = V2Add(PlayerP, MouseOffset);
        v2 Delta = V2Sub(TargetP, CameraP);

        f32 Friction = 50.0f;
        f32 Force = Friction * 5.0f;

        v2 CameraDP = GetEntityDP(Game->CameraID);

        v2 ForceToApply = V2Sub(
            V2MulScalar(Delta, Force),
            V2MulScalar(CameraDP, Friction)
        );

        SetEntityForce(Game->CameraID, ForceToApply);
    }

    // NOTE(vak): Update player movement
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

        v2 PlayerDP = GetEntityDP(Game->PlayerID);

        v2 ForceToApply = V2Sub(
            V2MulScalar(MoveDirection, Force),
            V2MulScalar(PlayerDP, Friction)
        );

        SetEntityForce(Game->PlayerID, ForceToApply);
    }

    // NOTE(vak): Update player shoot
    if (InputIsDown(InputButton_MouseLeft))
    {
        f32 Elapsed = GetSecondsElapsed(Game->LastShootTime, GetWallClock());
        if (Elapsed >= Game->ShootCooldown)
        {
            v2 MouseWorldP = ToWorldUnits(Game, InputGetMouseP());
            v2 PlayerP = GetEntityP(Game->PlayerID);

            v2 PlayerDP = GetEntityDP(Game->PlayerID);
            v2 AimDirection = V2NormalizeOrZero(V2Sub(MouseWorldP, PlayerP));

            f32 Lifetime = 1.0f;

            f32 Radius = 1.05f * V2Length(GetEntitySize(Game->PlayerID));
            f32 Speed = 20.0f + Maximum(0.0f, V2Dot(PlayerDP, AimDirection));
            f32 Friction = Speed / Lifetime;

            v2 P        = V2Add(PlayerP, V2MulScalar(AimDirection, Radius));
            v2 DP       = V2MulScalar(AimDirection, Speed);
            v2 DDP      = V2MulScalar(AimDirection, -Friction);
            v2 Size     = V2(0.4f, 0.4f);
            v4 Color    = V4(0.6f, 0.7f, 1.0f, 1.0f);

            entity_id BulletID = MakeEntity();

            SetEntityProp       (BulletID, EntityProp_Render, true);
            SetEntityProp       (BulletID, EntityProp_Lifetime, true);
            SetEntityP          (BulletID, P);
            SetEntityDP         (BulletID, DP);
            SetEntityForce      (BulletID, DDP);
            SetEntitySize       (BulletID, Size);
            SetEntityColor      (BulletID, Color);
            SetEntityLifetime   (BulletID, Lifetime);

            Game->LastShootTime = GetWallClock();
        }
    }

    // NOTE(vak): Do physics, timed removal, ... for everyone

    for (
        entity_iter Iter = IterateEntities();
        Iter.EntityID;
        NextEntity(&Iter)
    )
    {
        UpdateEntity(Iter.EntityID, DeltaTime);
    }
}

local v2 ToPredictedScreenUnits(game_state* Game, v2 WorldP)
{
    v2 CameraP      = GetEntityPredictedP(Game->CameraID, Game->StrayTime);
    v2 CameraSize   = GetEntitySize(Game->CameraID);
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

    for (
        entity_iter Iter = IterateEntities();
        Iter.EntityID;
        NextEntity(&Iter)
    )
    {
        entity_id EntityID = Iter.EntityID;

        if (GetEntityProp(EntityID, EntityProp_Render))
        {
            v2 P        = GetEntityPredictedP(EntityID, StrayTime);
            v2 Size     = GetEntitySize(EntityID);
            v4 Color    = GetEntityColor(EntityID);

            GameDrawRect(Game, R2CenterSize(P, Size), Color);
        }
    }

    EndRendering();
}

