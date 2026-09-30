
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

local void Main(void)
{
    SetupWindow();
    SetupRenderer();

    while (!IsWindowClosed())
    {
        PollEvents();

        SetClearColor(V4(0.07f, 0.08f, 0.1f, 1.0f));
        BeginRendering();

        RenderRect(
            R2MinMax(V2(-0.5f, -0.5f), V2(0.5f, 0.5f)),
            V4(1.0f, 0.8f, 0.5f, 1.0f)
        );

        RenderRect(
            R2MinMax(V2(-0.25f, -0.25f), V2(0.25f, 0.25f)),
            V4(0.4f, 0.5f, 1.0f, 1.0f)
        );

        EndRendering();

        PresentWindow();
    }
}

