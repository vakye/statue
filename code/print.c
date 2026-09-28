
// ==================================================================
// NOTE(vak): A collection of printing functions for
// characters, strings, numbers, ... with used-specified
// output.
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

// NOTE(vak): All print functions returns the number of bytes
// written

typedef usize print_dump(void* Data, usize Size, void* UserData);

typedef struct
{
    print_dump* Dump;
    void*       UserData;
} print_out;

local usize DumpToStdOut(void* Data, usize Size, void* UserData);
local usize DumpToStdErr(void* Data, usize Size, void* UserData);

#define StdOut (print_out){&DumpToStdOut, 0}
#define StdErr (print_out){&DumpToStdErr, 0}

local usize PrintWrite      (print_out Out, void* Data, usize Size);
local usize PrintCharacter  (print_out Out, char Character);
local usize PrintNewLine    (print_out Out);
local usize Print           (print_out Out, string Message);
local usize Println         (print_out Out, string Message);
local usize PrintUSize      (print_out Out, usize Value);
local usize PrintSSize      (print_out Out, ssize Value);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

local usize DumpToStdOut(void* Data, usize Size, void* UserData)
{
    Unused(UserData);
    return WriteStdOut(Data, Size);
}

local usize DumpToStdErr(void* Data, usize Size, void* UserData)
{
    Unused(UserData);
    return WriteStdErr(Data, Size);
}

local usize PrintWrite(print_out Out, void* Data, usize Size)
{
    usize Result = Out.Dump(Data, Size, Out.UserData);
    return (Result);
}

local usize PrintCharacter(print_out Out, char Character)
{
    usize Result = PrintWrite(Out, &Character, 1);
    return (Result);
}

local usize PrintNewLine(print_out Out)
{
    usize Result = PrintCharacter(Out, '\n');
    return (Result);
}

local usize Print(print_out Out, string Message)
{
    usize Result = PrintWrite(Out, Message.Data, Message.Size);
    return (Result);
}

local usize Println(print_out Out, string Message)
{
    usize Result = 0;

    Result += Print(Out, Message);
    Result += PrintNewLine(Out);

    return (Result);
}

local usize PrintUSize(print_out Out, usize Value)
{
    char Buffer[USizeBits];

    usize DigitIndex = ArrayCount(Buffer);
    usize DigitCount = 0;

    do
    {
        char Digit = '0' + (Value % 10);
        Value /= 10;

        DigitIndex--;
        DigitCount++;

        Buffer[DigitIndex] = Digit;
    } while (Value);

    usize Result = Print(Out, StrData(Buffer + DigitIndex, DigitCount));
    return (Result);
}

local usize PrintSSize(print_out Out, ssize Value)
{
    usize Result = 0;

    if (Value < 0)
    {
        Result += PrintCharacter(Out, '-');
        Value = -Value;
    }

    Result += PrintUSize(Out, Value);
    return (Result);
}

