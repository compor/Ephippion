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
    llvm::BasicBlock &Postexit, uint64_t Start, uint64_t End, uint64_t Step) {
  assert(Body.size() && "Body blocks is empty!");
  assert(Start != End && "Loop bounds are equal!");

  enum LoopDirection : uint8_t {
    LD_Increasing,
    LD_Decreasing,
  };

  LoopDirection direction = LD_Increasing;
  if (Start > End) {
    LoopDirection direction = LD_Decreasing;
  }

  auto &curCtx = Preheader.getParent()->getContext();

  auto *hdr = llvm::BasicBlock::Create(curCtx, "hdr");
  auto *latch = llvm::BasicBlock::Create(curCtx, "latch");
  auto *exit = llvm::BasicBlock::Create(curCtx, "exit");

  // add induction variable and loop condition
  llvm::IRBuilder<> builder{hdr};
  auto *ind = builder.CreatePHI(llvm::Type::getInt64Ty(curCtx), 2, "i");

  switch (direction) {
  default:
    builder.CreateCondBr(builder.CreateICmpULT(ind, builder.getInt64(End)),
                         Body[0], exit);
    break;
  case LD_Decreasing:
    builder.CreateCondBr(builder.CreateICmpUGT(ind, builder.getInt64(End)),
                         Body[0], exit);
    break;
  }

  // add induction variable step and branch to header
  builder.SetInsertPoint(latch);
  llvm::Value *step = nullptr;
  switch (direction) {
  default:
    step = builder.CreateAdd(ind, builder.getInt64(Step));
    break;
  case LD_Decreasing:
    step = builder.CreateSub(ind, builder.getInt64(Step));
    break;
  }

  builder.CreateBr(hdr);

  // add induction phi inputs
  ind->addIncoming(builder.getInt64(Start), &Preheader);
  ind->addIncoming(step, latch);

  // direct all unterminated body blocks to loop latch
  for (auto *e : Body) {
    if (!e->getTerminator()) {
      builder.SetInsertPoint(&Preheader);
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
