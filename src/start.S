.syntax unified
.arm
.global _start
.type _start, %function
_start:
    ldr r0, [sp]
    add r1, sp, #4
    bl appmain
    mov r7, #1
    svc #0
.section .note.GNU-stack,"",%progbits
