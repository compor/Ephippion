//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgDirection.hpp"

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/DerivedTypes.h"
// using llvm::FunctionType

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVectorImpl

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include <string>
// using std::string

#include <cstdint>
// using uint64_t

namespace llvm {
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
                        llvm::ArrayRef<ArgDirection> Directions,
                        llvm::BasicBlock &SetupBlock,
                        llvm::BasicBlock &TeardownBlock,
                        llvm::SmallVectorImpl<llvm::Value *> &CallArgs1,
                        llvm::SmallVectorImpl<llvm::Value *> &CallArgs2);

  void addSEAssertions(llvm::BasicBlock &Block,
                       llvm::SmallVectorImpl<llvm::Value *> &Values1,
                       llvm::SmallVectorImpl<llvm::Value *> &Values2,
                       llvm::ArrayRef<ArgDirection> Directions);

  bool encapsulateImpl(llvm::Function &F,
                       llvm::ArrayRef<ArgDirection> Directions);

public:
  explicit SymbolicEncapsulation(uint64_t AllocElementsNum,
                                 llvm::StringRef Prefix = "symenc_")
      : AllocElementsNum(AllocElementsNum), HarnessNamePrefix{Prefix},
        HeapAllocFuncName{"malloc"}, HeapDeallocFuncName{"free"} {}

  SymbolicEncapsulation(const SymbolicEncapsulation &) = default;

  bool encapsulate(llvm::Module &M);
  bool encapsulate(llvm::Function &F);
  bool encapsulate(llvm::Function &F, llvm::ArrayRef<ArgDirection> Directions);
};

} // namespace ephippion
