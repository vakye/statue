
void LinuxEntry(int ArgCount, char* Args[], char* Envp[])
{
    (void) ArgCount;
    (void) Args;
    (void) Envp;

    __asm__ volatile (
        "mov $231, %eax\n"
        "mov $127, %rdi\n"
        "syscall"
    );
}

__attribute__((naked))
void EntryPoint(void)
{
    __asm__ volatile (
        "mov 0(%rsp),           %edi\n"   // NOTE(vak): ArgCount
        "mov 8(%rsp),           %rsi\n"   // NOTE(vak): Args
        "mov 8(%rsp, %rdi, 8),  %rdx\n"   // NOTE(vak): Envp
        "call LinuxEntry\n"
    );
}

