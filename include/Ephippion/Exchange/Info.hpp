//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgSpec.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include <vector>
// using std::vector

namespace llvm {
class Function;
} // namespace llvm

namespace ephippion {

struct FunctionArgSpec {
  llvm::Function *Func;
  llvm::Loop *CurLoop;
  std::vector<ArgSpec> Args;
};

} // namespace ephippion

