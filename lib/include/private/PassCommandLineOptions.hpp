//
//
//

#pragma once

#include "llvm/Support/CommandLine.h"
// using llvm::cl::OptionCategory

#include <cstdint>
// using uint64_t

extern llvm::cl::OptionCategory EphippionCLCategory;

extern llvm::cl::opt<uint64_t> AllocElementsNum;

extern llvm::cl::list<std::string> FunctionWhiteList;
