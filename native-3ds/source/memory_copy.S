/* Bounded native ARM11 CPU-memory copy. Same alignment discipline as pinned
 * Melee byte_memory.h/libc.c; no endian conversion or device access. */
.syntax unified
.arm
.section .text.fh_memory_copy,"ax",%progbits
.global fh_memory_copy
.type fh_memory_copy,%function
fh_memory_copy:
    cmp r2,#0
    bxeq lr
    push {r4-r10,lr}
    mov r12,r0
    eor r3,r0,r1
    tst r3,#3
    bne 4f
1:  tst r12,#3
    beq 2f
    ldrb r3,[r1],#1
    strb r3,[r12],#1
    subs r2,r2,#1
    beq 6f
    b 1b
2:  cmp r2,#32
    blo 3f
    ldmia r1!,{r3-r10}
    stmia r12!,{r3-r10}
    sub r2,r2,#32
    b 2b
3:  cmp r2,#4
    blo 4f
    ldr r3,[r1],#4
    str r3,[r12],#4
    sub r2,r2,#4
    b 3b
4:  cmp r2,#0
    beq 6f
5:  ldrb r3,[r1],#1
    strb r3,[r12],#1
    subs r2,r2,#1
    bne 5b
6:  pop {r4-r10,pc}
.size fh_memory_copy,.-fh_memory_copy
/* Optional native link adaptation: byte-preserving memcpy ABI, including
 * arbitrary alignment and zero length. GPU/DSP cache operations stay at
 * their existing call sites. No device-memory or overlap contract is added. */
.global __wrap_memcpy
.type __wrap_memcpy,%function
.set __wrap_memcpy,fh_memory_copy

.section .text.fh_memory_zero,"ax",%progbits
.global fh_memory_zero
.type fh_memory_zero,%function
fh_memory_zero:
    cmp r1,#0
    bxeq lr
    push {r4-r9}
    mov r2,#0
1:  tst r0,#3
    beq 2f
    strb r2,[r0],#1
    subs r1,r1,#1
    beq 6f
    b 1b
2:  cmp r1,#32
    blo 4f
    mov r3,#0
    mov r4,#0
    mov r5,#0
    mov r6,#0
    mov r7,#0
    mov r8,#0
    mov r9,#0
3:  stmia r0!,{r2-r9}
    sub r1,r1,#32
    cmp r1,#32
    bhs 3b
4:  cmp r1,#4
    blo 5f
    str r2,[r0],#4
    sub r1,r1,#4
    b 4b
5:  cmp r1,#0
    beq 6f
    strb r2,[r0],#1
    sub r1,r1,#1
    b 5b
6:  pop {r4-r9}
    bx lr
.size fh_memory_zero,.-fh_memory_zero
