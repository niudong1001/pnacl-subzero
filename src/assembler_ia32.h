// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//
// Modified by the Subzero authors.
//
//===- subzero/src/assembler_ia32.h - Assembler for x86-32 ----------------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Assembler class for x86-32.
//
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ASSEMBLER_IA32_H_
#define SUBZERO_SRC_ASSEMBLER_IA32_H_

#include "IceDefs.h"
#include "IceConditionCodesX8632.h"
#include "IceRegistersX8632.h"
#include "IceTypes.h"
#include "IceUtils.h"

#include "assembler.h"

namespace Ice {

class Assembler;
class ConstantRelocatable;

using RegX8632::GPRRegister;
using RegX8632::XmmRegister;
using RegX8632::ByteRegister;

namespace x86 {

const int MAX_NOP_SIZE = 8;

enum ScaleFactor { TIMES_1 = 0, TIMES_2 = 1, TIMES_4 = 2, TIMES_8 = 3 };

class DisplacementRelocation : public AssemblerFixup {
public:
  static DisplacementRelocation *create(Assembler *Asm, FixupKind Kind,
                                        const ConstantRelocatable *Sym) {
    return new (Asm->Allocate<DisplacementRelocation>())
        DisplacementRelocation(Kind, Sym);
  }

  void Process(const MemoryRegion &region, intptr_t position) override {
    (void)region;
    (void)position;
    llvm_unreachable("We might not be using this Process() method later.");
  }

private:
  DisplacementRelocation(FixupKind Kind, const ConstantRelocatable *Sym)
      : AssemblerFixup(Kind, Sym) {}
  DisplacementRelocation(const DisplacementRelocation &) LLVM_DELETED_FUNCTION;
  DisplacementRelocation &
  operator=(const DisplacementRelocation &) LLVM_DELETED_FUNCTION;
};

class Immediate {
public:
  explicit Immediate(int32_t value) : value_(value) {}

  Immediate(const Immediate &other) : value_(other.value_) {}

  int32_t value() const { return value_; }

  bool is_int8() const { return Utils::IsInt(8, value_); }
  bool is_uint8() const { return Utils::IsUint(8, value_); }
  bool is_uint16() const { return Utils::IsUint(16, value_); }

private:
  const int32_t value_;
};

class Operand {
public:
  uint8_t mod() const { return (encoding_at(0) >> 6) & 3; }

  GPRRegister rm() const {
    return static_cast<GPRRegister>(encoding_at(0) & 7);
  }

  ScaleFactor scale() const {
    return static_cast<ScaleFactor>((encoding_at(1) >> 6) & 3);
  }

  GPRRegister index() const {
    return static_cast<GPRRegister>((encoding_at(1) >> 3) & 7);
  }

  GPRRegister base() const {
    return static_cast<GPRRegister>(encoding_at(1) & 7);
  }

  int8_t disp8() const {
    assert(length_ >= 2);
    return static_cast<int8_t>(encoding_[length_ - 1]);
  }

  int32_t disp32() const {
    assert(length_ >= 5);
    return bit_copy<int32_t>(encoding_[length_ - 4]);
  }

  AssemblerFixup *fixup() const { return fixup_; }

  Operand(const Operand &other) : length_(other.length_), fixup_(other.fixup_) {
    memmove(&encoding_[0], &other.encoding_[0], other.length_);
  }

  Operand &operator=(const Operand &other) {
    length_ = other.length_;
    fixup_ = other.fixup_;
    memmove(&encoding_[0], &other.encoding_[0], other.length_);
    return *this;
  }

protected:
  Operand() : length_(0), fixup_(NULL) {} // Needed by subclass Address.

  void SetModRM(int mod, GPRRegister rm) {
    assert((mod & ~3) == 0);
    encoding_[0] = (mod << 6) | rm;
    length_ = 1;
  }

  void SetSIB(ScaleFactor scale, GPRRegister index, GPRRegister base) {
    assert(length_ == 1);
    assert((scale & ~3) == 0);
    encoding_[1] = (scale << 6) | (index << 3) | base;
    length_ = 2;
  }

