//
//
//

#include "Ephippion/Config.hpp"

#include "Ephippion/Util.hpp"

#include "Ephippion/Debug.hpp"

#include "Ephippion/Exchange/JSONTransfer.hpp"

#include "Ephippion/Support/IR/ArgSpec.hpp"

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

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::init
// using llvm::cl::ParseEnvironmentOptions
// using llvm::cl::ResetAllOptionOccurrences

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::basic_file_status
// using llvm::sys::fs::directory_iterator
// using llvm::sys::fs::exists

#include "llvm/Support/Path.h"
// using llvm::sys::path::native
// using llvm::sys::path::filename

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <algorithm>
// using std::find

#include <fstream>
// using std::ifstream

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

  if (!JSONDescriptionFilename.size() and !EphippionReportsDir.size()) {
    return hasChanged;
  }

  SymbolicEncapsulation senc;
  llvm::SmallVector<ArgSpec, 16> argSpecs;
  llvm::SmallVector<llvm::Function *, 32> workList;
  workList.reserve(M.size());

  auto not_in = [](const auto &C, const auto &E) {
    return C.end() == std::find(std::begin(C), std::end(C), E);
  };

  llvm::SmallVector<std::string, 32> FunctionWhiteList;

  if (EphippionFunctionWhiteListFile.getPosition()) {
    std::ifstream wlFile{EphippionFunctionWhiteListFile};

    std::string funcName;
    while (wlFile >> funcName) {
      FunctionWhiteList.push_back(funcName);
    }
  }

  for (auto &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    if (EphippionFunctionWhiteListFile.getPosition() &&
        not_in(FunctionWhiteList, std::string{F.getName()})) {
      LLVM_DEBUG(llvm::dbgs() << "skipping func: " << F.getName()
                              << " reason: not in whitelist\n";);
      continue;
    }

    workList.push_back(&F);
  }

  if (!workList.empty() && EphippionReportsDir.size() &&
      llvm::sys::fs::exists(EphippionReportsDir)) {

    std::error_code EC;
    llvm::SmallString<128> ReportsPathNative;
    llvm::sys::path::native(EphippionReportsDir, ReportsPathNative);
    // walk all of the files within this directory.
    for (llvm::sys::fs::directory_iterator File(ReportsPathNative, EC), FileEnd;
         File != FileEnd && !EC; File.increment(EC)) {
      if (!llvm::sys::path::filename(File->path())
               .startswith(EphippionReportPrefix) ||
          !llvm::sys::path::filename(File->path()).endswith(".json")) {
        continue;
      }

      // if we can't stat it, there's nothing interesting there
      llvm::ErrorOr<llvm::sys::fs::basic_file_status> StatusOrErr =
          File->status();
      if (!StatusOrErr) {
        LLVM_DEBUG(llvm::dbgs() << "ignore " << File->path()
                                << " reason: cannot stat\n";);
        continue;
      }

      argSpecs.clear();
      auto filename = llvm::sys::path::filename(File->path());
      auto valOrError = ReadJSONFromFile(filename, EphippionReportsDir);

      if (!valOrError) {
        LLVM_DEBUG(llvm::dbgs() << "skipping file: " << filename;);
        if (valOrError.getError() != std::errc::no_such_file_or_directory) {
          LLVM_DEBUG(llvm::dbgs()
                         << " reason: could not read description file\n";);
        }
        LLVM_DEBUG(llvm::dbgs() << " reason: could not open file\n";);
        continue;
      }

      auto &v = *valOrError;

      auto funcName = v.getAsObject()->getString("func");
      auto *F = M.getFunction(*funcName);

      auto found = std::find(workList.begin(), workList.end(), F);
      if (!F || found == workList.end()) {
        LLVM_DEBUG(llvm::dbgs() << "skipping func: " << *funcName
                                << " reason: func name mismatch\n";);
        continue;
      }

      auto *asa = v.getAsObject()->getObject("args")->getArray("argspecs");
      for (auto &e : *asa) {
        ArgSpec as;
        llvm::json::fromJSON(e, as);
        argSpecs.push_back(as);
      }

      if (senc.encapsulate(*F, IterationsNum, argSpecs)) {
        hasChanged = true;
        LLVM_DEBUG(llvm::dbgs()
                       << "encapsulating func: " << F->getName() << '\n';);
      }
    }
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
