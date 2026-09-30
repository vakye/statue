#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

pushd code/shaders > /dev/null

for SourceFile in *.vert *.frag; do
    OutputFile="$SourceFile.h"

    glslangValidator --target-env vulkan1.4 -x $SourceFile -o $OutputFile
done

popd > /dev/null

SourceFile="code/linux_entry.c"
OutputFile="build/statue"

Compiler="clang"

CompileFlags=" \
    -g \
    -O0 \
    -ffreestanding \
    -fno-stack-protector \
    -std=c11 \
    -nostdlib \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-function \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,-e,EntryPoint \
    -lc \
    -lwayland-client"

$Compiler $CompileFlags $SourceFile $LinkFlags

if [ $? -eq 0 ]; then
    echo $(basename $SourceFile)
fi

