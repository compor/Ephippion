//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

namespace llvm {
class BasicBlock;
} // namespace llvm

namespace ephippion {

class LoopIRBuilder {
public:
  LoopIRBuilder() = default;

  void CreateLoop(llvm::ArrayRef<llvm::BasicBlock *> Body,
                  llvm::BasicBlock &Preheader);
};

} // namespace ephippion

