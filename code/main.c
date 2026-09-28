
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

#include "print.c"

local void Main(void)
{
    Println(StdOut, Str("Hello, world!"));

    PrintUSize(StdOut, 1337);
    PrintNewLine(StdOut);

    PrintUSize(StdOut, 123456789);
    PrintNewLine(StdOut);

    PrintSSize(StdOut, -123456789);
    PrintNewLine(StdOut);

    PrintSSize(StdOut, -9376);
    PrintNewLine(StdOut);
}

