//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/DerivedTypes.h"
// using llvm::FunctionType

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include <string>
// using std::string

#include <cstdint>
// using uint64_t

namespace llvm {
class Function;
class Module;
} // namespace llvm

namespace ephippion {

class SymbolicEncapsulation {
  llvm::StringRef HarnessNamePrefix;
  llvm::StringRef HeapAllocFuncName;
  llvm::StringRef HeapDeallocFuncName;
  uint64_t AllocElementsNum;

  void setupHarnessArgs(llvm::Function::arg_iterator Begin,
                        llvm::Function::arg_iterator End,
                        llvm::BasicBlock &SetupBlock,
                        llvm::BasicBlock &TeardownBlock,
                        llvm::SmallVectorImpl<llvm::Value *> &Args);

public:
  explicit SymbolicEncapsulation(uint64_t AllocElementsNum,
                                 llvm::StringRef Prefix = "symenc_")
      : AllocElementsNum(AllocElementsNum), HarnessNamePrefix{Prefix},
        HeapAllocFuncName{"malloc"}, HeapDeallocFuncName{"free"} {}

  SymbolicEncapsulation(const SymbolicEncapsulation &) = default;

  bool encapsulate(llvm::Module &M);
  bool encapsulate(llvm::Function &F);
};

} // namespace ephippion
