
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

#include "print.c"

local void Main(void)
{
    SetupWindow();

    void* MyVulkanLoader = GetVulkanLoader();

    PrintUSize(StdOut, PointerToInteger(MyVulkanLoader));
    PrintNewLine(StdOut);

    while (!IsWindowClosed())
    {
        PollEvents();
        PresentWindow();
    }
}

