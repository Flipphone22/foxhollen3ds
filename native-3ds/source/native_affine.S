/* ARM11 scalar VFP affine composition. No FPSCR vector mode or reassociation.
 * Hold all of B before writing: A==out, B==out and A==B==out remain valid.
 * Keep the source's explicit +0 and translation addition order. */
.syntax unified
.arm
.fpu vfp
.section .text.C_MTXConcat,"ax",%progbits
.align 2
.global C_MTXConcat
.type C_MTXConcat,%function
C_MTXConcat:
    vpush {d8-d11}
    vldmia r1, {s0-s11}
    mov r3, #0
    vmov s22, r3
    mov r3, #3
.Lrow:
    vldmia r0!, {s12-s15}
    vmul.f32 s16, s12, s0
    vmul.f32 s17, s13, s4
    vadd.f32 s16, s16, s17
    vmul.f32 s17, s14, s8
    vadd.f32 s17, s22, s17
    vadd.f32 s18, s17, s16

    vmul.f32 s16, s12, s1
    vmul.f32 s17, s13, s5
    vadd.f32 s16, s16, s17
    vmul.f32 s17, s14, s9
    vadd.f32 s17, s22, s17
    vadd.f32 s19, s17, s16

    vmul.f32 s16, s12, s2
    vmul.f32 s17, s13, s6
    vadd.f32 s16, s16, s17
    vmul.f32 s17, s14, s10
    vadd.f32 s17, s22, s17
    vadd.f32 s20, s17, s16

    vmul.f32 s16, s12, s3
    vmul.f32 s17, s13, s7
    vadd.f32 s16, s16, s17
    vmul.f32 s17, s14, s11
    vadd.f32 s17, s17, s16
    vadd.f32 s21, s15, s17
    vstmia r2!, {s18-s21}
    subs r3, r3, #1
    bne .Lrow
    vpop {d8-d11}
    bx lr
.size C_MTXConcat,.-C_MTXConcat
.section .note.GNU-stack,"",%progbits
