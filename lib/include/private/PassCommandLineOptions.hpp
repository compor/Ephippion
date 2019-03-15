//
//
//

#pragma once

#include "llvm/Support/CommandLine.h"
// using llvm::cl::OptionCategory

extern llvm::cl::OptionCategory EphippionCLCategory;

extern llvm::cl::list<std::string> FunctionWhiteList;