  void SetDisp8(int8_t disp) {
    assert(length_ == 1 || length_ == 2);
    encoding_[length_++] = static_cast<uint8_t>(disp);
  }

  void SetDisp32(int32_t disp) {
    assert(length_ == 1 || length_ == 2);
    intptr_t disp_size = sizeof(disp);
    memmove(&encoding_[length_], &disp, disp_size);
    length_ += disp_size;
  }

  void SetFixup(AssemblerFixup *fixup) { fixup_ = fixup; }

private:
  uint8_t length_;
  uint8_t encoding_[6];
  AssemblerFixup *fixup_;

  explicit Operand(GPRRegister reg) : fixup_(NULL) { SetModRM(3, reg); }

  // Get the operand encoding byte at the given index.
  uint8_t encoding_at(intptr_t index) const {
    assert(index >= 0 && index < length_);
    return encoding_[index];
  }

  // Returns whether or not this operand is really the given register in
  // disguise. Used from the assembler to generate better encodings.
  bool IsRegister(GPRRegister reg) const {
    return ((encoding_[0] & 0xF8) == 0xC0) // Addressing mode is register only.
           && ((encoding_[0] & 0x07) == reg); // Register codes match.
  }

  friend class AssemblerX86;
};

class Address : public Operand {
public:
  Address(GPRRegister base, int32_t disp) {
    if (disp == 0 && base != RegX8632::Encoded_Reg_ebp) {
      SetModRM(0, base);
      if (base == RegX8632::Encoded_Reg_esp)
        SetSIB(TIMES_1, RegX8632::Encoded_Reg_esp, base);
    } else if (Utils::IsInt(8, disp)) {
      SetModRM(1, base);
      if (base == RegX8632::Encoded_Reg_esp)
        SetSIB(TIMES_1, RegX8632::Encoded_Reg_esp, base);
      SetDisp8(disp);
    } else {
      SetModRM(2, base);
      if (base == RegX8632::Encoded_Reg_esp)
        SetSIB(TIMES_1, RegX8632::Encoded_Reg_esp, base);
      SetDisp32(disp);
    }
  }

  Address(GPRRegister index, ScaleFactor scale, int32_t disp) {
    assert(index != RegX8632::Encoded_Reg_esp); // Illegal addressing mode.
    SetModRM(0, RegX8632::Encoded_Reg_esp);
    SetSIB(scale, index, RegX8632::Encoded_Reg_ebp);
    SetDisp32(disp);
  }

  Address(GPRRegister base, GPRRegister index, ScaleFactor scale,
          int32_t disp) {
    assert(index != RegX8632::Encoded_Reg_esp); // Illegal addressing mode.
    if (disp == 0 && base != RegX8632::Encoded_Reg_ebp) {
      SetModRM(0, RegX8632::Encoded_Reg_esp);
      SetSIB(scale, index, base);
    } else if (Utils::IsInt(8, disp)) {
      SetModRM(1, RegX8632::Encoded_Reg_esp);
      SetSIB(scale, index, base);
      SetDisp8(disp);
    } else {
      SetModRM(2, RegX8632::Encoded_Reg_esp);
      SetSIB(scale, index, base);
      SetDisp32(disp);
    }
  }

  Address(const Address &other) : Operand(other) {}

  Address &operator=(const Address &other) {
    Operand::operator=(other);
    return *this;
  }

  static Address Absolute(const uintptr_t addr, AssemblerFixup *fixup) {
    Address result;
    result.SetModRM(0, RegX8632::Encoded_Reg_ebp);
    result.SetDisp32(addr);
    result.SetFixup(fixup);
    return result;
  }

  static Address ofConstPool(GlobalContext *Ctx, Assembler *Asm,
                             const Constant *Imm);

private:
  Address() {} // Needed by Address::Absolute.
};

class Label {
public:
  Label() : position_(0), num_unresolved_(0) {
#ifdef DEBUG
    for (int i = 0; i < kMaxUnresolvedBranches; i++) {
      unresolved_near_positions_[i] = -1;
    }
#endif // DEBUG
  }

  ~Label() {
    // Assert if label is being destroyed with unresolved branches pending.
    assert(!IsLinked());
    assert(!HasNear());
  }

