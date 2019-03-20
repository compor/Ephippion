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

  auto &curCtx = Body[0]->getParent()->getContext();

  auto *hdr = llvm::BasicBlock::Create(curCtx, "hdr");
  auto *latch = llvm::BasicBlock::Create(curCtx, "latch");
  auto *exit = llvm::BasicBlock::Create(curCtx, "exit");

  // add induction variable and loop condition
  llvm::IRBuilder<> builder{hdr};
  auto *ind = builder.CreatePHI(llvm::Type::getInt64Ty(curCtx), 2, "i");
  builder.CreateCondBr(builder.CreateICmpULT(ind, builder.getInt64(End)),
                       Body[0], exit);

  // add induction variable step and branch to header
  builder.SetInsertPoint(latch);
  auto *step = builder.CreateAdd(ind, builder.getInt64(Step));
  builder.CreateBr(hdr);

  // add induction phi inputs
  ind->setIncomingValue(0, builder.getInt64(Start));
  ind->setIncomingBlock(0, &Preheader);
  ind->setIncomingValue(1, step);
  ind->setIncomingBlock(1, latch);

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
