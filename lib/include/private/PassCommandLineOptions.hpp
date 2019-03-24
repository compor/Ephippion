//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/Utils/ArgSpecParser.hpp"

#include "llvm/Support/CommandLine.h"
// using llvm::cl::OptionCategory

#include <cstdint>
// using uint64_t

extern llvm::cl::OptionCategory EphippionCLCategory;

extern llvm::cl::opt<uint32_t> AllocElementsNum;

extern llvm::cl::list<std::string> FunctionWhiteList;

extern llvm::cl::list<ephippion::ArgSpec> ArgSpecs;