  // TODO(jvoung): why are labels offset by this?
  static const uint32_t kWordSize = sizeof(uint32_t);

  // Returns the position for bound labels (branches that come after this
  // are considered backward branches). Cannot be used for unused or linked
  // labels.
  intptr_t Position() const {
    assert(IsBound());
    return -position_ - kWordSize;
  }

  // Returns the position of an earlier branch instruction that was linked
  // to this label (branches that use this are considered forward branches).
  // The linked instructions form a linked list, of sorts, using the
  // instruction's displacement field for the location of the next
  // instruction that is also linked to this label.
  intptr_t LinkPosition() const {
    assert(IsLinked());
    return position_ - kWordSize;
  }

  // Returns the position of an earlier branch instruction which
  // assumes that this label is "near", and bumps iterator to the
  // next near position.
  intptr_t NearPosition() {
    assert(HasNear());
    return unresolved_near_positions_[--num_unresolved_];
  }

  bool IsBound() const { return position_ < 0; }
  bool IsLinked() const { return position_ > 0; }
  bool IsUnused() const { return (position_ == 0) && (num_unresolved_ == 0); }
  bool HasNear() const { return num_unresolved_ != 0; }

private:
  void BindTo(intptr_t position) {
    assert(!IsBound());
    assert(!HasNear());
    position_ = -position - kWordSize;
    assert(IsBound());
  }

  void LinkTo(intptr_t position) {
    assert(!IsBound());
    position_ = position + kWordSize;
    assert(IsLinked());
  }

  void NearLinkTo(intptr_t position) {
    assert(!IsBound());
    assert(num_unresolved_ < kMaxUnresolvedBranches);
    unresolved_near_positions_[num_unresolved_++] = position;
  }

  static const int kMaxUnresolvedBranches = 20;

  intptr_t position_;
  intptr_t num_unresolved_;
  intptr_t unresolved_near_positions_[kMaxUnresolvedBranches];

  friend class AssemblerX86;
  Label(const Label &) LLVM_DELETED_FUNCTION;
  Label &operator=(const Label &) LLVM_DELETED_FUNCTION;
};

class AssemblerX86 : public Assembler {
public:
  explicit AssemblerX86(bool use_far_branches = false) : buffer_(*this) {
    // This mode is only needed and implemented for MIPS and ARM.
    assert(!use_far_branches);
  }
  ~AssemblerX86() {}

  static const bool kNearJump = true;
  static const bool kFarJump = false;

  // Operations to emit GPR instructions (and dispatch on operand type).
  typedef void (AssemblerX86::*TypedEmitGPR)(Type, GPRRegister);
  typedef void (AssemblerX86::*TypedEmitAddr)(Type, const Address &);
  struct GPREmitterOneOp {
    TypedEmitGPR Reg;
    TypedEmitAddr Addr;
  };

  typedef void (AssemblerX86::*TypedEmitGPRGPR)(Type, GPRRegister, GPRRegister);
  typedef void (AssemblerX86::*TypedEmitGPRAddr)(Type, GPRRegister,
                                                 const Address &);
  typedef void (AssemblerX86::*TypedEmitGPRImm)(Type, GPRRegister,
                                                const Immediate &);
  struct GPREmitterRegOp {
    TypedEmitGPRGPR GPRGPR;
    TypedEmitGPRAddr GPRAddr;
    TypedEmitGPRImm GPRImm;
  };

  // Operations to emit XMM instructions (and dispatch on operand type).
  typedef void (AssemblerX86::*TypedEmitXmmXmm)(Type, XmmRegister, XmmRegister);
  typedef void (AssemblerX86::*TypedEmitXmmAddr)(Type, XmmRegister,
                                                 const Address &);
  typedef void (AssemblerX86::*TypedEmitAddrXmm)(Type, const Address &,
                                                 XmmRegister);
  struct XmmEmitterTwoOps {
    TypedEmitXmmXmm XmmXmm;
    TypedEmitXmmAddr XmmAddr;
    TypedEmitAddrXmm AddrXmm;
  };

  /*
   * Emit Machine Instructions.
   */
  void call(GPRRegister reg);
  void call(const Address &address);
  void call(Label *label);
  void call(const ConstantRelocatable *label);

