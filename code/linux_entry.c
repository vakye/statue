
#include "shared.c"
#include "platform.c"
#include "main.c"

// ==================================================================
// NOTE(vak): Internals
// ==================================================================

#include "linux_syscall.c"
#include "linux_platform.c"

// ==================================================================
// NOTE(vak): Entry point
// ==================================================================

void LinuxEntry(int ArgCount, char* Args[], char* Envp[])
{
    Unused(ArgCount);
    Unused(Args);
    Unused(Envp);

    Main();
    Exit(0);
}

__attribute__((naked))
void EntryPoint(void)
{
#if ArchitectureX64
    __asm__ volatile (
        "mov 0(%rsp),           %edi\n"   // NOTE(vak): ArgCount
        "mov 8(%rsp),           %rsi\n"   // NOTE(vak): Args
        "mov 8(%rsp, %rdi, 8),  %rdx\n"   // NOTE(vak): Envp
        "call LinuxEntry\n"
    );
#else
    #error Linux entry point is not implemented for this architecture
#endif
}

// ==================================================================
// NOTE(vak): Stupid CRT stuff
// ==================================================================

void* memset(void* DestInit, s32 Byte, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = Byte;

    return (DestInit);
}

void* memcpy(void* DestInit, const void* SourceInit, usize Size)
{
    u8* Dest = (u8*)DestInit;
    u8* Source = (u8*)SourceInit;

    while (Size--)
        *Dest++ = *Source++;

    return (DestInit);
}

