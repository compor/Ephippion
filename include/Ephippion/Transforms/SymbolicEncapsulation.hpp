//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgSpec.hpp"

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
class Function;
} // namespace llvm

namespace ephippion {

class SymbolicEncapsulation {
  llvm::StringRef HarnessNamePrefix;
  llvm::StringRef HeapAllocFuncName;
  llvm::StringRef HeapDeallocFuncName;

  void setupHarnessArgs(llvm::Function::arg_iterator Begin,
                        llvm::Function::arg_iterator End,
                        llvm::ArrayRef<ArgSpec> ArgSpecs,
                        llvm::BasicBlock &SetupBlock,
                        llvm::BasicBlock &TeardownBlock, uint64_t IterationsNum,
                        llvm::SmallVectorImpl<llvm::Value *> &CallArgs1,
                        llvm::SmallVectorImpl<llvm::Value *> &CallArgs2);

  void createSymbolicDeclarations(llvm::BasicBlock &Block,
                                  llvm::SmallVectorImpl<llvm::Value *> &Values,
                                  llvm::ArrayRef<ArgSpec> ArgSpecs);

  void createSymbolicAssertions(llvm::BasicBlock &Block,
                                llvm::SmallVectorImpl<llvm::Value *> &Values1,
                                llvm::SmallVectorImpl<llvm::Value *> &Values2,
                                llvm::ArrayRef<ArgSpec> ArgSpecs,
                                uint64_t IterationsNum);

  void createCall(llvm::BasicBlock &Block, llvm::Function &Func,
                  llvm::SmallVectorImpl<llvm::Value *> &Args);

  bool encapsulateImpl(llvm::Function &F, llvm::ArrayRef<ArgSpec> ArgSpecs,
                       uint64_t IterationsNum);

public:
  explicit SymbolicEncapsulation(llvm::StringRef Prefix = "symenc_")
      : HarnessNamePrefix{Prefix}, HeapAllocFuncName{"malloc"},
        HeapDeallocFuncName{"free"} {}

  SymbolicEncapsulation(const SymbolicEncapsulation &) = default;

  bool encapsulate(llvm::Module &M, uint64_t IterationsNum);
  bool encapsulate(llvm::Function &F, uint64_t IterationsNum);
  bool encapsulate(llvm::Function &F, uint64_t IterationsNum,
                   llvm::ArrayRef<ArgSpec> ArgSpecs);
};

} // namespace ephippion