  static const intptr_t kCallExternalLabelSize = 5;

  void pushl(GPRRegister reg);
  void pushl(const Address &address);
  void pushl(const Immediate &imm);

  void popl(GPRRegister reg);
  void popl(const Address &address);

  void pushal();
  void popal();

  void setcc(CondX86::BrCond condition, ByteRegister dst);

  void movl(GPRRegister dst, const Immediate &src);
  void movl(GPRRegister dst, GPRRegister src);

  void movl(GPRRegister dst, const Address &src);
  void movl(const Address &dst, GPRRegister src);
  void movl(const Address &dst, const Immediate &imm);

  void movzxb(GPRRegister dst, ByteRegister src);
  void movzxb(GPRRegister dst, const Address &src);
  void movsxb(GPRRegister dst, ByteRegister src);
  void movsxb(GPRRegister dst, const Address &src);

  void movb(ByteRegister dst, const Address &src);
  void movb(const Address &dst, ByteRegister src);
  void movb(const Address &dst, const Immediate &imm);

  void movzxw(GPRRegister dst, GPRRegister src);
  void movzxw(GPRRegister dst, const Address &src);
  void movsxw(GPRRegister dst, GPRRegister src);
  void movsxw(GPRRegister dst, const Address &src);
  void movw(GPRRegister dst, const Address &src);
  void movw(const Address &dst, GPRRegister src);

  void lea(Type Ty, GPRRegister dst, const Address &src);

  void cmov(CondX86::BrCond cond, GPRRegister dst, GPRRegister src);

  void rep_movsb();

  void movss(XmmRegister dst, const Address &src);
  void movss(const Address &dst, XmmRegister src);
  void movss(XmmRegister dst, XmmRegister src);

  void movd(XmmRegister dst, GPRRegister src);
  void movd(XmmRegister dst, const Address &src);
  void movd(GPRRegister dst, XmmRegister src);
  void movd(const Address &dst, XmmRegister src);

  void movq(const Address &dst, XmmRegister src);
  void movq(XmmRegister dst, const Address &src);

  void addss(Type Ty, XmmRegister dst, XmmRegister src);
  void addss(Type Ty, XmmRegister dst, const Address &src);
  void subss(Type Ty, XmmRegister dst, XmmRegister src);
  void subss(Type Ty, XmmRegister dst, const Address &src);
  void mulss(Type Ty, XmmRegister dst, XmmRegister src);
  void mulss(Type Ty, XmmRegister dst, const Address &src);
  void divss(Type Ty, XmmRegister dst, XmmRegister src);
  void divss(Type Ty, XmmRegister dst, const Address &src);

  void movsd(XmmRegister dst, const Address &src);
  void movsd(const Address &dst, XmmRegister src);
  void movsd(XmmRegister dst, XmmRegister src);

  void movaps(XmmRegister dst, XmmRegister src);

  void movups(XmmRegister dst, const Address &src);
  void movups(const Address &dst, XmmRegister src);

  void padd(Type Ty, XmmRegister dst, XmmRegister src);
  void padd(Type Ty, XmmRegister dst, const Address &src);
  void pand(Type Ty, XmmRegister dst, XmmRegister src);
  void pand(Type Ty, XmmRegister dst, const Address &src);
  void pandn(Type Ty, XmmRegister dst, XmmRegister src);
  void pandn(Type Ty, XmmRegister dst, const Address &src);
  void pmuludq(Type Ty, XmmRegister dst, XmmRegister src);
  void pmuludq(Type Ty, XmmRegister dst, const Address &src);
  void por(Type Ty, XmmRegister dst, XmmRegister src);
  void por(Type Ty, XmmRegister dst, const Address &src);
  void psub(Type Ty, XmmRegister dst, XmmRegister src);
  void psub(Type Ty, XmmRegister dst, const Address &src);
  void pxor(Type Ty, XmmRegister dst, XmmRegister src);
  void pxor(Type Ty, XmmRegister dst, const Address &src);

