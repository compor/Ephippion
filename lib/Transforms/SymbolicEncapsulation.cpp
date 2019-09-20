//
//
//

#include "Ephippion/Transforms/SymbolicEncapsulation.hpp"

#include "Ephippion/Support/Utils/FuncUtils.hpp"

#include "Ephippion/Support/IR/LoopIRBuilder.hpp"

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/IRBuilder.h"
// using llvm::IRBuilder

#include "llvm/IR/Instructions.h"
// using llvm::CreateZExtOrBitcast

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
  return encapsulateImpl(F, IterationsNum, ArgSpecs);
}

bool SymbolicEncapsulation::encapsulateImpl(llvm::Function &F,
                                            IterationsNumTy IterationsNum,
                                            llvm::ArrayRef<ArgSpec> ArgSpecs) {
  if (F.isDeclaration()) {
    return false;
  }

  LLVM_DEBUG(llvm::dbgs() << "processing func: " << F.getName() << '\n';);

  llvm::SmallVector<ArgSpec, 16> argSpecs;
  auto numParams = F.getFunctionType()->getNumParams();
  argSpecs.reserve(numParams);

  argSpecs.insert(argSpecs.begin(), ArgSpecs.begin(), ArgSpecs.end());

  LLVM_DEBUG({
    if (argSpecs.size() < numParams) {
      llvm::dbgs() << "filling in missing arg spec with default\n";
    }
  });

  while (argSpecs.size() < numParams) {
    argSpecs.push_back({"", ArgDirection::AD_Inbound, false});
  }

  auto &curM = *F.getParent();
  auto &curCtx = curM.getContext();

  std::string harnessName = HarnessNamePrefix.str() + F.getName().str();
  auto *harnessFunc =
      DeclareFunc(curM, harnessName, llvm::Type::getInt32Ty(curCtx));

  auto *setupBlock =
      llvm::BasicBlock::Create(curM.getContext(), "setup", harnessFunc);
  auto *seSetupBlock =
      llvm::BasicBlock::Create(curM.getContext(), "se.setup", harnessFunc);
  auto *call1SetupBlock =
      llvm::BasicBlock::Create(curM.getContext(), "call1.setup", harnessFunc);
  auto *call2SetupBlock =
      llvm::BasicBlock::Create(curM.getContext(), "call2.setup", harnessFunc);
  auto *call1Block =
      llvm::BasicBlock::Create(curM.getContext(), "call1.body", harnessFunc);
  auto *call2Block =
      llvm::BasicBlock::Create(curM.getContext(), "call2.body", harnessFunc);
  auto *call1TeardownBlock = llvm::BasicBlock::Create(
      curM.getContext(), "call1.teardown", harnessFunc);
  auto *call2TeardownBlock = llvm::BasicBlock::Create(
      curM.getContext(), "call2.teardown", harnessFunc);
  auto *seStartTeardownBlock = llvm::BasicBlock::Create(
      curM.getContext(), "se.start.teardown", harnessFunc);
  auto *seEndTeardownBlock = llvm::BasicBlock::Create(
      curM.getContext(), "se.end.teardown", harnessFunc);
  auto *teardownBlock =
      llvm::BasicBlock::Create(curM.getContext(), "teardown", harnessFunc);
  auto *exitBlock =
      llvm::BasicBlock::Create(curM.getContext(), "exit", harnessFunc);

  llvm::SmallVector<llvm::Value *, 8> callArgs1, callArgs2;
  setupHarnessArgs(F, argSpecs, *setupBlock, *teardownBlock, IterationsNum,
                   callArgs1, callArgs2);

  createSymbolicDeclarations(*seSetupBlock, F, callArgs1, callArgs2,
                             IterationsNum, argSpecs);
  createSymbolicAssertions(*seStartTeardownBlock, *seEndTeardownBlock,
                           callArgs1, callArgs2, IterationsNum, argSpecs);

  llvm::IRBuilder<> builder{curCtx};

  // setup calls
  LoopIRBuilder loopBuilder;

  auto *indVar1 = loopBuilder.CreateLoop({call1Block}, *call1SetupBlock,
                                         *call1TeardownBlock, 0, IterationsNum);
  auto *indVar2 =
      loopBuilder.CreateLoop({call2Block}, *call2SetupBlock,
                             *call2TeardownBlock, IterationsNum - 1, 0);

  // pass the iterator as the first argument
  callArgs1[0] = indVar1;
  callArgs2[0] = indVar2;

  createCall(*call1Block, F, callArgs1);
  createCall(*call2Block, F, callArgs2);

  // setup control flow

  builder.SetInsertPoint(setupBlock);
  builder.CreateBr(seSetupBlock);
  builder.SetInsertPoint(seSetupBlock);
  builder.CreateBr(call1SetupBlock);
  builder.SetInsertPoint(call1TeardownBlock);
  builder.CreateBr(call2SetupBlock);
  builder.SetInsertPoint(call2TeardownBlock);
  builder.CreateBr(seStartTeardownBlock);
  builder.SetInsertPoint(seEndTeardownBlock);
  builder.CreateBr(teardownBlock);
  builder.SetInsertPoint(teardownBlock);
  builder.CreateBr(exitBlock);

  // set harness return
  builder.SetInsertPoint(exitBlock);
  builder.CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder.getContext()), 0));

  return true;
}

