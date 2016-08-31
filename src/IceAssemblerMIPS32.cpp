//===- subzero/src/IceAssemblerMIPS32.cpp - MIPS32 Assembler --------------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Implements the Assembler class for MIPS32.
///
//===----------------------------------------------------------------------===//

#include "IceAssemblerMIPS32.h"
#include "IceCfgNode.h"
#include "IceRegistersMIPS32.h"
#include "IceUtils.h"

namespace {

using namespace Ice;
using namespace Ice::MIPS32;

// Offset modifier to current PC for next instruction.
static constexpr IOffsetT kPCReadOffset = 4;

// Mask to pull out PC offset from branch instruction.
static constexpr int kBranchOffsetBits = 16;
static constexpr IOffsetT kBranchOffsetMask = 0x0000ffff;

} // end of anonymous namespace

namespace Ice {
namespace MIPS32 {

void AssemblerMIPS32::emitTextInst(const std::string &Text, SizeT InstSize) {
  AssemblerFixup *F = createTextFixup(Text, InstSize);
  emitFixup(F);
  for (SizeT I = 0; I < InstSize; ++I) {
    AssemblerBuffer::EnsureCapacity ensured(&Buffer);
    Buffer.emit<char>(0);
  }
}

namespace {

// TEQ $0, $0 - Trap if equal
static constexpr uint8_t TrapBytesRaw[] = {0x00, 0x00, 0x00, 0x34};

const auto TrapBytes =
    llvm::ArrayRef<uint8_t>(TrapBytesRaw, llvm::array_lengthof(TrapBytesRaw));

} // end of anonymous namespace

llvm::ArrayRef<uint8_t> AssemblerMIPS32::getNonExecBundlePadding() const {
  return TrapBytes;
}

void AssemblerMIPS32::trap() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  for (const uint8_t &Byte : reverse_range(TrapBytes))
    Buffer.emit<uint8_t>(Byte);
}

void AssemblerMIPS32::nop() { emitInst(0); }

void AssemblerMIPS32::padWithNop(intptr_t Padding) {
  constexpr intptr_t InstWidth = sizeof(IValueT);
  assert(Padding % InstWidth == 0 &&
         "Padding not multiple of instruction size");
  for (intptr_t i = 0; i < Padding; i += InstWidth)
    nop();
}

Label *AssemblerMIPS32::getOrCreateLabel(SizeT Number, LabelVector &Labels) {
  Label *L = nullptr;
  if (Number == Labels.size()) {
    L = new (this->allocate<Label>()) Label();
    Labels.push_back(L);
    return L;
  }
  if (Number > Labels.size()) {
    Labels.resize(Number + 1);
  }
  L = Labels[Number];
  if (L == nullptr) {
    L = new (this->allocate<Label>()) Label();
    Labels[Number] = L;
  }
  return L;
}

void AssemblerMIPS32::bindCfgNodeLabel(const CfgNode *Node) {
  if (BuildDefs::dump() && !getFlags().getDisableHybridAssembly()) {
    constexpr SizeT InstSize = 0;
    emitTextInst(Node->getAsmName() + ":", InstSize);
  }
  SizeT NodeNumber = Node->getIndex();
  assert(!getPreliminary());
  Label *L = getOrCreateCfgNodeLabel(NodeNumber);
  this->bind(L);
}

// Checks that Offset can fit in imm16 constant of branch instruction.
void assertCanEncodeBranchOffset(IOffsetT Offset) {
  (void)Offset;
  (void)kBranchOffsetBits;
  assert(Utils::IsAligned(Offset, 4));
  assert(Utils::IsInt(kBranchOffsetBits, Offset >> 2));
}

IValueT encodeBranchOffset(IOffsetT Offset, IValueT Inst) {
  Offset -= kPCReadOffset;
  assertCanEncodeBranchOffset(Offset);
  Offset >>= 2;
  Offset &= kBranchOffsetMask;
  return (Inst & ~kBranchOffsetMask) | Offset;
}

IOffsetT AssemblerMIPS32::decodeBranchOffset(IValueT Inst) {
  int16_t imm = (Inst & kBranchOffsetMask);
  IOffsetT Offset = imm;
  Offset = Offset << 2;
  return (Offset + kPCReadOffset);
}

void AssemblerMIPS32::bind(Label *L) {
  IOffsetT BoundPc = Buffer.size();
  assert(!L->isBound()); // Labels can only be bound once.
  while (L->isLinked()) {
    IOffsetT Position = L->getLinkPosition();
    IOffsetT Dest = BoundPc - Position;
    IValueT Inst = Buffer.load<IValueT>(Position);
    Buffer.store<IValueT>(Position, encodeBranchOffset(Dest, Inst));
    L->setPosition(decodeBranchOffset(Inst));
  }
  L->bindTo(BoundPc);
}

enum RegSetWanted { WantGPRegs, WantFPRegs };

IValueT getEncodedGPRegNum(const Variable *Var) {
  assert(Var->hasReg());
  const auto Reg = Var->getRegNum();
  return RegMIPS32::getEncodedGPR(Reg);
}

bool encodeOperand(const Operand *Opnd, IValueT &Value,
                   RegSetWanted WantedRegSet) {
  Value = 0;
  if (const auto *Var = llvm::dyn_cast<Variable>(Opnd)) {
    if (Var->hasReg()) {
      switch (WantedRegSet) {
      case WantGPRegs:
        Value = getEncodedGPRegNum(Var);
        break;
      default:
        break;
      }
      return true;
    }
    return false;
  }
  return false;
}

IValueT encodeRegister(const Operand *OpReg, RegSetWanted WantedRegSet,
                       const char *RegName, const char *InstName) {
  IValueT Reg = 0;
  if (encodeOperand(OpReg, Reg, WantedRegSet) != true)
    llvm::report_fatal_error(std::string(InstName) + ": Can't find register " +
                             RegName);
  return Reg;
}

IValueT encodeGPRegister(const Operand *OpReg, const char *RegName,
                         const char *InstName) {
  return encodeRegister(OpReg, WantGPRegs, RegName, InstName);
}

void AssemblerMIPS32::emitRtRsImm16(IValueT Opcode, const Operand *OpRt,
                                    const Operand *OpRs, const uint32_t Imm,
                                    const char *InsnName) {
  const IValueT Rt = encodeGPRegister(OpRt, "Rt", InsnName);
  const IValueT Rs = encodeGPRegister(OpRs, "Rs", InsnName);

  Opcode |= Rs << 21;
  Opcode |= Rt << 16;
  Opcode |= Imm & 0xffff;

  emitInst(Opcode);
}

void AssemblerMIPS32::emitRdRtSa(IValueT Opcode, const Operand *OpRd,
                                 const Operand *OpRt, const uint32_t Sa,
                                 const char *InsnName) {
  const IValueT Rd = encodeGPRegister(OpRd, "Rd", InsnName);
  const IValueT Rt = encodeGPRegister(OpRt, "Rt", InsnName);

  Opcode |= Rt << 16;
  Opcode |= Rd << 11;
  Opcode |= (Sa & 0x1f) << 6;

  emitInst(Opcode);
}

void AssemblerMIPS32::emitRdRsRt(IValueT Opcode, const Operand *OpRd,
                                 const Operand *OpRs, const Operand *OpRt,
                                 const char *InsnName) {
  const IValueT Rd = encodeGPRegister(OpRd, "Rd", InsnName);
  const IValueT Rs = encodeGPRegister(OpRs, "Rs", InsnName);
  const IValueT Rt = encodeGPRegister(OpRt, "Rt", InsnName);

  Opcode |= Rs << 21;
  Opcode |= Rt << 16;
  Opcode |= Rd << 11;

  emitInst(Opcode);
}

void AssemblerMIPS32::addiu(const Operand *OpRt, const Operand *OpRs,
                            const uint32_t Imm) {
  static constexpr IValueT Opcode = 0x24000000;
  emitRtRsImm16(Opcode, OpRt, OpRs, Imm, "addiu");
}

void AssemblerMIPS32::slti(const Operand *OpRt, const Operand *OpRs,
                           const uint32_t Imm) {
  static constexpr IValueT Opcode = 0x28000000;
  emitRtRsImm16(Opcode, OpRt, OpRs, Imm, "slti");
}

void AssemblerMIPS32::sltiu(const Operand *OpRt, const Operand *OpRs,
                            const uint32_t Imm) {
  static constexpr IValueT Opcode = 0x2c000000;
  emitRtRsImm16(Opcode, OpRt, OpRs, Imm, "sltiu");
}

void AssemblerMIPS32::and_(const Operand *OpRd, const Operand *OpRs,
                           const Operand *OpRt) {
  static constexpr IValueT Opcode = 0x00000024;
  emitRdRsRt(Opcode, OpRd, OpRs, OpRt, "and");
}

void AssemblerMIPS32::andi(const Operand *OpRt, const Operand *OpRs,
                           const uint32_t Imm) {
  static constexpr IValueT Opcode = 0x30000000;
  emitRtRsImm16(Opcode, OpRt, OpRs, Imm, "andi");
}

void AssemblerMIPS32::or_(const Operand *OpRd, const Operand *OpRs,
                          const Operand *OpRt) {
  static constexpr IValueT Opcode = 0x00000025;
  emitRdRsRt(Opcode, OpRd, OpRs, OpRt, "or");
}

void AssemblerMIPS32::ori(const Operand *OpRt, const Operand *OpRs,
                          const uint32_t Imm) {
  static constexpr IValueT Opcode = 0x34000000;
  emitRtRsImm16(Opcode, OpRt, OpRs, Imm, "ori");
}

void AssemblerMIPS32::xor_(const Operand *OpRd, const Operand *OpRs,
                           const Operand *OpRt) {
  static constexpr IValueT Opcode = 0x00000026;
  emitRdRsRt(Opcode, OpRd, OpRs, OpRt, "xor");
}

void AssemblerMIPS32::xori(const Operand *OpRt, const Operand *OpRs,
                           const uint32_t Imm) {
  static constexpr IValueT Opcode = 0x38000000;
  emitRtRsImm16(Opcode, OpRt, OpRs, Imm, "xori");
}

void AssemblerMIPS32::sll(const Operand *OpRd, const Operand *OpRt,
                          const uint32_t Sa) {
  static constexpr IValueT Opcode = 0x00000000;
  emitRdRtSa(Opcode, OpRd, OpRt, Sa, "sll");
}

void AssemblerMIPS32::srl(const Operand *OpRd, const Operand *OpRt,
                          const uint32_t Sa) {
  static constexpr IValueT Opcode = 0x00000002;
  emitRdRtSa(Opcode, OpRd, OpRt, Sa, "srl");
}

void AssemblerMIPS32::sra(const Operand *OpRd, const Operand *OpRt,
                          const uint32_t Sa) {
  static constexpr IValueT Opcode = 0x00000003;
  emitRdRtSa(Opcode, OpRd, OpRt, Sa, "sra");
}

void AssemblerMIPS32::move(const Operand *OpRd, const Operand *OpRs) {
  IValueT Opcode = 0x00000021;
  const IValueT Rd = encodeGPRegister(OpRd, "Rd", "pseudo-move");
  const IValueT Rs = encodeGPRegister(OpRs, "Rs", "pseudo-move");
  const IValueT Rt = 0; // $0
  Opcode |= Rs << 21;
  Opcode |= Rt << 16;
  Opcode |= Rd << 11;
  emitInst(Opcode);
}

void AssemblerMIPS32::addu(const Operand *OpRd, const Operand *OpRs,
                           const Operand *OpRt) {
  static constexpr IValueT Opcode = 0x00000021;
  emitRdRsRt(Opcode, OpRd, OpRs, OpRt, "addu");
}

void AssemblerMIPS32::sltu(const Operand *OpRd, const Operand *OpRs,
                           const Operand *OpRt) {
  static constexpr IValueT Opcode = 0x0000002B;
  emitRdRsRt(Opcode, OpRd, OpRs, OpRt, "sltu");
}

void AssemblerMIPS32::slt(const Operand *OpRd, const Operand *OpRs,
                          const Operand *OpRt) {
  static constexpr IValueT Opcode = 0x0000002A;
  emitRdRsRt(Opcode, OpRd, OpRs, OpRt, "slt");
}

void AssemblerMIPS32::sw(const Operand *OpRt, const Operand *OpBase,
                         const uint32_t Offset) {
  static constexpr IValueT Opcode = 0xAC000000;
  emitRtRsImm16(Opcode, OpRt, OpBase, Offset, "sw");
}

void AssemblerMIPS32::lw(const Operand *OpRt, const Operand *OpBase,
                         const uint32_t Offset) {
  static constexpr IValueT Opcode = 0x8C000000;
  emitRtRsImm16(Opcode, OpRt, OpBase, Offset, "lw");
}

void AssemblerMIPS32::ret(void) {
  static constexpr IValueT Opcode = 0x03E00008; // JR $31
  emitInst(Opcode);
  nop(); // delay slot
}

void AssemblerMIPS32::emitBr(const CondMIPS32::Cond Cond, const Operand *OpRs,
                             const Operand *OpRt, IOffsetT Offset) {
  IValueT Opcode = 0;

  switch (Cond) {
  default:
    break;
  case CondMIPS32::AL:
  case CondMIPS32::EQ:
  case CondMIPS32::EQZ:
    Opcode = 0x10000000;
    break;
  case CondMIPS32::NE:
  case CondMIPS32::NEZ:
    Opcode = 0x14000000;
    break;
  case CondMIPS32::LEZ:
    Opcode = 0x18000000;
    break;
  case CondMIPS32::LTZ:
    Opcode = 0x04000000;
    break;
  case CondMIPS32::GEZ:
    Opcode = 0x04010000;
    break;
  case CondMIPS32::GTZ:
    Opcode = 0x1C000000;
    break;
  }

  if (Opcode == 0) {
    llvm::report_fatal_error("Branch: Invalid condition");
  }

  if (OpRs != nullptr) {
    IValueT Rs = encodeGPRegister(OpRs, "Rs", "branch");
    Opcode |= Rs << 21;
  }

  if (OpRt != nullptr) {
    IValueT Rt = encodeGPRegister(OpRt, "Rt", "branch");
    Opcode |= Rt << 16;
  }

  Opcode = encodeBranchOffset(Offset, Opcode);
  emitInst(Opcode);
  nop(); // delay slot
}

void AssemblerMIPS32::b(Label *TargetLabel) {
  static constexpr Operand *OpRsNone = nullptr;
  static constexpr Operand *OpRtNone = nullptr;
  if (TargetLabel->isBound()) {
    const int32_t Dest = TargetLabel->getPosition() - Buffer.size();
    emitBr(CondMIPS32::AL, OpRsNone, OpRtNone, Dest);
    return;
  }
  const IOffsetT Position = Buffer.size();
  emitBr(CondMIPS32::AL, OpRsNone, OpRtNone, TargetLabel->getEncodedPosition());
  TargetLabel->linkTo(*this, Position);
}

void AssemblerMIPS32::bcc(const CondMIPS32::Cond Cond, const Operand *OpRs,
                          const Operand *OpRt, Label *TargetLabel) {
  if (TargetLabel->isBound()) {
    const int32_t Dest = TargetLabel->getPosition() - Buffer.size();
    emitBr(Cond, OpRs, OpRt, Dest);
    return;
  }
  const IOffsetT Position = Buffer.size();
  emitBr(Cond, OpRs, OpRt, TargetLabel->getEncodedPosition());
  TargetLabel->linkTo(*this, Position);
}

void AssemblerMIPS32::bzc(const CondMIPS32::Cond Cond, const Operand *OpRs,
                          Label *TargetLabel) {
  static constexpr Operand *OpRtNone = nullptr;
  if (TargetLabel->isBound()) {
    const int32_t Dest = TargetLabel->getPosition() - Buffer.size();
    emitBr(Cond, OpRs, OpRtNone, Dest);
    return;
  }
  const IOffsetT Position = Buffer.size();
  emitBr(Cond, OpRs, OpRtNone, TargetLabel->getEncodedPosition());
  TargetLabel->linkTo(*this, Position);
}

} // end of namespace MIPS32
} // end of namespace Ice