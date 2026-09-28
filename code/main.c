
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

local void Main(void)
{
    char Message[] = "Hello, world!\n";

    WriteStdOut(Message, sizeof(Message) - 1);
}

