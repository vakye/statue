
// ==================================================================
// NOTE(vak): Implements syscall wrappers for Linux
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Standard file descriptors numbers
// ==================================================================

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

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

local s32   socket      (s32 Domain, s32 Type, s32 Protocol);
local s32   connect     (s32 SocketFD, const struct sockaddr* Address, u32 AddressLength);
local ssize send        (s32 SocketFD, const void* Buffer, usize Size, s32 Flags);
local ssize recv        (s32 SocketFD, const void* Buffer, usize Size, s32 Flags);
local ssize write       (s32 FileDescriptor, void* Data, usize Size);
local void  exit_group  (s32 Status);

// ==================================================================
// NOTE(vak): Syscall
// ==================================================================

typedef enum
{
#if ArchitectureX64
    SyscallNR_Write         = (1),
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

local ssize write(s32 FileDescriptor, void* Data, usize Size)
{
    ssize Result = (ssize)LinuxSyscall3(SyscallNR_Write, FileDescriptor, Data, Size);
    return (Result);
}

local void exit_group(s32 Status)
{
    LinuxSyscall1(SyscallNR_ExitGroup, Status);
}

