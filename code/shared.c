
// ==================================================================
// NOTE(vak): A collection of commonly used definitions:
//      + Compiler detection
//      + Architecture detection
//      + Operating system detection
//      + Intrinsics includes (immintrin.h, ...)
//      + Keywords
//      + Macros
//      + Types
//      + Constants
//      + Memory zero, fill, copy, ...
//      + UTF-8 string
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Compiler detection
// ==================================================================

#if defined(__clang__)
    #define CompilerClang (1)
#endif

#if defined(_MSC_VER)
    #define CompilerMSVC (1)
#elif defined(__GNUC__)
    #define CompilerGCC (1)
#endif

#if !defined(CompilerClang) && \
    !defined(CompilerMSVC) && \
    !defined(CompilerGCC)
#error Unknown compiler
#endif

#if !defined(CompilerClang)
    #define CompilerClang (0)
#endif

#if !defined(CompilerMSVC)
    #define CompilerMSVC (0)
#endif

#if !defined(CompilerGCC)
    #define CompilerGCC (0)
#endif

// ==================================================================
// NOTE(vak): Architecture detection
// ==================================================================

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64) || defined(_M_X64)
    #define ArchitectureX64 (1)
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define ArchitectureARM64 (1)
#else
    #error Unknown architecture
#endif

#if !defined(ArchitectureX64)
    #define ArchitectureX64 (0)
#endif

#if !defined(ArchitectureARM64)
    #define ArchitectureARM64 (0)
#endif

// ==================================================================
// note(vak): operating system detection
// ==================================================================

#if defined(_WIN32)
    #define PlatformWindows (1)
#elif defined(__linux__)
    #define PlatformLinux (1)
#elif defined(__APPLE__)
    #define PlatformMacOS (1)
#else
    #error Unknown operating system
#endif

#if !defined(PlatformWindows)
    #define PlatformWindows (0)
#endif

#if !defined(PlatformLinux)
    #define PlatformLinux (0)
#endif

#if !defined(PlatformMacOS)
    #define PlatformMacOS (0)
#endif

// ==================================================================
// NOTE(vak): Intrinsics include
// ==================================================================

#if ArchitectureX64
    #include <immintrin.h>
#endif

// ==================================================================
// NOTE(vak): Keywords
// ==================================================================

#define local static
#define persist static

// ==================================================================
// NOTE(vak): Macros
// ==================================================================

#define Unused(Name) ((void)(Name))

#define CompileTimeAssert(Expression) \
    _Static_assert(Expression, "Compile-time assertion failed")

#define IntegerToPointer(Integer) ((void*)((usize)(Integer)))
#define PointerToInteger(Pointer) ((usize)(Pointer))

#define ArrayCount(Array) \
    (sizeof(Array) / sizeof((Array)[0]))

#define OffsetOf(StructType, Member) \
    IntegerToPointer(&((StructType*)0)->Member)

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Clamp(Min, Value, Max) Maximum(Min, Minimum(Max, Value))

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

#define Thousand(Amount)    ((Amount) * 1000)
#define Million(Amount)     ((Amount) * 1000000)
#define Billion(Amount)     ((Amount) * 1000000000ull)

#define AlignDown(Value, PowerOf2) ((Value) & ~((PowerOf2) - 1))
#define AlignUp(Value, PowerOf2) AlignDown((Value) + (PowerOf2) - 1, PowerOf2)

#define IsAligned(Value, PowerOf2) (((Value) & ((PowerOf2) - 1)) == 0)
#define IsPowerOf2(Value) (((Value) > 0) && IsAligned(Value, Value))

// ==================================================================
// NOTE(vak): Types
// ==================================================================

typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef s64 ssize;
typedef u64 usize;

typedef float   f32;
typedef double  f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

// ==================================================================
// NOTE(vak): Type size checks
// ==================================================================

CompileTimeAssert(sizeof(s8)  == 1);
CompileTimeAssert(sizeof(s16) == 2);
CompileTimeAssert(sizeof(s32) == 4);
CompileTimeAssert(sizeof(s64) == 8);

