; Test encoding of MIPS32 floating point arithmetic instructions

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols --skip-unimplemented \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols --skip-unimplemented \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --args -O2 \
; RUN:   --allow-externally-defined-symbols --skip-unimplemented \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=mips32 --assemble --disassemble \
; RUN:   --args -O2 --allow-externally-defined-symbols --skip-unimplemented \
; RUN:   | FileCheck %s --check-prefix=DIS

declare float @llvm.fabs.f32(float)
declare double @llvm.fabs.f64(double)
declare float @llvm.sqrt.f32(float)
declare double @llvm.sqrt.f64(double)

define internal float @encAbsFloat(float %a) {
entry:
  %c = call float @llvm.fabs.f32(float %a)
  ret float %c
}

; ASM-LABEL: encAbsFloat
; ASM-NEXT: .LencAbsFloat$entry:
; ASM-NEXT: 	abs.s	$f12, $f12
; ASM-NEXT: 	mov.s	$f0, $f12
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000000 <encAbsFloat>:
; DIS-NEXT:     0:	46006305 	abs.s	$f12,$f12
; DIS-NEXT:     4:	46006006 	mov.s	$f0,$f12
; DIS-NEXT:     8:	03e00008 	jr	ra
; DIS-NEXT:     c:	00000000 	nop

; IASM-LABEL: encAbsFloat:
; IASM-NEXT: .LencAbsFloat$entry:
; IASM-NEXT: 	.byte 0x5
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal double @encAbsDouble(double %a) {
entry:
  %c = call double @llvm.fabs.f64(double %a)
  ret double %c
}

; ASM-LABEL: encAbsDouble:
; ASM-NEXT: .LencAbsDouble$entry:
; ASM-NEXT: 	abs.d	$f12, $f12
; ASM-NEXT: 	mov.d	$f0, $f12
; ASM-NEXT: 	jr	$ra

; DIS-LABEL: 00000010 <encAbsDouble>:
; DIS-NEXT:     10:	46206305 	abs.d	$f12,$f12
; DIS-NEXT:     14:	46206006 	mov.d	$f0,$f12
; DIS-NEXT:     18:	03e00008 	jr	ra
; DIS-NEXT:     1c:	00000000 	nop

; IASM-LABEL: encAbsDouble:
; IASM-NEXT: .LencAbsDouble$entry:
; IASM-NEXT: 	.byte 0x5
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal float @encAddFloat(float %a, float %b) {
entry:
  %c = fadd float %a, %b
  ret float %c
}

; ASM-LABEL: encAddFloat
; ASM-NEXT: .LencAddFloat$entry:
; ASM-NEXT:	add.s	$f12, $f12, $f14
; ASM-NEXT:	mov.s	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000020 <encAddFloat>:
; DIS-NEXT:    20:	460e6300 	add.s	$f12,$f12,$f14
; DIS-NEXT:    24:	46006006 	mov.s	$f0,$f12
; DIS-NEXT:    28:	03e00008 	jr	ra
; DIS-NEXT:    2c:	00000000 	nop

; IASM-LABEL: encAddFloat:
; IASM-NEXT: .LencAddFloat$entry:
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal double @encAddDouble(double %a, double %b) {
entry:
  %c = fadd double %a, %b
  ret double %c
}

; ASM-LABEL: encAddDouble
; ASM-NEXT: .LencAddDouble$entry:
; ASM-NEXT:	add.d	$f12, $f12, $f14
; ASM-NEXT:	mov.d	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000030 <encAddDouble>:
; DIS-NEXT:    30:	462e6300 	add.d	$f12,$f12,$f14
; DIS-NEXT:    34:	46206006 	mov.d	$f0,$f12
; DIS-NEXT:    38:	03e00008 	jr	ra
; DIS-NEXT:    3c:	00000000 	nop

; IASM-LABEL: encAddDouble:
; IASM-NEXT: .LencAddDouble$entry:
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal float @encDivFloat(float %a, float %b) {
entry:
  %c = fdiv float %a, %b
  ret float %c
}

; ASM-LABEL: encDivFloat
; ASM-NEXT: .LencDivFloat$entry:
; ASM-NEXT:	div.s	$f12, $f12, $f14
; ASM-NEXT:	mov.s	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000040 <encDivFloat>:
; DIS-NEXT:    40:	460e6303 	div.s	$f12,$f12,$f14
; DIS-NEXT:    44:	46006006 	mov.s	$f0,$f12
; DIS-NEXT:    48:	03e00008 	jr	ra
; DIS-NEXT:    4c:	00000000 	nop

; IASM-LABEL: encDivFloat:
; IASM-NEXT: .LencDivFloat$entry:
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal double @encDivDouble(double %a, double %b) {
entry:
  %c = fdiv double %a, %b
  ret double %c
}

; ASM-LABEL: encDivDouble
; ASM-NEXT: .LencDivDouble$entry:
; ASM-NEXT:	div.d	$f12, $f12, $f14
; ASM-NEXT:	mov.d	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000050 <encDivDouble>:
; DIS-NEXT:    50:	462e6303 	div.d	$f12,$f12,$f14
; DIS-NEXT:    54:	46206006 	mov.d	$f0,$f12
; DIS-NEXT:    58:	03e00008 	jr	ra
; DIS-NEXT:    5c:	00000000 	nop

