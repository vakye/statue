
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

#include "print.c"
#include "math.c"
#include "render.c"

local void Main(void)
{
    SetupWindow();
    SetupRenderer();

    while (!IsWindowClosed())
    {
        PollEvents();

        SetClearColor(V4(0.9f, 0.9f, 0.9f, 1.0f));
        BeginRendering();
        EndRendering();

        PresentWindow();
    }
}

