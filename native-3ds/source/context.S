.syntax unified
.arm
.fpu vfp
.section .text.fh_context_swap,"ax",%progbits
.global fh_context_swap
.type fh_context_swap,%function
fh_context_swap:
    stmia r0!,{r4-r11,sp,lr}
    vstmia r0,{d8-d15}
    ldmia r1!,{r4-r11,sp,lr}
    vldmia r1,{d8-d15}
    bx lr
.size fh_context_swap,.-fh_context_swap
.section .text.fh_thread_trampoline,"ax",%progbits
.global fh_thread_trampoline
.type fh_thread_trampoline,%function
fh_thread_trampoline:
    mov r0,r4
    bl fh_thread_start
    b .
.size fh_thread_trampoline,.-fh_thread_trampoline
.section .note.GNU-stack,"",%progbits
