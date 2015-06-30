//===- subzero/src/IceConditionCodesX8664.h - Condition Codes ---*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the condition codes for x86-64.
//
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICECONDITIONCODESX8664_H
#define SUBZERO_SRC_ICECONDITIONCODESX8664_H

#include "IceDefs.h"
#include "IceInstX8664.def"

namespace Ice {

namespace CondX8664 {
// An enum of condition codes used for branches and cmov. The enum value
// should match the value used to encode operands in binary instructions.
enum BrCond {
#define X(tag, encode, opp, dump, emit) tag encode,
  ICEINSTX8664BR_TABLE
#undef X
      Br_None
};

// An enum of condition codes relevant to the CMPPS instruction. The enum
// value should match the value used to encode operands in binary
// instructions.
enum CmppsCond {
#define X(tag, emit) tag,
  ICEINSTX8664CMPPS_TABLE
#undef X
      Cmpps_Invalid
};

} // end of namespace CondX8664

} // end of namespace Ice

#endif // SUBZERO_SRC_ICECONDITIONCODESX8664_H