/* Adapted from pinned melee-3ds port/engine/libc.c.
 * See native-3ds/vendor/melee/PROVENANCE.md. Native little-endian byte ordering.
 * Bounded reads, no speculative overread; not for device memory. */
.syntax unified
.arm
.section .text.fh_memory_compare,"ax",%progbits
.global fh_memory_compare
.type fh_memory_compare,%function
fh_memory_compare:
push {r4-r10,lr}
eor r3,r0,r1
tst r3,#3
bne 4f
1: cmp r2,#0
beq 6f
tst r0,#3
beq 2f
ldrb r3,[r0],#1
ldrb r4,[r1],#1
subs r2,r2,#1
cmp r3,r4
bne 5f
b 1b
2: cmp r2,#16
blo 3f
ldmia r0!,{r3-r6}
ldmia r1!,{r7-r10}
cmp r3,r7
cmpeq r4,r8
cmpeq r5,r9
cmpeq r6,r10
bne 7f
sub r2,r2,#16
b 2b
3: cmp r2,#4
blo 4f
ldr r3,[r0],#4
ldr r4,[r1],#4
cmp r3,r4
bne 8f
sub r2,r2,#4
b 3b
4: cmp r2,#0
beq 6f
ldrb r3,[r0],#1
ldrb r4,[r1],#1
sub r2,r2,#1
cmp r3,r4
beq 4b
5: sub r0,r3,r4
pop {r4-r10,pc}
6: mov r0,#0
pop {r4-r10,pc}
7: sub r0,r0,#16
sub r1,r1,#16
b 4b
8: sub r0,r0,#4
sub r1,r1,#4
b 4b
.size fh_memory_compare,.-fh_memory_compare
