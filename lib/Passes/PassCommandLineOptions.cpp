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
// using llvm::cl::CommaSeparated
// using llvm::cl::OptionCategory

#include <string>
// using std::string

llvm::cl::OptionCategory EphippionCLCategory("Ephipion Pass",
                                             "Options for Ephippion pass");

llvm::cl::opt<ephippion::SymbolicEncapsulation::IterationsNumTy>
    IterationsNum("eph-iterations", llvm::cl::Hidden,
                  llvm::cl::desc("num of iterations for heap allocations"),
                  llvm::cl::init(1), llvm::cl::cat(EphippionCLCategory));

llvm::cl::list<std::string>
    FunctionWhiteList("eph-func-wl", llvm::cl::Hidden,
                      llvm::cl::desc("process only the specified functions"),
                      llvm::cl::CommaSeparated,
                      llvm::cl::cat(EphippionCLCategory));

llvm::cl::list<ephippion::ArgSpec>
    ArgSpecs("eph-argspec", llvm::cl::Hidden,
             llvm::cl::desc("specify function argument specifications"),
             llvm::cl::CommaSeparated, llvm::cl::cat(EphippionCLCategory));

