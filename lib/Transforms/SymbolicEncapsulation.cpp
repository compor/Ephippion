//
//
//

#include "Ephippion/Transforms/SymbolicEncapsulation.hpp"

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/IRBuilder.h"
// using llvm::IRBuilder

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include <cassert>
// using assert

bool ephippion::SymbolicEncapsulation::encapsulate(llvm::Module &M,
                                                   llvm::Function &F) {
  auto *encapsulatedFunc = M.getFunction(F.getName());
  if (!encapsulatedFunc) {
    llvm::report_fatal_error("Function: " + F.getName() +
                             " was not found in module: " + M.getName());
  }

  std::string harnessName = HarnessNamePrefix.str() + F.getName().str();
  auto *harnessFuncPtr =
      M.getOrInsertFunction(harnessName, getHarnessType(M.getContext()));
  auto *harnessFunc = llvm::cast<llvm::Function>(harnessFuncPtr);

  auto *setupBlock =
      llvm::BasicBlock::Create(M.getContext(), "setup", harnessFunc);
  auto *symbBlock =
      llvm::BasicBlock::Create(M.getContext(), "symb", harnessFunc);
  auto *callBlock =
      llvm::BasicBlock::Create(M.getContext(), "call", harnessFunc);
  auto *teardownBlock =
      llvm::BasicBlock::Create(M.getContext(), "teardown", harnessFunc);

  return true;
}

void ephippion::SymbolicEncapsulation::setupHarnessArgs(
    llvm::Function::arg_iterator Begin, llvm::Function::arg_iterator End,
    llvm::BasicBlock &SetupBlock, llvm::BasicBlock &TeardownBlock,
    llvm::SmallVectorImpl<llvm::Value *> &Args) {
  llvm::IRBuilder<> builder{&SetupBlock};
  llvm::SmallVector<llvm::Instruction *, 16> heapAllocs;

  for (auto &curArg : llvm::make_range(Begin, End)) {
    if (!curArg.getType()->isPointerTy()) {
      Args.push_back(
          builder.CreateAlloca(curArg.getType(), 0, curArg.getName()));
    } else {
      // TODO more checking has to take place here for more ptr indirection

      auto &dataLayout = curArg.getParent()->getParent()->getDataLayout();
      size_t typeSize = dataLayout.getTypeAllocSize(
          curArg.getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      auto *allocSize =
          builder.CreateMul(builder.getInt32(5), builder.getInt32(typeSize));

      heapAllocs.push_back(builder.CreateCall(HeapAllocFunc, allocSize));
      Args.push_back(heapAllocs.back());
    }
  }

  builder.SetInsertPoint(&TeardownBlock);
  std::reverse(heapAllocs.begin(), heapAllocs.end());
  for (auto *alloc : heapAllocs) {
    builder.CreateCall(HeapDeallocFunc, alloc);
  }
}