void SymbolicEncapsulation::setupHarnessArgs(
    llvm::Function &EncapsulatedFunc, llvm::ArrayRef<ArgSpec> ArgSpecs,
    llvm::BasicBlock &SetupBlock, llvm::BasicBlock &TeardownBlock,
    IterationsNumTy IterationsNum,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs1,
    llvm::SmallVectorImpl<llvm::Value *> &CallArgs2) {
  if (EncapsulatedFunc.arg_size() == 0) {
    return;
  }

  llvm::IRBuilder<> builder{&SetupBlock};
  llvm::SmallVector<llvm::Instruction *, 16> heapAllocs;

  auto &curM = *EncapsulatedFunc.getParent();
  auto &dataLayout = curM.getDataLayout();

  auto *heapAllocFunc = DeclareMallocLikeFunc(curM, HeapAllocFuncName);
  auto *heapDeallocFunc = DeclareFreeLikeFunc(curM, HeapDeallocFuncName);

  size_t argIdx = 0;
  for (auto &curArg : EncapsulatedFunc.args()) {
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
    llvm::BasicBlock &Block, llvm::Function &EncapsulatedFunc,
    llvm::SmallVectorImpl<llvm::Value *> &Values1,
    llvm::SmallVectorImpl<llvm::Value *> &Values2,
    IterationsNumTy IterationsNum, llvm::ArrayRef<ArgSpec> ArgSpecs) {
  assert(Values1.size() && Values2.size() && "Value sets are empty!");

  auto &curM = *Block.getParent()->getParent();
  auto &dataLayout = curM.getDataLayout();

  auto *symbolizeFunc = DeclareKLEELikeFunc(curM, "klee_make_symbolic");
  auto *memcpyFunc = DeclareMemcpyLikeFunc(curM, "memcpy");

  llvm::IRBuilder<> builder{&Block};

  size_t argIdx = 0;
  for (auto &curArg : EncapsulatedFunc.args()) {
    if (ArgSpecs.size() && isOnlyOutbound(ArgSpecs[argIdx].Direction)) {
      ++argIdx;
      continue;
    }

    if (!curArg.getType()->isPointerTy()) {
      size_t typeSize = dataLayout.getTypeAllocSize(
          Values1[argIdx]->getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      auto *allocSize =
          builder.CreateMul(builder.getInt64(1), builder.getInt64(typeSize));

      std::string symName = "sym.in1." + Values1[argIdx]->getName().str() +
                            std::to_string(argIdx);
      builder.CreateCall(
          symbolizeFunc,
          {Values1[argIdx], allocSize, builder.CreateGlobalStringPtr(symName)});

      if (isOutbound(ArgSpecs[argIdx].Direction)) {
        std::string symName = "sym.in2." + Values2[argIdx]->getName().str() +
                              std::to_string(argIdx);
        builder.CreateCall(symbolizeFunc,
                           {Values2[argIdx], allocSize,
                            builder.CreateGlobalStringPtr(symName)});

        builder.CreateCall(memcpyFunc,
                           {Values2[argIdx], Values1[argIdx], allocSize});
      }
    } else {
      size_t typeSize = dataLayout.getTypeAllocSize(
          curArg.getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      llvm::Value *multiplier = ArgSpecs[argIdx].IteratorDependent
                                    ? builder.getInt64(IterationsNum)
                                    : builder.getInt64(1);

      auto *allocSize =
          builder.CreateMul(multiplier, builder.getInt64(typeSize));

      std::string symName = "sym.in1." + Values1[argIdx]->getName().str() +
                            std::to_string(argIdx);
      builder.CreateCall(
          symbolizeFunc,
          {Values1[argIdx], allocSize, builder.CreateGlobalStringPtr(symName)});

      if (isOutbound(ArgSpecs[argIdx].Direction)) {
        std::string symName = "sym.in2." + Values2[argIdx]->getName().str() +
                              std::to_string(argIdx);
        builder.CreateCall(symbolizeFunc,
                           {Values2[argIdx], allocSize,
                            builder.CreateGlobalStringPtr(symName)});

        builder.CreateCall(memcpyFunc,
                           {Values2[argIdx], Values1[argIdx], allocSize});
      }
    }

    ++argIdx;
  }
}

void SymbolicEncapsulation::createSymbolicAssertions(
    llvm::BasicBlock &StartBlock, llvm::BasicBlock &EndBlock,
    llvm::SmallVectorImpl<llvm::Value *> &Values1,
    llvm::SmallVectorImpl<llvm::Value *> &Values2,
    IterationsNumTy IterationsNum, llvm::ArrayRef<ArgSpec> ArgSpecs) {
  assert(Values1.size() && Values2.size() && "Value sets are empty!");
  assert(Values1.size() == Values2.size() && "Value set sizes differ!");

  auto &curF = *StartBlock.getParent();
  auto &curM = *curF.getParent();
  auto &dataLayout = curM.getDataLayout();
  auto *assertFunc = DeclareAssertLikeFunc(curM, "__assert_fail");
  auto *memcmpFunc = DeclareKLEELikeFunc(curM, "memcmp");

  llvm::SmallVector<llvm::Value *, 8> conditions;
  llvm::SmallVector<llvm::BasicBlock *, 8> condBlocks, failBlocks;

  llvm::IRBuilder<> builder{&StartBlock};
  auto *funcName = builder.CreateGlobalStringPtr(curF.getName());
  auto *modName = builder.CreateGlobalStringPtr(curM.getName());
  auto *assertText = builder.CreateGlobalStringPtr("FAILED");

  for (size_t i = 0; i < Values1.size(); ++i) {
    if (ArgSpecs.size() && !isOutbound(ArgSpecs[i].Direction)) {
      continue;
    }

    LLVM_DEBUG(llvm::dbgs() << "creating assertion on arg: " << i << '\n';);

    condBlocks.push_back(llvm::BasicBlock::Create(
        curM.getContext(), "cond" + std::to_string(i), &curF));
    failBlocks.push_back(llvm::BasicBlock::Create(
        curM.getContext(), "fail" + std::to_string(i), &curF));

    if (!Values1[i]->getType()->isPointerTy()) {
      builder.SetInsertPoint(condBlocks.back());
      conditions.push_back(builder.CreateICmpEQ(Values1[i], Values2[i]));

      builder.SetInsertPoint(failBlocks.back());
      builder.CreateCall(assertFunc,
                         {assertText, modName, builder.getInt32(0), funcName});
    } else {
      size_t typeSize = dataLayout.getTypeAllocSize(
          Values1[i]->getType()->getPointerElementType());
      assert(typeSize && "type size cannot be zero!");

      builder.SetInsertPoint(condBlocks.back());
      llvm::Value *multiplier = ArgSpecs[i].IteratorDependent
                                    ? builder.getInt64(IterationsNum)
                                    : builder.getInt64(1);

      auto *allocSize =
          builder.CreateMul(multiplier, builder.getInt64(typeSize));

      auto *call =
          builder.CreateCall(memcmpFunc, {Values1[i], Values2[i], allocSize});
      conditions.push_back(builder.CreateICmpEQ(call, builder.getInt32(0)));

      builder.SetInsertPoint(failBlocks.back());
      builder.CreateCall(assertFunc,
                         {assertText, modName, builder.getInt32(0), funcName});
    }
  }

  // add unreachable terminator to fail assert blocks
  for (auto *e : failBlocks) {
    builder.SetInsertPoint(e);
    builder.CreateUnreachable();
  }

  // link condition blocks to their fail assert blocks
  for (size_t i = 0; i < condBlocks.size(); ++i) {
    builder.SetInsertPoint(condBlocks[i]);
    // temporarily point to truth branch to the end block
    builder.CreateCondBr(conditions[i], &EndBlock, failBlocks[i]);
  }

  // link condition blocks on truth
  for (size_t i = 1; condBlocks.size() && (i <= condBlocks.size() - 1); ++i) {
    auto *br =
        llvm::dyn_cast<llvm::BranchInst>(condBlocks[i - 1]->getTerminator());
    br->setSuccessor(0, condBlocks[i]);
  }

  // hook up start and end blocks
  if (!condBlocks.empty()) {
    builder.SetInsertPoint(&StartBlock);
    builder.CreateBr(condBlocks.front());

    auto *br =
        llvm::dyn_cast<llvm::BranchInst>(condBlocks.back()->getTerminator());
    br->setSuccessor(0, &EndBlock);
  } else {
    builder.SetInsertPoint(&StartBlock);
    builder.CreateBr(&EndBlock);
  }
}

void SymbolicEncapsulation::createCall(
    llvm::BasicBlock &Block, llvm::Function &EncapsulatedFunc,
    llvm::SmallVectorImpl<llvm::Value *> &Args) {
  llvm::IRBuilder<> builder{EncapsulatedFunc.getContext()};

  if (auto *term = Block.getTerminator()) {
    builder.SetInsertPoint(term);
  } else {
    builder.SetInsertPoint(&Block);
  }

  llvm::SmallVector<llvm::Value *, 8> actualArgs;
  auto *funcType = EncapsulatedFunc.getFunctionType();
  LLVM_DEBUG(llvm::dbgs() << "creating call to func type: " << *funcType
                          << '\n';);

  for (size_t i = 0; i < Args.size(); ++i) {
    if (Args[i]->getType()->isPointerTy()) {
      if (llvm::isa<llvm::AllocaInst>(Args[i])) {
        actualArgs.push_back(builder.CreateLoad(Args[i]));
      } else {
        actualArgs.push_back(
            builder.CreateBitCast(Args[i], funcType->getParamType(i)));
      }
    } else {
      llvm::Value *arg = Args[i];

      if (funcType->getParamType(i) != arg->getType()) {
        LLVM_DEBUG(llvm::dbgs() << "casting arg " << i << ": " << *arg << " to "
                                << *funcType->getParamType(i) << '\n';);
        arg =
            llvm::CastInst::CreateZExtOrBitCast(arg, funcType->getParamType(i));
      }

      actualArgs.push_back(arg);
    }
  }

  builder.CreateCall(&EncapsulatedFunc, actualArgs);
}

} // namespace ephippion

