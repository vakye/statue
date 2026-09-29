
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

        SetClearColor(V4(0.07f, 0.08f, 0.1f, 1.0f));
        BeginRendering();
        EndRendering();

        PresentWindow();
    }
}

