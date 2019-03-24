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

bool SymbolicEncapsulation::encapsulate(llvm::Module &M,
                                        IterationsNumTy IterationsNum) {
  bool hasChanged = false;

  for (auto &func : M) {
    hasChanged |= encapsulate(func, IterationsNum);
  }

  return hasChanged;
}

bool SymbolicEncapsulation::encapsulate(llvm::Function &F,
                                        IterationsNumTy IterationsNum) {
  return encapsulateImpl(F, IterationsNum, {});
}

bool SymbolicEncapsulation::encapsulate(llvm::Function &F,
                                        IterationsNumTy IterationsNum,
                                        llvm::ArrayRef<ArgSpec> ArgSpecs) {
  // either argspec information must match func args or be empty
  assert((ArgSpecs.size() == 0 || F.arg_size() == ArgSpecs.size()) &&
         "Missing argspec information for function arguments!");

  return encapsulateImpl(F, IterationsNum, ArgSpecs);
}

bool SymbolicEncapsulation::encapsulateImpl(llvm::Function &F,
                                            IterationsNumTy IterationsNum,
                                            llvm::ArrayRef<ArgSpec> ArgSpecs) {
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
  setupHarnessArgs(F.arg_begin(), F.arg_end(), ArgSpecs, *setupBlock,
                   *teardownBlock, IterationsNum, callArgs1, callArgs2);

  createSymbolicDeclarations(*seSetupBlock, F, callArgs1, callArgs2,
                             IterationsNum, ArgSpecs);
  createSymbolicAssertions(*seTeardownBlock, callArgs1, callArgs2,
                           IterationsNum, ArgSpecs);

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
    llvm::ArrayRef<ArgSpec> ArgSpecs, llvm::BasicBlock &SetupBlock,
    llvm::BasicBlock &TeardownBlock, IterationsNumTy IterationsNum,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs1,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs2) {
  if (Begin == End) {
    return;
  }

  llvm::IRBuilder<> builder{&SetupBlock};
  llvm::SmallVector<llvm::Instruction *, 16> heapAllocs;

  auto &curM = *Begin->getParent()->getParent();
  auto &dataLayout = curM.getDataLayout();

  auto *heapAllocFunc = DeclareMallocLikeFunc(curM, HeapAllocFuncName);
  auto *heapDeallocFunc = DeclareFreeLikeFunc(curM, HeapDeallocFuncName);

  size_t argIdx = 0;
  for (auto &curArg : llvm::make_range(Begin, End)) {
    ArgDirection dir = AD_Inbound;
    if (ArgSpecs.size() && argIdx < ArgSpecs.size()) {
      dir = ArgSpecs[argIdx].Direction;
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

      size_t typeSize = dataLayout.getTypeAllocSize(
          curArg.getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      llvm::Value *multiplier = ArgSpecs[argIdx].IteratorDependent
                                    ? builder.getInt64(IterationsNum)
                                    : builder.getInt64(1);

      auto *allocSize =
          builder.CreateMul(multiplier, builder.getInt64(typeSize));

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
    llvm::BasicBlock &Block, llvm::Function &Func,
    llvm::SmallVectorImpl<llvm::Value *> &Values1,
    llvm::SmallVectorImpl<llvm::Value *> &Values2,
    IterationsNumTy IterationsNum, llvm::ArrayRef<ArgSpec> ArgSpecs) {
  assert((Values1.size() || Values2.size()) && "Value sets are empty!");

  auto &curM = *Block.getParent()->getParent();
  auto &dataLayout = curM.getDataLayout();

  auto *symbolizeFunc = DeclareKLEELikeFunc(curM, "klee_make_symbolic");

  llvm::IRBuilder<> builder{&Block};

  size_t argIdx = 0;
  for (auto &curArg : Func.args()) {
    if (ArgSpecs.size() && isOnlyOutbound(ArgSpecs[argIdx].Direction)) {
      ++argIdx;
      continue;
    }

    if (!Values1[argIdx]->getType()->isPointerTy()) {
      // TODO
    } else {
      size_t typeSize = dataLayout.getTypeAllocSize(
          curArg.getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      llvm::Value *multiplier = ArgSpecs[argIdx].IteratorDependent
                                    ? builder.getInt64(IterationsNum)
                                    : builder.getInt64(1);

      auto *allocSize =
          builder.CreateMul(multiplier, builder.getInt64(typeSize));

      std::string symName =
          "sym.in." + Values1[argIdx]->getName().str() + std::to_string(argIdx);
      builder.CreateCall(
          symbolizeFunc,
          {Values1[argIdx], allocSize, builder.CreateGlobalStringPtr(symName)});

      if (isOutbound(ArgSpecs[argIdx].Direction)) {
        std::string symName = "sym.in." + Values2[argIdx]->getName().str() +
                              std::to_string(argIdx);
        builder.CreateCall(symbolizeFunc,
                           {Values2[argIdx], allocSize,
                            builder.CreateGlobalStringPtr(symName)});
      }
    }

    ++argIdx;
  }
}

void SymbolicEncapsulation::createSymbolicAssertions(
    llvm::BasicBlock &Block, llvm::SmallVectorImpl<llvm::Value *> &Values1,
    llvm::SmallVectorImpl<llvm::Value *> &Values2,
    IterationsNumTy IterationsNum, llvm::ArrayRef<ArgSpec> ArgSpecs) {
  assert(Values1.size() && Values2.size() && "Value sets are empty!");
  assert(Values1.size() == Values2.size() && "Value set sizes differ!");

  auto &curM = *Block.getParent()->getParent();
  auto &dataLayout = curM.getDataLayout();

  auto *assertFunc = DeclareKLEELikeFunc(curM, "klee_assert");
  auto *memcmpFunc = DeclareKLEELikeFunc(curM, "memcmp");

  llvm::IRBuilder<> builder{&Block};

  for (size_t i = 0; i < Values1.size(); ++i) {
    if (ArgSpecs.size() && !isOutbound(ArgSpecs[i].Direction)) {
      continue;
    }

    if (!Values1[i]->getType()->isPointerTy()) {
      auto *cond = builder.CreateICmpEQ(Values1[i], Values2[i]);
      builder.CreateCall(assertFunc, cond);
    } else {
      size_t typeSize = dataLayout.getTypeAllocSize(
          Values1[i]->getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      llvm::Value *multiplier = ArgSpecs[i].IteratorDependent
                                    ? builder.getInt64(IterationsNum)
                                    : builder.getInt64(1);

      auto *allocSize =
          builder.CreateMul(multiplier, builder.getInt64(typeSize));

      auto *call =
          builder.CreateCall(memcmpFunc, {Values1[i], Values2[i], allocSize});
      auto *cond = builder.CreateICmpEQ(call, builder.getInt32(0));
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