; IASM-LABEL: encDivDouble:
; IASM-NEXT: .LencDivDouble$entry:
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal float @encMulFloat(float %a, float %b) {
entry:
  %c = fmul float %a, %b
  ret float %c
}

; ASM-LABEL: encMulFloat
; ASM-NEXT: .LencMulFloat$entry:
; ASM-NEXT:	mul.s	$f12, $f12, $f14
; ASM-NEXT:	mov.s	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000060 <encMulFloat>:
; DIS-NEXT:    60:	460e6302 	mul.s	$f12,$f12,$f14
; DIS-NEXT:    64:	46006006 	mov.s	$f0,$f12
; DIS-NEXT:    68:	03e00008 	jr	ra
; DIS-NEXT:    6c:	00000000 	nop

; IASM-LABEL: encMulFloat:
; IASM-NEXT: .LencMulFloat$entry:
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal double @encMulDouble(double %a, double %b) {
entry:
  %c = fmul double %a, %b
  ret double %c
}

; ASM-LABEL: encMulDouble
; ASM-NEXT: .LencMulDouble$entry:
; ASM-NEXT:	mul.d	$f12, $f12, $f14
; ASM-NEXT:	mov.d	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000070 <encMulDouble>:
; DIS-NEXT:    70:	462e6302 	mul.d	$f12,$f12,$f14
; DIS-NEXT:    74:	46206006 	mov.d	$f0,$f12
; DIS-NEXT:    78:	03e00008 	jr	ra
; DIS-NEXT:    7c:	00000000 	nop

; IASM-LABEL: encMulDouble:
; IASM-NEXT: .LencMulDouble$entry:
; IASM-NEXT: 	.byte 0x2
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal float @encSqrtFloat(float %a) {
entry:
  %c = call float @llvm.sqrt.f32(float %a)
  ret float %c
}

; ASM-LABEL: encSqrtFloat
; ASM-NEXT: .LencSqrtFloat$entry:
; ASM-NEXT:	sqrt.s	$f12, $f12
; ASM-NEXT:	mov.s	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000080 <encSqrtFloat>:
; DIS-NEXT:    80:	46006304 	sqrt.s	$f12,$f12
; DIS-NEXT:    84:	46006006 	mov.s	$f0,$f12
; DIS-NEXT:    88:	03e00008 	jr	ra
; DIS-NEXT:    8c:	00000000 	nop

; IASM-LABEL: encSqrtFloat:
; IASM-NEXT: .LencSqrtFloat$entry:
; IASM-NEXT: 	.byte 0x4
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal double @encSqrtDouble(double %a) {
entry:
  %c = call double @llvm.sqrt.f64(double %a)
  ret double %c
}

; ASM-LABEL: encSqrtDouble
; ASM-NEXT: .LencSqrtDouble$entry:
; ASM-NEXT:	sqrt.d	$f12, $f12
; ASM-NEXT:	mov.d	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 00000090 <encSqrtDouble>:
; DIS-NEXT:    90:	46206304 	sqrt.d	$f12,$f12
; DIS-NEXT:    94:	46206006 	mov.d	$f0,$f12
; DIS-NEXT:    98:	03e00008 	jr	ra
; DIS-NEXT:    9c:	00000000 	nop

; IASM-LABEL: encSqrtDouble:
; IASM-NEXT: .LencSqrtDouble$entry:
; IASM-NEXT: 	.byte 0x4
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal float @encSubFloat(float %a, float %b) {
entry:
  %c = fsub float %a, %b
  ret float %c
}

; ASM-LABEL: encSubFloat
; ASM-NEXT: .LencSubFloat$entry:
; ASM-NEXT:	sub.s	$f12, $f12, $f14
; ASM-NEXT:	mov.s	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 000000a0 <encSubFloat>:
; DIS-NEXT:    a0:	460e6301 	sub.s	$f12,$f12,$f14
; DIS-NEXT:    a4:	46006006 	mov.s	$f0,$f12
; DIS-NEXT:    a8:	03e00008 	jr	ra
; DIS-NEXT:    ac:	00000000 	nop

; IASM-LABEL: encSubFloat:
; IASM-NEXT: .LencSubFloat$entry:
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0xe
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0

define internal double @encSubDouble(double %a, double %b) {
entry:
  %c = fsub double %a, %b
  ret double %c
}

; ASM-LABEL: encSubDouble
; ASM-NEXT: .LencSubDouble$entry:
; ASM-NEXT:	sub.d	$f12, $f12, $f14
; ASM-NEXT:	mov.d	$f0, $f12
; ASM-NEXT:	jr	$ra

; DIS-LABEL: 000000b0 <encSubDouble>:
; DIS-NEXT:    b0:	462e6301 	sub.d	$f12,$f12,$f14
; DIS-NEXT:    b4:	46206006 	mov.d	$f0,$f12
; DIS-NEXT:    b8:	03e00008 	jr	ra
; DIS-NEXT:    bc:	00000000 	nop

; IASM-LABEL: encSubDouble:
; IASM-NEXT: .LencSubDouble$entry:
; IASM-NEXT: 	.byte 0x1
; IASM-NEXT: 	.byte 0x63
; IASM-NEXT: 	.byte 0x2e
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x6
; IASM-NEXT: 	.byte 0x60
; IASM-NEXT: 	.byte 0x20
; IASM-NEXT: 	.byte 0x46
; IASM-NEXT: 	.byte 0x8
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0xe0
; IASM-NEXT: 	.byte 0x3
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0
; IASM-NEXT: 	.byte 0x0