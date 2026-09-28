
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

#include "print.c"

local void Main(void)
{
    SetupWindow();

    while (!IsWindowClosed())
    {
        PollEvents();
        PresentWindow();
    }
}

