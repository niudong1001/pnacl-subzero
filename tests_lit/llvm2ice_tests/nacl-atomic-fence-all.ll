; Test that loads/stores don't move across a nacl.atomic.fence.all.
; This should apply to both atomic and non-atomic loads/stores
; (unlike the non-"all" variety of nacl.atomic.fence, which only
; applies to atomic load/stores).
;
; RUN: %llvm2ice -O2 --verbose none %s | FileCheck %s
; RUN: %llvm2ice -O2 --verbose none %s | llvm-mc -x86-asm-syntax=intel

declare void @llvm.nacl.atomic.fence.all()
declare i32 @llvm.nacl.atomic.load.i32(i32*, i32)
declare void @llvm.nacl.atomic.store.i32(i32, i32*, i32)

@g32_a = internal global [4 x i8] zeroinitializer, align 4
@g32_b = internal global [4 x i8] zeroinitializer, align 4
@g32_c = internal global [4 x i8] zeroinitializer, align 4
@g32_d = internal global [4 x i8] c"\02\00\00\00", align 4

define i32 @test_fused_load_add_a() {
entry:
  %p_alloca = alloca i8, i32 4, align 4
  %p_alloca_bc = bitcast i8* %p_alloca to i32*
  store i32 999, i32* %p_alloca_bc, align 1

  %p_a = bitcast [4 x i8]* @g32_a to i32*
  %l_a = call i32 @llvm.nacl.atomic.load.i32(i32* %p_a, i32 6)
  %l_a2 = add i32 %l_a, 1
  call void @llvm.nacl.atomic.store.i32(i32 %l_a2, i32* %p_a, i32 6)

  %p_b = bitcast [4 x i8]* @g32_b to i32*
  %l_b = load i32* %p_b
  %l_b2 = add i32 %l_b, 1
  store i32 %l_b2, i32* %p_b, align 1

  %p_c = bitcast [4 x i8]* @g32_c to i32*
  %l_c = load i32* %p_c
  %l_c2 = add i32 %l_c, 1
  call void @llvm.nacl.atomic.fence.all()
  store i32 %l_c2, i32* %p_c, align 1

  ret i32 %l_c2
}
; CHECK-LABEL: test_fused_load_add_a
;    alloca store
; CHECK: mov {{.*}}, esp
; CHECK: mov dword ptr {{.*}}, 999
;    atomic store (w/ its own mfence)
; CHECK: lea {{.*}}, g32_a
; The load + add are optimized into one everywhere.
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr
; CHECK: mfence
; CHECK: lea {{.*}}, g32_b
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr
; CHECK: lea {{.*}}, g32_c
; CHECK: add {{.*}}, dword ptr
; CHECK: mfence
; CHECK: mov dword ptr

; Test with the fence moved up a bit.
define i32 @test_fused_load_add_b() {
entry:
  %p_alloca = alloca i8, i32 4, align 4
  %p_alloca_bc = bitcast i8* %p_alloca to i32*
  store i32 999, i32* %p_alloca_bc, align 1

  %p_a = bitcast [4 x i8]* @g32_a to i32*
  %l_a = call i32 @llvm.nacl.atomic.load.i32(i32* %p_a, i32 6)
  %l_a2 = add i32 %l_a, 1
  call void @llvm.nacl.atomic.store.i32(i32 %l_a2, i32* %p_a, i32 6)

  %p_b = bitcast [4 x i8]* @g32_b to i32*
  %l_b = load i32* %p_b
  %l_b2 = add i32 %l_b, 1
  store i32 %l_b2, i32* %p_b, align 1

  %p_c = bitcast [4 x i8]* @g32_c to i32*
  call void @llvm.nacl.atomic.fence.all()
  %l_c = load i32* %p_c
  %l_c2 = add i32 %l_c, 1
  store i32 %l_c2, i32* %p_c, align 1

  ret i32 %l_c2
}
; CHECK-LABEL: test_fused_load_add_b
;    alloca store
; CHECK: mov {{.*}}, esp
; CHECK: mov dword ptr {{.*}}, 999
;    atomic store (w/ its own mfence)
; CHECK: lea {{.*}}, g32_a
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr
; CHECK: mfence
; CHECK: lea {{.*}}, g32_b
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr
; CHECK: lea {{.*}}, g32_c
; CHECK: mfence
; Load + add can still be optimized into one instruction
; because it is not separated by a fence.
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr

