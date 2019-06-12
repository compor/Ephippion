//
//
//

#include "Ephippion/Support/IR/LoopIRBuilder.hpp"

#include "llvm/IR/IRBuilder.h"
// using llvm::IRBuilder

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include <cassert>
// using assert

namespace ephippion {

llvm::Instruction *LoopIRBuilder::CreateLoop(
    llvm::ArrayRef<llvm::BasicBlock *> Body, llvm::BasicBlock &Preheader,
    llvm::BasicBlock &Postexit, uint32_t Start, uint32_t End, uint32_t Step) {
  assert(Body.size() && "Body blocks is empty!");
  assert(Start != End && "Loop bounds are equal!");

  enum LoopDirection : uint8_t {
    LD_Increasing,
    LD_Decreasing,
  };

  LoopDirection direction = LD_Increasing;
  if (Start > End) {
    direction = LD_Decreasing;
  }

  auto *curFunc = Preheader.getParent();
  auto &curCtx = curFunc->getContext();

  auto *hdr = llvm::BasicBlock::Create(curCtx, "hdr", curFunc);
  auto *latch = llvm::BasicBlock::Create(curCtx, "latch", curFunc);
  auto *exit = llvm::BasicBlock::Create(curCtx, "exit", curFunc);

  // add induction variable and loop condition
  llvm::IRBuilder<> builder{hdr};
  const unsigned indBitWidth = 64;
  auto *ind =
      builder.CreatePHI(llvm::Type::getIntNTy(curCtx, indBitWidth), 2, "i");

  switch (direction) {
  default:
    builder.CreateCondBr(
        builder.CreateICmpSLT(ind, builder.getIntN(indBitWidth, End)), Body[0],
        exit);
    break;
  case LD_Decreasing:
    builder.CreateCondBr(
        builder.CreateICmpSGE(ind, builder.getIntN(indBitWidth, End)), Body[0],
        exit);
    break;
  }

  // add induction variable step and branch to header
  builder.SetInsertPoint(latch);
  llvm::Value *step = nullptr;
  switch (direction) {
  default:
    step = builder.CreateAdd(ind, builder.getIntN(indBitWidth, Step));
    break;
  case LD_Decreasing:
    step = builder.CreateSub(ind, builder.getIntN(indBitWidth, Step));
    break;
  }

  builder.CreateBr(hdr);

  // add induction phi inputs
  ind->addIncoming(builder.getIntN(indBitWidth, Start), &Preheader);
  ind->addIncoming(step, latch);

  // direct all unterminated body blocks to loop latch
  for (auto *e : Body) {
    if (!e->getTerminator()) {
      builder.SetInsertPoint(e);
      builder.CreateBr(latch);
    }
  }

  // add preheader to header branch
  builder.SetInsertPoint(&Preheader);
  builder.CreateBr(hdr);

  // add exit to postexit branch
  builder.SetInsertPoint(exit);
  builder.CreateBr(&Postexit);

  return ind;
}

} // namespace ephippion
