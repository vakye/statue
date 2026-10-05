
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

local void Main(void)
{
    SetupWindow();
    SetupRenderer();
    ToggleFullscreen();

    game_state Game = {0};
    GameSetup(&Game);

    f64 AccumulatedTime = 0.0f;
    f32 UpdateTimeStep  = 1.0f/80.0f;
    f32 RenderTimeStep  = 1.0f/GetRefreshRate();

    time FrameBegin = GetWallClock();

    while (!IsWindowClosed())
    {
        PollEvents();

        if (InputIsPressed(InputButton_KeyF11))
            ToggleFullscreen();

        while (AccumulatedTime >= UpdateTimeStep)
        {
            GameTick(&Game, UpdateTimeStep);
            AccumulatedTime -= UpdateTimeStep;
        }

        GameRender(&Game, AccumulatedTime);

        PresentWindow();

        f64 DeltaTime = GetSecondsElapsed(FrameBegin, GetWallClock());

        while (DeltaTime < RenderTimeStep)
        {
            Wait(RenderTimeStep - DeltaTime);
            DeltaTime = GetSecondsElapsed(FrameBegin, GetWallClock());
        }

        AccumulatedTime += DeltaTime;
        FrameBegin = GetWallClock();
    }
}

