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

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include <string>
// using std::string

namespace llvm {
class Function;
class Module;
} // namespace llvm

namespace ephippion {

class SymbolicEncapsulation {
  llvm::StringRef HarnessNamePrefix;
  llvm::FunctionType *HarnessType;

  llvm::FunctionType *getCapsuleType(llvm::LLVMContext &C) {
    return llvm::FunctionType::get(
        llvm::Type::getVoidTy(C),
        llvm::ArrayRef<llvm::Type *>{llvm::Type::getVoidTy(C)}, false);
  }

  void setupHarnessArgs(llvm::BasicBlock &SetupBlock,
                        llvm::BasicBlock &TeardownBlock,
                        llvm::Function::arg_iterator Begin,
                        llvm::Function::arg_iterator End);

public:
  explicit SymbolicEncapsulation(llvm::StringRef Prefix = "symenc_")
      : HarnessNamePrefix(Prefix), HarnessType(nullptr) {}

  SymbolicEncapsulation(const SymbolicEncapsulation &) = default;

  bool encapsulate(llvm::Module &M, llvm::Function &F);
};

} // namespace ephippion
