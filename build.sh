#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ -z "${ARM_CC:-}" ]; then
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        ARM_CC=$(command -v arm-none-eabi-gcc)
    elif [ -x .tools/usr/bin/arm-none-eabi-gcc ]; then
        ARM_CC="$PWD/.tools/usr/bin/arm-none-eabi-gcc"
    else
        ARM_CC=/tools/Xilinx/Vitis/2021.1/gnu/aarch32/lin/gcc-arm-none-eabi/bin/arm-none-eabi-gcc
    fi
fi
"$ARM_CC" -march=armv6 -marm -mfloat-abi=soft -Os -Wall -Wextra -Werror \
    -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -static \
    -Wl,-e,_start -Wl,-Ttext=0x10000 \
    -o package/extensions/weather-clock/weather-clock src/start.S src/app.c -lgcc
