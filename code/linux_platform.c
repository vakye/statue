
// ==================================================================
// NOTE(vak): Linux implementation of platform.c
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): State
// ==================================================================

typedef struct
{
    char** Envp;
} linux_state;

local linux_state LinuxState = {0};

local void LinuxEquipEnvp(char** Envp)
{
    LinuxState.Envp = Envp;
}

// ==================================================================
// NOTE(vak): Standard file descriptors numbers
// ==================================================================

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

local string GetEnv(string Name)
{
    string Found = NilString;

    for (usize Index = 0; LinuxState.Envp[Index] != 0; Index++)
    {
        string Candidate = CString(LinuxState.Envp[Index]);

        if (StringStartsWith(Candidate, Name))
        {
            Found = Candidate;
            break;
        }
    }

    string Result = NilString;

    if (!IsNilString(Found))
        Result = StringView(Found, Name.Size + 1, USizeMax);

    return (Result);
}

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

