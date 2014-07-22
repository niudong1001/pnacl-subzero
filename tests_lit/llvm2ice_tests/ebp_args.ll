; This test originally exhibited a bug in ebp-based stack slots.  The
; problem was that during a function call push sequence, the esp
; adjustment was incorrectly added to the stack/frame offset for
; ebp-based frames.

; RUN: %llvm2ice -Om1 --target=x8632 --verbose none %s | FileCheck %s

declare i32 @memcpy_helper2(i32 %buf, i32 %buf2, i32 %n);

define i32 @memcpy_helper(i32 %buf, i32 %n) {
entry:
  %n.arg_trunc = trunc i32 %n to i8
  %buf2 = alloca i8, i32 128, align 4
  %buf2.asint = ptrtoint i8* %buf2 to i32
  %arg_ext = zext i8 %n.arg_trunc to i32
  %call = call i32 @memcpy_helper2(i32 %buf, i32 %buf2.asint, i32 %arg_ext)
  ret i32 %call
}

; This check sequence is highly specific to the current Om1 lowering
; and stack slot assignment code, and may need to be relaxed if the
; lowering code changes.

; CHECK: memcpy_helper:
; CHECK: push     ebp
; CHECK: mov      ebp, esp
; CHECK: sub      esp, 20
; CHECK: mov      eax, dword ptr [ebp+12]
; CHECK: mov      dword ptr [ebp-4], eax
; CHECK: sub      esp, 128
; CHECK: mov      dword ptr [ebp-8], esp
; CHECK: mov      eax, dword ptr [ebp-8]
; CHECK: mov      dword ptr [ebp-12], eax
; CHECK: movzx    eax, byte ptr [ebp-4]
; CHECK: mov      dword ptr [ebp-16], eax
; CHECK: push     dword ptr [ebp-16]
; CHECK: push     dword ptr [ebp-12]
; CHECK: push     dword ptr [ebp+8]
; CHECK: call     memcpy_helper2