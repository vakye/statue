#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

SourceFile="code/linux_platform.c"
OutputFile="build/statue"

Compiler="clang"

CompileFlags=" \
    -g \
    -O0 \
    -ffreestanding \
    -fno-stack-protector \
    -nostdlib \
    -std=c11 \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-function \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,--entry,EntryPoint"

$Compiler $CompileFlags $SourceFile $LinkFlags

