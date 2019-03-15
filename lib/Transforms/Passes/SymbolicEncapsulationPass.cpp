//
//
//

#include "Ephippion/Config.hpp"

#include "Ephippion/Util.hpp"

#include "Ephippion/Debug.hpp"

#include "Ephippion/Transforms/Passes/SymbolicEncapsulationPass.hpp"

#include "Ephippion/Transforms/SymbolicEncapsulation.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::init
// using llvm::cl::ParseEnvironmentOptions
// using llvm::cl::ResetAllOptionOccurrences

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <algorithm>
// using std::find

#include <iterator>
// using std::begin
// using std::end

#define DEBUG_TYPE EPHIPPION_SYMENCAP_PASS_NAME
#define PASS_CMDLINE_OPTIONS_ENVVAR "SENCAP_CMDLINE_OPTIONS"

// plugin registration for opt

char ephippion::SymbolicEncapsulationLegacyPass::ID = 0;

static llvm::RegisterPass<ephippion::SymbolicEncapsulationLegacyPass>
    X(DEBUG_TYPE, PRJ_CMDLINE_DESC("symbolic encapsulation pass"), false,
      false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerSymbolicEncapsulationLegacyPass(const llvm::PassManagerBuilder &Builder,
                                        llvm::legacy::PassManagerBase &PM) {
  PM.add(new ephippion::SymbolicEncapsulationLegacyPass());

  return;
}

static llvm::RegisterStandardPasses RegisterSymbolicEncapsulationLegacyPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerSymbolicEncapsulationLegacyPass);

//

namespace ephippion {

// new passmanager pass

SymbolicEncapsulationPass::SymbolicEncapsulationPass() {
  llvm::cl::ResetAllOptionOccurrences();
  llvm::cl::ParseEnvironmentOptions(DEBUG_TYPE, PASS_CMDLINE_OPTIONS_ENVVAR);
}

bool SymbolicEncapsulationPass::run(llvm::Module &M) {
  bool hasChanged = false;
  SymbolicEncapsulation senc;

  for (auto &func : M) {
    if (FunctionWhiteList.size()) {
      auto found = std::find(FunctionWhiteList.begin(), FunctionWhiteList.end(),
                             std::string{func.getName()});

      if (found == FunctionWhiteList.end()) {
        continue;
      }
    }

    hasChanged |= senc.encapsulate(M);
  }

  return hasChanged;
}

llvm::PreservedAnalyses
SymbolicEncapsulationPass::run(llvm::Module &M,
                               llvm::ModuleAnalysisManager &MAM) {
  bool hasChanged = run(M);

  return hasChanged ? llvm::PreservedAnalyses::all()
                    : llvm::PreservedAnalyses::none();
}

// legacy passmanager pass

void SymbolicEncapsulationLegacyPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool SymbolicEncapsulationLegacyPass::runOnModule(llvm::Module &M) {
  SymbolicEncapsulationPass pass;
  return pass.run(M);
}

} // namespace ephippion
