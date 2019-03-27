//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

#include <cstdint>
// using uint32_t

namespace llvm {
class BasicBlock;
class Instruction;
} // namespace llvm

namespace ephippion {

class LoopIRBuilder {
public:
  llvm::Instruction *CreateLoop(llvm::ArrayRef<llvm::BasicBlock *> Body,
                                llvm::BasicBlock &Preheader,
                                llvm::BasicBlock &Postexit, uint32_t Start = 0,
                                uint32_t End = 10, uint32_t Step = 1);
};

} // namespace ephippion

