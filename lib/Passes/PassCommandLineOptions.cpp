//
//
//

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::list
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include <string>
// using std::string

llvm::cl::OptionCategory EphippionCLCategory("Ephipion Pass",
                                             "Options for Ephippion pass");

llvm::cl::opt<uint64_t> AllocElementsNum(
    "eph-alloc-elements", llvm::cl::Hidden,
    llvm::cl::desc("num of heap allocation elements for arrays"),
    llvm::cl::init(5), llvm::cl::cat(EphippionCLCategory));

llvm::cl::list<std::string>
    FunctionWhiteList("eph-func-wl", llvm::cl::Hidden,
                      llvm::cl::desc("process only the specified functions"),
                      llvm::cl::cat(EphippionCLCategory));

llvm::cl::list<ephippion::ArgSpec>
    ArgSpecs("eph-argspec", llvm::cl::Hidden,
             llvm::cl::desc("use this argspec to process function arguments"),
             llvm::cl::cat(EphippionCLCategory));

