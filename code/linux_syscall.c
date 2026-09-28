
// ==================================================================
// NOTE(vak): Implements syscall wrappers for Linux
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Open flags
// ==================================================================

#define O_RDONLY (0)

// ==================================================================
// NOTE(vak): Standard file descriptors numbers
// ==================================================================

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

// ==================================================================
// NOTE(vak): File stat
// ==================================================================

// NOTE(vak): We don't target 32 bit architectures so having
// only the 64-bit version here should be okay.

// NOTE(vak): Taken from /usr/include/asm-generic/stat.h

struct stat
{
    u64 Device;
    u64 FileSerialNumber;
    u32 Mode;
    u32 LinkCount;
    u32 UserID;
    u32 GroupID;
    u64 DeviceNumber;
    u64 Pad1;
    s64 FileSize;
    s32 BlockSize;
    s32 Pad2;
    s64 Blocks;
    s64 AccessSec;
    u64 AccessNanosec;
    s64 ModificationSec;
    u64 ModificationNanosec;
    s64 StatusSec;
    u64 StatusNanosec;
    u32 Unused4;
    u32 Unused5;
};

// ==================================================================
// NOTE(vak): MMap
// ==================================================================

#define PROT_NONE   (0x0)
#define PROT_READ   (0x1)
#define PROT_WRITE  (0x2)
#define PROT_EXEC   (0x4)

#define MAP_PRIVATE (0x02)
#define MAP_ANON    (0x20)

// ==================================================================
// NOTE(vak): Socket
// ==================================================================

#define AF_UNIX     (1)
#define SOCK_STREAM (1)

struct sockaddr_un
{
    u16     sun_family;
    char    sun_path[108];    
};

struct sockaddr;

// ==================================================================
// NOTE(vak): Syscall wrappers implemented by this file
// ==================================================================

local ssize read        (s32 FileDescriptor, void* Data, usize Size);
local ssize write       (s32 FileDescriptor, void* Data, usize Size);
local s32   open        (const char* Path, s32 Flags, s32 Mode);
local s32   close       (s32 FileDescriptor);
local s32   fstat       (s32 FileDescriptor, struct stat* Buffer);
local void* mmap        (void* Base, usize Length, int ProtectionFlags, int Flags, int FileDescriptor, ssize Offset);
local s32   mprotect    (void* Base, usize Length, int ProtectionFlags);
local s32   socket      (s32 Domain, s32 Type, s32 Protocol);
local s32   connect     (s32 SocketFD, const struct sockaddr* Address, u32 AddressLength);
local ssize send        (s32 SocketFD, const void* Buffer, usize Size, s32 Flags);
local ssize recv        (s32 SocketFD, const void* Buffer, usize Size, s32 Flags);
local void  exit_group  (s32 Status);

// ==================================================================
// NOTE(vak): Syscall
// ==================================================================

typedef enum
{
#if ArchitectureX64
    SyscallNR_Read          = (0),
    SyscallNR_Write         = (1),
    SyscallNR_Open          = (2),
    SyscallNR_Close         = (3),
    SyscallNR_FStat         = (5),
    SyscallNR_MMap          = (9),
    SyscallNR_MProtect      = (10),
    SyscallNR_Socket        = (41),
    SyscallNR_Connect       = (42),
    SyscallNR_SendTo        = (44),
    SyscallNR_RecvFrom      = (45),
    SyscallNR_ExitGroup     = (231),
#else
    #error Linux syscall numbers are not defined for this architecture.
#endif
} syscall_nr;

local usize LinuxSyscall(
    syscall_nr NR,
    usize Arg0, usize Arg1, usize Arg2,
    usize Arg3, usize Arg4, usize Arg5
)
{
    usize Result = 0;

#if ArchitectureX64
    register usize R10 __asm__("r10") = Arg3;
    register usize R8  __asm__("r8")  = Arg4;
    register usize R9  __asm__("r9")  = Arg5;

    __asm__ volatile (
        "syscall" :
        "=a"(Result) :
        "a"(NR),
        "D"(Arg0),
        "S"(Arg1),
        "d"(Arg2),
        "r"(R10),
        "r"(R8),
        "r"(R9) :
        "memory", "rcx", "r11"
    );
#else
    #error Linux syscall is not implemented for this architecture.
#endif

    return (Result);
}

