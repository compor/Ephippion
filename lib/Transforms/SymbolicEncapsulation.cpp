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

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <cstring>
// using std::to_string

#include <cassert>
// using assert

#define DEBUG_TYPE "eph-symenc"

namespace ephippion {

bool SymbolicEncapsulation::encapsulate(llvm::Module &M) {
  bool hasChanged = false;

  for (auto &func : M) {
    hasChanged |= encapsulate(func);
  }

  return hasChanged;
}

bool SymbolicEncapsulation::encapsulate(llvm::Function &F) {
  return encapsulateImpl(F, {});
}

bool SymbolicEncapsulation::encapsulate(
    llvm::Function &F, llvm::ArrayRef<ArgDirection> Directions) {
  assert(F.arg_size() == Directions.size() &&
         "Missing direction information for function arguments!");

  return encapsulateImpl(F, Directions);
}

bool SymbolicEncapsulation::encapsulateImpl(
    llvm::Function &F, llvm::ArrayRef<ArgDirection> Directions) {
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
  auto *seSetupBlock =
      llvm::BasicBlock::Create(curM.getContext(), "se.setup", harnessFunc);
  auto *callsBlock =
      llvm::BasicBlock::Create(curM.getContext(), "calls", harnessFunc);
  auto *seTeardownBlock =
      llvm::BasicBlock::Create(curM.getContext(), "se.teardown", harnessFunc);
  auto *teardownBlock =
      llvm::BasicBlock::Create(curM.getContext(), "teardown", harnessFunc);
  auto *exitBlock =
      llvm::BasicBlock::Create(curM.getContext(), "exit", harnessFunc);

  llvm::SmallVector<llvm::Value *, 8> callArgs1, callArgs2;
  setupHarnessArgs(F.arg_begin(), F.arg_end(), Directions, *setupBlock,
                   *teardownBlock, callArgs1, callArgs2);

  createSymbolicDeclarations(*seSetupBlock, callArgs1, Directions);
  createSymbolicAssertions(*seTeardownBlock, callArgs1, callArgs2, Directions);

  llvm::IRBuilder<> builder{curCtx};

  // setup calls

  createCall(*callsBlock, F, callArgs1);
  createCall(*callsBlock, F, callArgs2);

  // setup control flow

  builder.SetInsertPoint(setupBlock);
  builder.CreateBr(seSetupBlock);
  builder.SetInsertPoint(seSetupBlock);
  builder.CreateBr(callsBlock);
  builder.SetInsertPoint(callsBlock);
  builder.CreateBr(seTeardownBlock);
  builder.SetInsertPoint(seTeardownBlock);
  builder.CreateBr(teardownBlock);
  builder.SetInsertPoint(teardownBlock);
  builder.CreateBr(exitBlock);

  // set harness return
  builder.SetInsertPoint(exitBlock);
  builder.CreateRetVoid();

  return true;
}

void SymbolicEncapsulation::setupHarnessArgs(
    llvm::Function::arg_iterator Begin, llvm::Function::arg_iterator End,
    llvm::ArrayRef<ArgDirection> Directions, llvm::BasicBlock &SetupBlock,
    llvm::BasicBlock &TeardownBlock,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs1,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs2) {
  if (Begin == End) {
    return;
  }

  llvm::IRBuilder<> builder{&SetupBlock};
  llvm::SmallVector<llvm::Instruction *, 16> heapAllocs;

  auto &curM = *Begin->getParent()->getParent();

  auto *heapAllocFunc = DeclareMallocLikeFunc(curM, HeapAllocFuncName);
  auto *heapDeallocFunc = DeclareFreeLikeFunc(curM, HeapDeallocFuncName);

  size_t argIdx = 0;
  for (auto &curArg : llvm::make_range(Begin, End)) {
    ArgDirection dir = AD_Inbound;
    if (Directions.size() && argIdx < Directions.size()) {
      dir = Directions[argIdx];
    }

    if (!curArg.getType()->isPointerTy()) {
      auto *arg1 = builder.CreateAlloca(curArg.getType(), 0, curArg.getName());
      CallArgs1.push_back(arg1);

      decltype(auto) arg2 = arg1;
      if (isOutbound(dir)) {
        arg2 = builder.CreateAlloca(curArg.getType(), 0, curArg.getName());
      }
      CallArgs2.push_back(arg2);
    } else {
      // TODO more checking has to take place here for more ptr indirection

      auto &dataLayout = curArg.getParent()->getParent()->getDataLayout();
      size_t typeSize = dataLayout.getTypeAllocSize(
          curArg.getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      auto *allocSize = builder.CreateMul(builder.getInt64(AllocElementsNum),
                                          builder.getInt64(typeSize));

      auto *arg1 = builder.CreateCall(heapAllocFunc, allocSize);
      heapAllocs.push_back(arg1);
      CallArgs1.push_back(arg1);

      decltype(auto) arg2 = arg1;
      if (isOutbound(dir)) {
        arg2 = builder.CreateCall(heapAllocFunc, allocSize);
        heapAllocs.push_back(arg2);
      }
      CallArgs2.push_back(arg2);
    }

    ++argIdx;
  }

  builder.SetInsertPoint(&TeardownBlock);
  std::reverse(heapAllocs.begin(), heapAllocs.end());
  for (auto *alloc : heapAllocs) {
    builder.CreateCall(heapDeallocFunc, alloc);
  }
}

void SymbolicEncapsulation::createSymbolicDeclarations(
    llvm::BasicBlock &Block, llvm::SmallVectorImpl<llvm::Value *> &Values,
    llvm::ArrayRef<ArgDirection> Directions) {
  assert(Values.size() && "Value set is empty!");

  auto &curM = *Block.getParent()->getParent();
  auto &dataLayout = curM.getDataLayout();

  auto *symbolizeFunc = DeclareKLEELikeFunc(curM, "klee_make_symbolic");

  llvm::IRBuilder<> builder{&Block};

  for (size_t i = 0; i < Values.size(); ++i) {
    if (isOnlyOutbound(Directions[i])) {
      continue;
    }

    if (!Values[i]->getType()->isPointerTy()) {
      // TODO
    } else {
      // TODO indexing should be modified depending the values iterator
      // dependence
      size_t typeSize = dataLayout.getTypeAllocSize(
          Values[i]->getType()->getPointerElementType());
      // TODO this allocation size needs to change
      auto *allocSize =
          builder.CreateMul(builder.getInt64(1), builder.getInt64(typeSize));

      std::string symName =
          "sym.in." + Values[i]->getName().str() + std::to_string(i);
      builder.CreateCall(
          symbolizeFunc,
          {Values[i], allocSize, builder.CreateGlobalStringPtr(symName)});
    }
  }
}

void SymbolicEncapsulation::createSymbolicAssertions(
    llvm::BasicBlock &Block, llvm::SmallVectorImpl<llvm::Value *> &Values1,
    llvm::SmallVectorImpl<llvm::Value *> &Values2,
    llvm::ArrayRef<ArgDirection> Directions) {
  assert(Values1.size() && Values2.size() && "Value sets are empty!");
  assert(Values1.size() == Values2.size() && "Value set sizes differ!");

  auto &curM = *Block.getParent()->getParent();

  auto *assertFunc = DeclareKLEELikeFunc(curM, "klee_assert");

  llvm::IRBuilder<> builder{&Block};

  for (size_t i = 0; i < Values1.size(); ++i) {
    if (!isOutbound(Directions[i])) {
      continue;
    }

    if (!Values1[i]->getType()->isPointerTy()) {
      auto *cond = builder.CreateICmpEQ(Values1[i], Values2[i]);
      builder.CreateCall(assertFunc, cond);
    } else {
      // TODO indexing should be modified depending the values iterator
      // dependence
      auto *val1 = builder.CreateInBoundsGEP(Values1[i], {builder.getInt64(0)});
      auto *val2 = builder.CreateInBoundsGEP(Values2[i], {builder.getInt64(0)});

      auto *cond = builder.CreateICmpEQ(val1, val2);
      builder.CreateCall(assertFunc, cond);
    }
  }
}

void SymbolicEncapsulation::createCall(
    llvm::BasicBlock &Block, llvm::Function &Func,
    llvm::SmallVectorImpl<llvm::Value *> &Args) {
  llvm::IRBuilder<> builder{&Block};
  llvm::SmallVector<llvm::Value *, 8> castArgs;
  auto *funcType = Func.getFunctionType();

  for (size_t i = 0; i < Args.size(); ++i) {
    castArgs.push_back(
        builder.CreateBitCast(Args[i], funcType->getParamType(i)));
  }

  builder.CreateCall(&Func, castArgs);
}

} // namespace ephippion

