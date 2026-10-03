
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

local void Main(void)
{
    SetupWindow();
    SetupRenderer();
    ToggleFullscreen();

    f64 AccumulatedTime = 0.0f;
    f32 UpdateTimeStep  = 1.0f/1.0f;
    f32 RenderTimeStep  = 1.0f/2.0f;

    usize FrameBegin = GetWallClock();

    while (!IsWindowClosed())
    {
        PollEvents();

        // NOTE(vak): Controls

        if (InputIsPressed(InputButton_KeyF11))
            ToggleFullscreen();

        // NOTE(vak): Update
        while (AccumulatedTime >= UpdateTimeStep)
        {
            Println(StdOut, Str("Update..."));
            AccumulatedTime -= UpdateTimeStep;
        }

        // NOTE(vak): Render
        {
            Println(StdOut, Str("Render..."));

            SetClearColor(V4(1.0f, 1.0f, 1.0f, 1.0f));
            BeginRendering();

            EndRendering();
        }

        PresentWindow();

        f64 DeltaTime = GetSecondsElapsed(FrameBegin, GetWallClock());

        while (DeltaTime < RenderTimeStep)
        {
            WaitSeconds(RenderTimeStep - DeltaTime);
            DeltaTime = GetSecondsElapsed(FrameBegin, GetWallClock());
        }

        AccumulatedTime += DeltaTime;
        FrameBegin = GetWallClock();
    }
}