  void addps(Type Ty, XmmRegister dst, XmmRegister src);
  void addps(Type Ty, XmmRegister dst, const Address &src);
  void subps(Type Ty, XmmRegister dst, XmmRegister src);
  void subps(Type Ty, XmmRegister dst, const Address &src);
  void divps(Type Ty, XmmRegister dst, XmmRegister src);
  void divps(Type Ty, XmmRegister dst, const Address &src);
  void mulps(Type Ty, XmmRegister dst, XmmRegister src);
  void mulps(Type Ty, XmmRegister dst, const Address &src);
  void minps(XmmRegister dst, XmmRegister src);
  void maxps(XmmRegister dst, XmmRegister src);
  void andps(XmmRegister dst, XmmRegister src);
  void andps(XmmRegister dst, const Address &src);
  void orps(XmmRegister dst, XmmRegister src);

  void cmpps(XmmRegister dst, XmmRegister src, CondX86::CmppsCond CmpCondition);
  void cmpps(XmmRegister dst, const Address &src,
             CondX86::CmppsCond CmpCondition);

  void sqrtps(XmmRegister dst);
  void rsqrtps(XmmRegister dst);
  void reciprocalps(XmmRegister dst);
  void movhlps(XmmRegister dst, XmmRegister src);
  void movlhps(XmmRegister dst, XmmRegister src);
  void unpcklps(XmmRegister dst, XmmRegister src);
  void unpckhps(XmmRegister dst, XmmRegister src);
  void unpcklpd(XmmRegister dst, XmmRegister src);
  void unpckhpd(XmmRegister dst, XmmRegister src);

  void set1ps(XmmRegister dst, GPRRegister tmp, const Immediate &imm);
  void shufps(XmmRegister dst, XmmRegister src, const Immediate &mask);

  void minpd(XmmRegister dst, XmmRegister src);
  void maxpd(XmmRegister dst, XmmRegister src);
  void sqrtpd(XmmRegister dst);
  void cvtps2pd(XmmRegister dst, XmmRegister src);
  void cvtpd2ps(XmmRegister dst, XmmRegister src);
  void shufpd(XmmRegister dst, XmmRegister src, const Immediate &mask);

  void cvtsi2ss(XmmRegister dst, GPRRegister src);
  void cvtsi2sd(XmmRegister dst, GPRRegister src);

  void cvtss2si(GPRRegister dst, XmmRegister src);
  void cvtss2sd(XmmRegister dst, XmmRegister src);

  void cvtsd2si(GPRRegister dst, XmmRegister src);
  void cvtsd2ss(XmmRegister dst, XmmRegister src);

  void cvttss2si(GPRRegister dst, XmmRegister src);
  void cvttsd2si(GPRRegister dst, XmmRegister src);

  void cvtdq2pd(XmmRegister dst, XmmRegister src);

  void ucomiss(Type Ty, XmmRegister a, XmmRegister b);
  void ucomiss(Type Ty, XmmRegister a, const Address &b);

  void movmskpd(GPRRegister dst, XmmRegister src);
  void movmskps(GPRRegister dst, XmmRegister src);

  void sqrtss(Type Ty, XmmRegister dst, const Address &src);
  void sqrtss(Type Ty, XmmRegister dst, XmmRegister src);

  void xorpd(XmmRegister dst, const Address &src);
  void xorpd(XmmRegister dst, XmmRegister src);
  void xorps(XmmRegister dst, const Address &src);
  void xorps(XmmRegister dst, XmmRegister src);

  void andpd(XmmRegister dst, const Address &src);
  void andpd(XmmRegister dst, XmmRegister src);

  void orpd(XmmRegister dst, XmmRegister src);

  void pextrd(GPRRegister dst, XmmRegister src, const Immediate &imm);
  void pmovsxdq(XmmRegister dst, XmmRegister src);
  void pcmpeqq(XmmRegister dst, XmmRegister src);

  enum RoundingMode {
    kRoundToNearest = 0x0,
    kRoundDown = 0x1,
    kRoundUp = 0x2,
    kRoundToZero = 0x3
  };
  void roundsd(XmmRegister dst, XmmRegister src, RoundingMode mode);

  void flds(const Address &src);
  void fstps(const Address &dst);

  void fldl(const Address &src);
  void fstpl(const Address &dst);

  void fnstcw(const Address &dst);
  void fldcw(const Address &src);

