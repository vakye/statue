
// ==================================================================
// NOTE(vak): Linux implementation of platform.c
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Standard file descriptors numbers
// ==================================================================

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

local usize WriteStdOut(void* Data, usize Size)
{
    ssize Written = write(STDOUT_FILENO, Data, Size);
    usize Result = Maximum(0, Written);
    return (Result);
}

local usize WriteStdErr(void* Data, usize Size)
{
    ssize Written = write(STDERR_FILENO, Data, Size);
    usize Result = Maximum(0, Written);
    return (Result);
}

local void Exit(u8 Code)
{
    exit_group(Code);
}