#define LinuxSyscall0(NR)                           LinuxSyscall(NR, 0, 0, 0, 0, 0, 0)
#define LinuxSyscall1(NR, A0)                       LinuxSyscall(NR, (usize)(A0), 0, 0, 0, 0, 0)
#define LinuxSyscall2(NR, A0, A1)                   LinuxSyscall(NR, (usize)(A0), (usize)(A1), 0, 0, 0, 0)
#define LinuxSyscall3(NR, A0, A1, A2)               LinuxSyscall(NR, (usize)(A0), (usize)(A1), (usize)(A2), 0, 0, 0)
#define LinuxSyscall4(NR, A0, A1, A2, A3)           LinuxSyscall(NR, (usize)(A0), (usize)(A1), (usize)(A2), (usize)(A3), 0, 0)
#define LinuxSyscall5(NR, A0, A1, A2, A3, A4)       LinuxSyscall(NR, (usize)(A0), (usize)(A1), (usize)(A2), (usize)(A3), (usize)(A4), 0)
#define LinuxSyscall6(NR, A0, A1, A2, A3, A4, A5)   LinuxSyscall(NR, (usize)(A0), (usize)(A1), (usize)(A2), (usize)(A3), (usize)(A4), (usize)(A5))

// ==================================================================
// NOTE(vak): Syscall wrapper implementations
// ==================================================================

local s32 socket(s32 Domain, s32 Type, s32 Protocol)
{
    s32 Result = (s32)LinuxSyscall3(SyscallNR_Socket, Domain, Type, Protocol);
    return (Result);
}

local s32 connect(s32 SocketFD, const struct sockaddr* Address, u32 AddressLength)
{
    s32 Result = (s32)LinuxSyscall3(SyscallNR_Connect, SocketFD, Address, AddressLength);
    return (Result);
}

local ssize send(s32 SocketFD, const void* Buffer, usize Size, s32 Flags)
{
    ssize Result = (ssize)LinuxSyscall6(
        SyscallNR_SendTo,
        SocketFD, Buffer, Size, Flags,
        0, 0
    );

    return (Result);
}

local ssize recv(s32 SocketFD, const void* Buffer, usize Size, s32 Flags)
{
    ssize Result = (ssize)LinuxSyscall6(
        SyscallNR_RecvFrom,
        SocketFD, Buffer, Size, Flags,
        0, 0
    );

    return (Result);
}

local ssize read(s32 FileDescriptor, void* Data, usize Size)
{
    ssize Result = (ssize)LinuxSyscall3(SyscallNR_Read, FileDescriptor, Data, Size);
    return (Result);
}

local ssize write(s32 FileDescriptor, void* Data, usize Size)
{
    ssize Result = (ssize)LinuxSyscall3(SyscallNR_Write, FileDescriptor, Data, Size);
    return (Result);
}

local s32 open(const char* Path, int Flags, int Mode)
{
    s32 Result = (s32)LinuxSyscall3(SyscallNR_Open, Path, Flags, Mode);
    return (Result);
}

local s32 close(s32 FileDescriptor)
{
    s32 Result = (s32)LinuxSyscall1(SyscallNR_Close, FileDescriptor);
    return (Result);
}

local s32 fstat(s32 FileDescriptor, struct stat* Buffer)
{
    s32 Result = (s32)LinuxSyscall2(
        SyscallNR_FStat,
        FileDescriptor,
        Buffer
    );

    return (Result);
}

local void* mmap(void* Base, usize Length, int ProtectionFlags, int Flags, int FileDescriptor, ssize Offset)
{
    void* Result = (void*)LinuxSyscall6(
        SyscallNR_MMap,
        Base,
        Length,
        ProtectionFlags,
        Flags,
        FileDescriptor,
        Offset
    );

    return (Result);
}

local s32 mprotect(void* Base, usize Length, int ProtectionFlags)
{
    s32 Result = (s32)LinuxSyscall3(
        SyscallNR_MProtect,
        Base,
        Length,
        ProtectionFlags
    );

    return (Result);
}

local void exit_group(s32 Status)
{
    LinuxSyscall1(SyscallNR_ExitGroup, Status);
}

