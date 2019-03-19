//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

#include <cstdint>
// using uint64_t

namespace llvm {
class BasicBlock;
class Instruction;
} // namespace llvm

namespace ephippion {

class LoopIRBuilder {
public:
  LoopIRBuilder() = default;

  llvm::Instruction *CreateLoop(llvm::ArrayRef<llvm::BasicBlock *> Body,
                                llvm::BasicBlock &Preheader,
                                llvm::BasicBlock &Postexit, uint64_t Step = 1);
};

} // namespace ephippion