  void fistpl(const Address &dst);
  void fistps(const Address &dst);
  void fildl(const Address &src);
  void filds(const Address &src);

  void fincstp();

  void cmpl(GPRRegister reg, const Immediate &imm);
  void cmpl(GPRRegister reg0, GPRRegister reg1);
  void cmpl(GPRRegister reg, const Address &address);
  void cmpl(const Address &address, GPRRegister reg);
  void cmpl(const Address &address, const Immediate &imm);
  void cmpb(const Address &address, const Immediate &imm);

  void testl(GPRRegister reg1, GPRRegister reg2);
  void testl(GPRRegister reg, const Immediate &imm);

  void And(Type Ty, GPRRegister dst, GPRRegister src);
  void And(Type Ty, GPRRegister dst, const Address &address);
  void And(Type Ty, GPRRegister dst, const Immediate &imm);

  void Or(Type Ty, GPRRegister dst, GPRRegister src);
  void Or(Type Ty, GPRRegister dst, const Address &address);
  void Or(Type Ty, GPRRegister dst, const Immediate &imm);

  void Xor(Type Ty, GPRRegister dst, GPRRegister src);
  void Xor(Type Ty, GPRRegister dst, const Address &address);
  void Xor(Type Ty, GPRRegister dst, const Immediate &imm);

  void add(Type Ty, GPRRegister dst, GPRRegister src);
  void add(Type Ty, GPRRegister reg, const Address &address);
  void add(Type Ty, GPRRegister reg, const Immediate &imm);

  void adc(Type Ty, GPRRegister dst, GPRRegister src);
  void adc(Type Ty, GPRRegister dst, const Address &address);
  void adc(Type Ty, GPRRegister reg, const Immediate &imm);

  void sub(Type Ty, GPRRegister dst, GPRRegister src);
  void sub(Type Ty, GPRRegister reg, const Address &address);
  void sub(Type Ty, GPRRegister reg, const Immediate &imm);

  void sbb(Type Ty, GPRRegister dst, GPRRegister src);
  void sbb(Type Ty, GPRRegister reg, const Address &address);
  void sbb(Type Ty, GPRRegister reg, const Immediate &imm);

  void cbw();
  void cwd();
  void cdq();

  void div(Type Ty, GPRRegister reg);
  void div(Type Ty, const Address &address);

  void idiv(Type Ty, GPRRegister reg);
  void idiv(Type Ty, const Address &address);

  void imull(GPRRegister dst, GPRRegister src);
  void imull(GPRRegister reg, const Immediate &imm);
  void imull(GPRRegister reg, const Address &address);

  void imull(GPRRegister reg);
  void imull(const Address &address);

  void mul(Type Ty, GPRRegister reg);
  void mul(Type Ty, const Address &address);

  void incl(GPRRegister reg);
  void incl(const Address &address);

  void decl(GPRRegister reg);
  void decl(const Address &address);

  void shll(GPRRegister reg, const Immediate &imm);
  void shll(GPRRegister operand, GPRRegister shifter);
  void shll(const Address &operand, GPRRegister shifter);
  void shrl(GPRRegister reg, const Immediate &imm);
  void shrl(GPRRegister operand, GPRRegister shifter);
  void sarl(GPRRegister reg, const Immediate &imm);
  void sarl(GPRRegister operand, GPRRegister shifter);
  void sarl(const Address &address, GPRRegister shifter);
  void shld(GPRRegister dst, GPRRegister src);
  void shld(GPRRegister dst, GPRRegister src, const Immediate &imm);
  void shld(const Address &operand, GPRRegister src);
  void shrd(GPRRegister dst, GPRRegister src);
  void shrd(GPRRegister dst, GPRRegister src, const Immediate &imm);
  void shrd(const Address &dst, GPRRegister src);

  void neg(Type Ty, GPRRegister reg);
  void neg(Type Ty, const Address &addr);
  void notl(GPRRegister reg);

  void bsf(Type Ty, GPRRegister dst, GPRRegister src);
  void bsf(Type Ty, GPRRegister dst, const Address &src);
  void bsr(Type Ty, GPRRegister dst, GPRRegister src);
  void bsr(Type Ty, GPRRegister dst, const Address &src);

