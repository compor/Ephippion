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

llvm::cl::opt<bool>
    ForceEncapsulation("eph-force-encapsulation", llvm::cl::Hidden,
                       llvm::cl::desc("force function encapsulation"),
                       llvm::cl::init(false),
                       llvm::cl::cat(EphippionCLCategory));

llvm::cl::opt<ephippion::SymbolicEncapsulation::IterationsNumTy>
    IterationsNum("eph-iterations", llvm::cl::Hidden,
                  llvm::cl::desc("num of iterations for heap allocations"),
                  llvm::cl::init(1), llvm::cl::cat(EphippionCLCategory));

llvm::cl::list<ephippion::ArgSpec>
    ArgSpecs("eph-argspec", llvm::cl::Hidden,
             llvm::cl::desc("specify function argument specifications"),
             llvm::cl::CommaSeparated, llvm::cl::cat(EphippionCLCategory));

llvm::cl::opt<std::string>
    EphippionReportPrefix("eph-report-prefix", llvm::cl::init("lpc."),
                          llvm::cl::desc("reports prefix"),
                          llvm::cl::cat(EphippionCLCategory));

llvm::cl::opt<std::string>
    EphippionReportsDir("eph-reports-dir", llvm::cl::init(""),
                        llvm::cl::desc("reports directory"),
                        llvm::cl::cat(EphippionCLCategory));

llvm::cl::opt<std::string>
    JSONDescriptionFilename("eph-argspec-json", llvm::cl::Hidden,
                            llvm::cl::desc("parse arg specs from JSON file"),
                            llvm::cl::cat(EphippionCLCategory));

llvm::cl::opt<std::string> EphippionFunctionWhiteListFile(
    "eph-func-wl-file", llvm::cl::Hidden,
    llvm::cl::desc("process only the specified functions"),
    llvm::cl::cat(EphippionCLCategory));