; Test with the fence splitting a load/add.
define i32 @test_fused_load_add_c() {
entry:
  %p_alloca = alloca i8, i32 4, align 4
  %p_alloca_bc = bitcast i8* %p_alloca to i32*
  store i32 999, i32* %p_alloca_bc, align 1

  %p_a = bitcast [4 x i8]* @g32_a to i32*
  %l_a = call i32 @llvm.nacl.atomic.load.i32(i32* %p_a, i32 6)
  %l_a2 = add i32 %l_a, 1
  call void @llvm.nacl.atomic.store.i32(i32 %l_a2, i32* %p_a, i32 6)

  %p_b = bitcast [4 x i8]* @g32_b to i32*
  %l_b = load i32* %p_b
  call void @llvm.nacl.atomic.fence.all()
  %l_b2 = add i32 %l_b, 1
  store i32 %l_b2, i32* %p_b, align 1

  %p_c = bitcast [4 x i8]* @g32_c to i32*
  %l_c = load i32* %p_c
  %l_c2 = add i32 %l_c, 1
  store i32 %l_c2, i32* %p_c, align 1

  ret i32 %l_c2
}
; CHECK-LABEL: test_fused_load_add_c
;    alloca store
; CHECK: mov {{.*}}, esp
; CHECK: mov dword ptr {{.*}}, 999
;    atomic store (w/ its own mfence)
; CHECK: lea {{.*}}, g32_a
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr
; CHECK: mfence
; CHECK: lea {{.*}}, g32_b
; This load + add are no longer optimized into one,
; though perhaps it should be legal as long as
; the load stays on the same side of the fence.
; CHECK: mov {{.*}}, dword ptr
; CHECK: mfence
; CHECK: add {{.*}}, 1
; CHECK: mov dword ptr
; CHECK: lea {{.*}}, g32_c
; CHECK: add {{.*}}, dword ptr
; CHECK: mov dword ptr


; Test where a bunch of i8 loads could have been fused into one
; i32 load, but a fence blocks that.
define i32 @could_have_fused_loads() {
entry:
  %ptr1 = bitcast [4 x i8]* @g32_d to i8*
  %b1 = load i8* %ptr1

  %int_ptr2 = ptrtoint [4 x i8]* @g32_d to i32
  %int_ptr_bump2 = add i32 %int_ptr2, 1
  %ptr2 = inttoptr i32 %int_ptr_bump2 to i8*
  %b2 = load i8* %ptr2

  %int_ptr_bump3 = add i32 %int_ptr2, 2
  %ptr3 = inttoptr i32 %int_ptr_bump3 to i8*
  %b3 = load i8* %ptr3

  call void @llvm.nacl.atomic.fence.all()

  %int_ptr_bump4 = add i32 %int_ptr2, 3
  %ptr4 = inttoptr i32 %int_ptr_bump4 to i8*
  %b4 = load i8* %ptr4

  %b1.ext = zext i8 %b1 to i32
  %b2.ext = zext i8 %b2 to i32
  %b2.shift = shl i32 %b2.ext, 8
  %b12 = or i32 %b1.ext, %b2.shift
  %b3.ext = zext i8 %b3 to i32
  %b3.shift = shl i32 %b3.ext, 16
  %b123 = or i32 %b12, %b3.shift
  %b4.ext = zext i8 %b4 to i32
  %b4.shift = shl i32 %b4.ext, 24
  %b1234 = or i32 %b123, %b4.shift
  ret i32 %b1234
}
; CHECK-LABEL: could_have_fused_loads
; CHECK: lea {{.*}}, g32_d
; CHECK: mov {{.*}}, byte ptr
; CHECK: mov {{.*}}, byte ptr
; CHECK: mov {{.*}}, byte ptr
; CHECK: mfence
; CHECK: mov {{.*}}, byte ptr


; Test where an identical load from two branches could have been hoisted
; up, and then the code merged, but a fence prevents it.
define i32 @could_have_hoisted_loads(i32 %x) {
entry:
  %ptr = bitcast [4 x i8]* @g32_d to i32*
  %cmp = icmp eq i32 %x, 1
  br i1 %cmp, label %branch1, label %branch2
branch1:
  %y = load i32* %ptr
  ret i32 %y
branch2:
  call void @llvm.nacl.atomic.fence.all()
  %z = load i32* %ptr
  ret i32 %z
}
; CHECK-LABEL: could_have_hoisted_loads
; CHECK: lea {{.*}}, g32_d
; CHECK: je {{.*}}
; CHECK: jmp {{.*}}
; CHECK: mov {{.*}}, dword ptr
; CHECK: ret
; CHECK: mfence
; CHECK: mov {{.*}}, dword ptr
; CHECK: ret