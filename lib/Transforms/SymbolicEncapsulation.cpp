//
//
//

#include "Ephippion/Transforms/SymbolicEncapsulation.hpp"

#include "Ephippion/Support/Utils/FuncUtils.hpp"

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

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <cassert>
// using assert

#define DEBUG_TYPE "eph-symenc"

bool ephippion::SymbolicEncapsulation::encapsulate(llvm::Module &M) {
  bool hasChanged = false;

  for (auto &func : M) {
    hasChanged |= encapsulate(func);
  }

  return hasChanged;
}

bool ephippion::SymbolicEncapsulation::encapsulate(llvm::Function &F) {
  if (F.isDeclaration()) {
    return false;
  }

  LLVM_DEBUG(llvm::dbgs() << "processing func: " << F.getName() << '\n';);

  auto &curM = *F.getParent();
  auto *encapsulatedFunc = curM.getFunction(F.getName());
  if (!encapsulatedFunc) {
    llvm::report_fatal_error("Function: " + F.getName() +
                             " was not found in module: " + curM.getName());
  }

  auto &curCtx = curM.getContext();

  std::string harnessName = HarnessNamePrefix.str() + F.getName().str();
  auto *harnessFunc =
      DeclareFunc(curM, harnessName, llvm::Type::getVoidTy(curCtx));

  auto *setupBlock =
      llvm::BasicBlock::Create(curM.getContext(), "setup", harnessFunc);
  auto *symbBlock =
      llvm::BasicBlock::Create(curM.getContext(), "symb", harnessFunc);
  auto *callBlock =
      llvm::BasicBlock::Create(curM.getContext(), "call", harnessFunc);
  auto *teardownBlock =
      llvm::BasicBlock::Create(curM.getContext(), "teardown", harnessFunc);

  llvm::SmallVector<llvm::Value *, 8> callArgs;
  setupHarnessArgs(F.arg_begin(), F.arg_end(), *setupBlock, *teardownBlock,
                   callArgs);

  return true;
}

void ephippion::SymbolicEncapsulation::setupHarnessArgs(
    llvm::Function::arg_iterator Begin, llvm::Function::arg_iterator End,
    llvm::BasicBlock &SetupBlock, llvm::BasicBlock &TeardownBlock,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs) {
  if (Begin != End) {
    return;
  }

  llvm::IRBuilder<> builder{&SetupBlock};
  llvm::SmallVector<llvm::Instruction *, 16> heapAllocs;

  auto &curM = *Begin->getParent()->getParent();

  auto *heapAllocFunc = DeclareMallocLikeFunc(curM, HeapAllocFuncName);
  auto *heapDeallocFunc = DeclareFreeLikeFunc(curM, HeapDeallocFuncName);

  for (auto &curArg : llvm::make_range(Begin, End)) {
    if (!curArg.getType()->isPointerTy()) {
      CallArgs.push_back(
          builder.CreateAlloca(curArg.getType(), 0, curArg.getName()));
    } else {
      // TODO more checking has to take place here for more ptr indirection

      auto &dataLayout = curArg.getParent()->getParent()->getDataLayout();
      size_t typeSize = dataLayout.getTypeAllocSize(
          curArg.getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      auto *allocSize =
          builder.CreateMul(builder.getInt32(5), builder.getInt32(typeSize));

      heapAllocs.push_back(builder.CreateCall(heapAllocFunc, allocSize));
      CallArgs.push_back(heapAllocs.back());
    }
  }

  builder.SetInsertPoint(&TeardownBlock);
  std::reverse(heapAllocs.begin(), heapAllocs.end());
  for (auto *alloc : heapAllocs) {
    builder.CreateCall(heapDeallocFunc, alloc);
  }
}

