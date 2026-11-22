.syntax unified
.cpu arm7tdmi
.arm
.section .text.__aeabi_uidivmod, "ax", %progbits
.align 2
.global __aeabi_uidivmod
.type __aeabi_uidivmod,%function
__aeabi_uidivmod:
    stmdb sp!, {r4-r6, lr}
    mov r4, r0
    mov r5, r1
    bl __aeabi_uidiv
    mov r6, r0
    mul r1, r6, r5
    sub r1, r4, r1
    mov r0, r6
    ldmia sp!, {r4-r6, lr}
    bx lr
.section .text.__aeabi_idivmod, "ax", %progbits
.align 2
.global __aeabi_idivmod
.type __aeabi_idivmod,%function
__aeabi_idivmod:
    stmdb sp!, {r4-r6, lr}
    mov r4, r0
    mov r5, r1
    bl __aeabi_idiv
    mov r6, r0
    mul r1, r6, r5
    sub r1, r4, r1
    mov r0, r6
    ldmia sp!, {r4-r6, lr}
    bx lr
