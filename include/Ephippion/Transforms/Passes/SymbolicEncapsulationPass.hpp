//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "llvm/Pass.h"
// using llvm::ModulePass

#include "llvm/IR/PassManager.h"
// using llvm::ModuleAnalysisManager
// using llvm::PassInfoMixin

namespace llvm {
class Module;
} // namespace llvm

#define EPHIPPION_SYMENCAP_PASS_NAME "eph-encapsulate"

namespace ephippion {

// new passmanager pass
class SymbolicEncapsulationPass
    : public llvm::PassInfoMixin<SymbolicEncapsulationPass> {
public:
  SymbolicEncapsulationPass();

  bool run(llvm::Module &F);
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

// legacy passmanager pass
class SymbolicEncapsulationLegacyPass : public llvm::ModulePass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  SymbolicEncapsulationLegacyPass() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module &M) override;
};

} // namespace ephippion