  void bswap(Type Ty, GPRRegister reg);

  void bt(GPRRegister base, GPRRegister offset);

  void ret();
  void ret(const Immediate &imm);

  // 'size' indicates size in bytes and must be in the range 1..8.
  void nop(int size = 1);
  void int3();
  void hlt();

  void j(CondX86::BrCond condition, Label *label, bool near = kFarJump);
  void j(CondX86::BrCond condition, const ConstantRelocatable *label);

  void jmp(GPRRegister reg);
  void jmp(Label *label, bool near = kFarJump);
  void jmp(const ConstantRelocatable *label);

  void mfence();

  void lock();
  void cmpxchg(Type Ty, const Address &address, GPRRegister reg);
  void cmpxchg8b(const Address &address);
  void xadd(Type Ty, const Address &address, GPRRegister reg);
  void xchg(Type Ty, const Address &address, GPRRegister reg);

  void LockCmpxchg(Type Ty, const Address &address, GPRRegister reg) {
    lock();
    cmpxchg(Ty, address, reg);
  }

  intptr_t PreferredLoopAlignment() { return 16; }
  void Align(intptr_t alignment, intptr_t offset);
  void Bind(Label *label);

  intptr_t CodeSize() const { return buffer_.Size(); }

  void FinalizeInstructions(const MemoryRegion &region) {
    buffer_.FinalizeInstructions(region);
  }

  // Expose the buffer, for bringup...
  intptr_t GetPosition() const { return buffer_.GetPosition(); }
  template <typename T> T LoadBuffer(intptr_t position) const {
    return buffer_.Load<T>(position);
  }
  AssemblerFixup *GetLatestFixup() const { return buffer_.GetLatestFixup(); }

private:
  inline void EmitUint8(uint8_t value);
  inline void EmitInt16(int16_t value);
  inline void EmitInt32(int32_t value);
  inline void EmitRegisterOperand(int rm, int reg);
  inline void EmitXmmRegisterOperand(int rm, XmmRegister reg);
  inline void EmitFixup(AssemblerFixup *fixup);
  inline void EmitOperandSizeOverride();

  void EmitOperand(int rm, const Operand &operand);
  void EmitImmediate(Type ty, const Immediate &imm);
  void EmitComplexI8(int rm, const Operand &operand,
                     const Immediate &immediate);
  void EmitComplex(Type Ty, int rm, const Operand &operand,
                   const Immediate &immediate);
  void EmitLabel(Label *label, intptr_t instruction_size);
  void EmitLabelLink(Label *label);
  void EmitNearLabelLink(Label *label);

  void EmitGenericShift(int rm, GPRRegister reg, const Immediate &imm);
  void EmitGenericShift(int rm, const Operand &operand, GPRRegister shifter);

  AssemblerBuffer buffer_;

  AssemblerX86(const AssemblerX86 &) LLVM_DELETED_FUNCTION;
  AssemblerX86 &operator=(const AssemblerX86 &) LLVM_DELETED_FUNCTION;
};

inline void AssemblerX86::EmitUint8(uint8_t value) {
  buffer_.Emit<uint8_t>(value);
}

inline void AssemblerX86::EmitInt16(int16_t value) {
  buffer_.Emit<int16_t>(value);
}

inline void AssemblerX86::EmitInt32(int32_t value) {
  buffer_.Emit<int32_t>(value);
}

inline void AssemblerX86::EmitRegisterOperand(int rm, int reg) {
  assert(rm >= 0 && rm < 8);
  buffer_.Emit<uint8_t>(0xC0 + (rm << 3) + reg);
}

inline void AssemblerX86::EmitXmmRegisterOperand(int rm, XmmRegister reg) {
  EmitRegisterOperand(rm, static_cast<GPRRegister>(reg));
}

inline void AssemblerX86::EmitFixup(AssemblerFixup *fixup) {
  buffer_.EmitFixup(fixup);
}

inline void AssemblerX86::EmitOperandSizeOverride() { EmitUint8(0x66); }

} // end of namespace x86
} // end of namespace Ice

#endif // SUBZERO_SRC_ASSEMBLER_IA32_H_