//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/Utils/ArgSpecParser.hpp"

#include "Ephippion/Transforms/SymbolicEncapsulation.hpp"

#include "llvm/Support/CommandLine.h"
// using llvm::cl::OptionCategory

#include <cstdint>
// using uint64_t

extern llvm::cl::OptionCategory EphippionCLCategory;

extern llvm::cl::opt<ephippion::SymbolicEncapsulation::IterationsNumTy>
    IterationsNum;

extern llvm::cl::opt<std::string> JSONDescriptionFilename;

extern llvm::cl::opt<std::string> EphippionReportPrefix;

extern llvm::cl::opt<std::string> EphippionReportsDir;

extern llvm::cl::list<ephippion::ArgSpec> ArgSpecs;

extern llvm::cl::opt<std::string> EphippionFunctionWhiteListFile;