CompileTimeAssert(sizeof(u8)  == 1);
CompileTimeAssert(sizeof(u16) == 2);
CompileTimeAssert(sizeof(u32) == 4);
CompileTimeAssert(sizeof(u64) == 8);

CompileTimeAssert(sizeof(ssize) == 8);
CompileTimeAssert(sizeof(usize) == 8);

CompileTimeAssert(sizeof(f32) == 4);
CompileTimeAssert(sizeof(f64) == 8);

// ==================================================================
// NOTE(vak): Constants
// ==================================================================

#define true    (1)
#define false   (0)

#define S8Min    ((s8 )(-128))
#define S16Min   ((s16)(-32768))
#define S32Min   ((s32)(-2147483648))
#define S64Min   ((s64)(-9223372036854775808ll))

#define S8Max    ((s8 )(+127))
#define S16Max   ((s16)(+32767))
#define S32Max   ((s32)(+2147483647))
#define S64Max   ((s64)(+9223372036854775807ll))

#define U8Max    ((u8 )(+255))
#define U16Max   ((u16)(+65535))
#define U32Max   ((u32)(+4294967295))
#define U64Max   ((u64)(+18446744073709551615ull))

#define F32Eps   (1.1920928955078125e-7f)
#define F32Max   (3.40282346638528859812e+38f)
#define F32Min   (-F32Max)

#define F64Eps   (2.22044604925031308085e-16f)
#define F64Max   (1.79769313486231570815e+308)
#define F64Min   (-F64Max)

#define USizeBits (sizeof(usize) * 8)

#define SSizeMin  ((ssize)(1ull << (USizeBits - 1)))
#define SSizeMax  ((ssize)((usize)SSizeMin - 1))
#define USizeMax  (~((usize)0))

// ==================================================================
// NOTE(vak): Memory
// ==================================================================

void* memset(void* DestInit, s32 Byte, usize Size);
void* memcpy(void* DestInit, const void* SourceInit, usize Size);

local void MemoryZero(void* DestInit,                   usize Size) { memset(DestInit, 0,           Size); }
local void MemoryFill(void* DestInit, u8 Byte,          usize Size) { memset(DestInit, Byte,        Size); }
local void MemoryCopy(void* DestInit, void* SourceInit, usize Size) { memcpy(DestInit, SourceInit,  Size); }

#define ZeroStruct(Pointer)         MemoryZero(Pointer,         sizeof(*(Pointer)))
#define ZeroArray(FixedSizeArray)   MemoryZero(FixedSizeArray,  sizeof(FixedSizeArray))

// ==================================================================
// NOTE(vak): UTF-8 string
// ==================================================================

typedef struct
{
    char* Data;
    usize Size;
} string;

#define NilString           (string){0}
#define IsNilString(String) (!(String).Data || !(String).Size)

#define Str(Literal)        (string){Literal, sizeof(Literal) - 1}
#define StrData(Data, Size) (string){Data, Size}

local string CString(const char* Data)
{
    string Result = NilString;

    if (Data)
    {
        Result = StrData((char*)Data, 0);

        while (Data[Result.Size] != '\0')
            Result.Size++;
    }

    return (Result);
}

local string StringView(string Source, usize From, usize Size)
{
    From = Minimum(From, Source.Size);
    Size = Minimum(Size, Source.Size - From);

    string Result = StrData(Source.Data + From, Size);
    return (Result);
}

local b32 StringEquals(string A, string B)
{
    b32 Result = (A.Size == B.Size);

    if (Result)
    {
        for (usize Index = 0; Index < A.Size; Index++)
        {
            if (A.Data[Index] != B.Data[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return (Result);
}

local b32 StringStartsWith(string String, string Match)
{
    b32 Result = (String.Size >= Match.Size);

    if (Result)
    {
        for (usize Index = 0; Index < Match.Size; Index++)
        {
            if (String.Data[Index] != Match.Data[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return (Result);
}

