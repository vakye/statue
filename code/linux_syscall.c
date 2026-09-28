
// ==================================================================
// NOTE(vak): Implements syscall wrappers for Linux
// ==================================================================

#pragma once

typedef enum
{
#if ArchitectureX64
    SyscallNR_Write         = (1),
    SyscallNR_ExitGroup     = (231),
#else
    #error Linux syscall numbers are not defined for this architecture.
#endif
} syscall_nr;

static usize LinuxSyscall(
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

static ssize write(s32 FileDescriptor, void* Data, usize Size)
{
    ssize Result = (ssize)LinuxSyscall3(SyscallNR_Write, FileDescriptor, Data, Size);
    return (Result);
}

static void exit_group(s32 Status)
{
    LinuxSyscall1(SyscallNR_ExitGroup, Status);
}

